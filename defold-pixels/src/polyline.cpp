#include "polyline.h"

#include <float.h>
#include <math.h>
#include <vector>

namespace polyline
{
    namespace
    {
        // Consecutive points closer than this are merged into one.
        const float MIN_SEGMENT_LENGTH = 1e-3f;
        // Extra reach around each part for the one-pixel anti-aliasing ramp.
        const float AA_MARGIN = 1.5f;

        struct Vec
        {
            float x;
            float y;
        };

        inline Vec add(Vec a, Vec b) { return {a.x + b.x, a.y + b.y}; }
        inline Vec sub(Vec a, Vec b) { return {a.x - b.x, a.y - b.y}; }
        inline Vec scale(Vec a, float s) { return {a.x * s, a.y * s}; }
        inline float dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y; }
        inline float cross(Vec a, Vec b) { return a.x * b.y - a.y * b.x; }
        inline float length(Vec a) { return sqrtf(dot(a, a)); }
        inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
        // Position v across a stroke of half width hw; a zero width counts as its edge.
        inline float across(float v, float hw) { return hw > 1e-6f ? v / hw : (v < 0 ? -1.0f : 1.0f); }

        struct Segment
        {
            Vec a;
            Vec b;
            // Unit direction from a to b.
            Vec dir;
            // Unit normal on the left of dir.
            Vec normal;
            float length;
            // Half width and alpha factor at a and at b.
            float hw_a;
            float hw_b;
            float alpha_a;
            float alpha_b;
            // How far the segment and the joins at its ends reach from its end
            // points, without anti-aliasing.
            float reach;
        };

        // Join geometry at an interior vertex.
        struct Join
        {
            Vec p;
            // Unit normal of the bevel edge, pointing away from the turn.
            Vec m;
            // Distance from p to the bevel edge along m.
            float h;
            // +1 when the outer side of the turn is left of the path, -1 when right.
            float side;
            // Half width and alpha factor at the vertex.
            float hw;
            float alpha;
            bool miter;
            // Normals of the incoming and outgoing segments on the outer side.
            Vec n_in;
            Vec n_out;
            // Distance from p along m where the miter ends: at its tip, or cut
            // flat at the miter limit.
            float limit;
        };

        struct Shape
        {
            std::vector<Segment> segments;
            // Indexed by vertex; only interior vertices 1 .. segments.size() - 1 are set.
            std::vector<Join> joins;
        };

        // A path point with its own half width and alpha factor.
        struct Vertex
        {
            Vec p;
            float hw;
            float alpha;
        };

        void build_shape(const float *xy, int count, const Style &style, Shape &shape)
        {
            std::vector<Vertex> points;
            points.reserve(count);
            for (int i = 0; i < count; i++)
            {
                Vertex v;
                v.p = {xy[i * 2], xy[i * 2 + 1]};
                v.hw = fmaxf(0.0f, (style.widths ? style.widths[i] : style.width) * 0.5f);
                v.alpha = style.alphas ? clamp01(style.alphas[i]) : 1.0f;
                // A merged duplicate keeps the values of the first point.
                if (points.empty() || length(sub(v.p, points.back().p)) >= MIN_SEGMENT_LENGTH)
                {
                    points.push_back(v);
                }
            }
            if (points.size() < 2)
            {
                return;
            }

            for (size_t i = 0; i + 1 < points.size(); i++)
            {
                Segment s;
                s.a = points[i].p;
                s.b = points[i + 1].p;
                Vec d = sub(s.b, s.a);
                s.length = length(d);
                s.dir = scale(d, 1.0f / s.length);
                s.normal = {-s.dir.y, s.dir.x};
                s.hw_a = points[i].hw;
                s.hw_b = points[i + 1].hw;
                s.alpha_a = points[i].alpha;
                s.alpha_b = points[i + 1].alpha;
                s.reach = fmaxf(s.hw_a, s.hw_b);
                shape.segments.push_back(s);
            }

            shape.joins.resize(points.size());
            const float miter_limit = fmaxf(1.0f, style.miter_limit);
            for (size_t v = 1; v + 1 < points.size(); v++)
            {
                Segment &in = shape.segments[v - 1];
                Segment &out = shape.segments[v];
                Join &j = shape.joins[v];
                j.p = points[v].p;
                j.hw = points[v].hw;
                j.alpha = points[v].alpha;
                // A left turn has its outer side on the right.
                j.side = cross(in.dir, out.dir) > 0 ? -1.0f : 1.0f;
                j.n_in = scale(in.normal, j.side);
                j.n_out = scale(out.normal, j.side);
                Vec sum = add(j.n_in, j.n_out);
                float sum_length = length(sum);
                float cos_half = 0;
                if (sum_length < 1e-6f)
                {
                    // The path reverses: the bevel is a flat cut through the vertex.
                    j.m = in.dir;
                    j.h = 0;
                }
                else
                {
                    j.m = scale(sum, 1.0f / sum_length);
                    cos_half = dot(j.n_in, j.m);
                    j.h = j.hw * cos_half;
                }

                j.miter = style.join == JOIN_MITER;
                if (j.miter)
                {
                    // The tip lies hw / cos_half from the vertex. Past the limit,
                    // and for a reversal, which has no tip, the miter is cut flat.
                    j.limit = cos_half * miter_limit > 1 ? j.hw / cos_half : miter_limit * j.hw;
                    in.reach = fmaxf(in.reach, j.limit);
                    out.reach = fmaxf(out.reach, j.limit);
                }
                else
                {
                    j.limit = j.h;
                }
            }
        }

        // Signed distance from q to the part of the stroke owned by segment i
        // (negative inside), and the signed position across the stroke: 0 on the
        // center line, -1/+1 at the right/left edge. `alpha` is the alpha factor
        // there.
        void evaluate(const Shape &shape, size_t i, Vec q, float &sd, float &pos, float &alpha)
        {
            const Segment &s = shape.segments[i];
            Vec d = sub(q, s.a);
            float u = dot(d, s.dir);
            float v = dot(d, s.normal);
            if (u >= 0 && u <= s.length)
            {
                // A changing width makes the edge slightly slanted; |v| - hw is
                // then a close stand-in for the true distance.
                float f = u / s.length;
                float hw = s.hw_a + (s.hw_b - s.hw_a) * f;
                sd = fabsf(v) - hw;
                pos = across(v, hw);
                alpha = s.alpha_a + (s.alpha_b - s.alpha_a) * f;
                return;
            }

            bool before_start = u < 0;
            size_t vertex = before_start ? i : i + 1;
            if (vertex == 0 || vertex == shape.segments.size())
            {
                // Butt cap: the stroke ends flat at the end point.
                float hw = before_start ? s.hw_a : s.hw_b;
                float beyond = before_start ? -u : u - s.length;
                sd = fmaxf(fabsf(v) - hw, beyond);
                pos = across(v, hw);
                alpha = before_start ? s.alpha_a : s.alpha_b;
                return;
            }

            const Join &j = shape.joins[vertex];
            Vec r = sub(q, j.p);
            float along = dot(r, j.m);
            // The miter fills only the corner between the end of the incoming
            // segment and the start of the outgoing one. Past either, the outer
            // edges run on as straight lines that leave the stroke when a
            // neighbor segment is short, so the bevel shape takes over there.
            bool in_corner = dot(r, shape.segments[vertex - 1].dir) >= 0 && dot(r, shape.segments[vertex].dir) <= 0;
            if (j.miter && along > 0 && in_corner)
            {
                // Miter join, outer side: both outer edges extended until they
                // meet, cut flat at the limit. The inner side is covered by the
                // segments themselves.
                float edge = fmaxf(dot(r, j.n_in), dot(r, j.n_out));
                sd = fmaxf(edge - j.hw, along - j.limit);
                pos = j.side * clamp01(across(edge, j.hw));
                alpha = j.alpha;
                return;
            }

            // Bevel join: the round area around the vertex, cut by the bevel edge.
            float dist = length(r);
            sd = fmaxf(dist - j.hw, along - j.h);
            // Measure against the bevel edge instead of the half width, so the
            // bevel edge gets the same edge color as the straight segments.
            float t = j.h > 1e-6f ? along / j.h : across(dist, j.hw);
            pos = j.side * clamp01(t);
            alpha = j.alpha;
        }

        // How far segment s and its joins can cover from its center line, anti-aliasing included.
        float segment_reach(const Segment &s) { return s.reach + AA_MARGIN; }

        // Corners of a box that contains everything segment s can cover.
        void segment_box(const Segment &s, float reach, Vec corners[4])
        {
            Vec back = sub(s.a, scale(s.dir, reach));
            Vec front = add(s.b, scale(s.dir, reach));
            Vec side = scale(s.normal, reach);
            corners[0] = add(back, side);
            corners[1] = add(front, side);
            corners[2] = sub(front, side);
            corners[3] = sub(back, side);
        }

        // Horizontal extent of the convex quad at height y; false when y misses it.
        bool row_span(const Vec corners[4], float y, float &min_x, float &max_x)
        {
            min_x = FLT_MAX;
            max_x = -FLT_MAX;
            for (int k = 0; k < 4; k++)
            {
                Vec p0 = corners[k];
                Vec p1 = corners[(k + 1) % 4];
                float lo = fminf(p0.y, p1.y);
                float hi = fmaxf(p0.y, p1.y);
                if (y < lo || y > hi)
                {
                    continue;
                }
                if (hi - lo < 1e-6f)
                {
                    min_x = fminf(min_x, fminf(p0.x, p1.x));
                    max_x = fmaxf(max_x, fmaxf(p0.x, p1.x));
                    continue;
                }
                float x = p0.x + (y - p0.y) * (p1.x - p0.x) / (p1.y - p0.y);
                min_x = fminf(min_x, x);
                max_x = fmaxf(max_x, x);
            }
            return min_x <= max_x;
        }

        // First and last whole pixel inside [lo, hi], limited to [first, last].
        bool pixel_range(float lo, float hi, int first, int last, int &from, int &to)
        {
            float f = fmaxf(ceilf(lo), (float)first);
            float t = fminf(floorf(hi), (float)last);
            if (!(f <= t))
            {
                return false;
            }
            from = (int)f;
            to = (int)t;
            return true;
        }
    }

    void draw(const float *xy, int count, const Style &style, const Rect &bounds, PlotFn plot, void *ctx)
    {
        if (count < 2 || (!style.widths && !(style.width > 0)) || bounds.w <= 0 || bounds.h <= 0)
        {
            return;
        }

        Shape shape;
        build_shape(xy, count, style, shape);
        if (shape.segments.empty())
        {
            return;
        }

        // Mask area: the boxes of all segments, limited to bounds.
        float min_x = FLT_MAX, min_y = FLT_MAX, max_x = -FLT_MAX, max_y = -FLT_MAX;
        float max_hw = 0;
        for (size_t i = 0; i < shape.segments.size(); i++)
        {
            const Segment &s = shape.segments[i];
            max_hw = fmaxf(max_hw, fmaxf(s.hw_a, s.hw_b));
            Vec corners[4];
            segment_box(s, segment_reach(s), corners);
            for (int k = 0; k < 4; k++)
            {
                min_x = fminf(min_x, corners[k].x);
                max_x = fmaxf(max_x, corners[k].x);
                min_y = fminf(min_y, corners[k].y);
                max_y = fmaxf(max_y, corners[k].y);
            }
        }
        if (!(max_hw > 0))
        {
            return;
        }
        int ax0, ax1, ay0, ay1;
        if (!pixel_range(min_x, max_x, bounds.x, bounds.x + bounds.w - 1, ax0, ax1) ||
            !pixel_range(min_y, max_y, bounds.y, bounds.y + bounds.h - 1, ay0, ay1))
        {
            return;
        }
        const int mask_w = ax1 - ax0 + 1;
        const int mask_h = ay1 - ay0 + 1;
        std::vector<float> mask_sd((size_t)mask_w * mask_h, FLT_MAX);
        std::vector<float> mask_pos((size_t)mask_w * mask_h, 0.0f);
        std::vector<float> mask_alpha((size_t)mask_w * mask_h, 1.0f);

        // Pass 1: combine all parts. The union of shapes keeps the smallest
        // signed distance, so overlaps are resolved before anything is drawn.
        for (size_t i = 0; i < shape.segments.size(); i++)
        {
            Vec corners[4];
            segment_box(shape.segments[i], segment_reach(shape.segments[i]), corners);
            float lo_y = FLT_MAX, hi_y = -FLT_MAX;
            for (int k = 0; k < 4; k++)
            {
                lo_y = fminf(lo_y, corners[k].y);
                hi_y = fmaxf(hi_y, corners[k].y);
            }
            int y0, y1;
            if (!pixel_range(lo_y, hi_y, ay0, ay1, y0, y1))
            {
                continue;
            }
            for (int y = y0; y <= y1; y++)
            {
                float lo_x, hi_x;
                int x0, x1;
                if (!row_span(corners, (float)y, lo_x, hi_x) || !pixel_range(lo_x, hi_x, ax0, ax1, x0, x1))
                {
                    continue;
                }
                size_t row = (size_t)(y - ay0) * mask_w;
                for (int x = x0; x <= x1; x++)
                {
                    float sd, pos, alpha;
                    evaluate(shape, i, {(float)x, (float)y}, sd, pos, alpha);
                    size_t idx = row + (x - ax0);
                    if (sd < mask_sd[idx])
                    {
                        mask_sd[idx] = sd;
                        mask_pos[idx] = pos;
                        mask_alpha[idx] = alpha;
                    }
                }
            }
        }

        // Pass 2: emit each covered pixel once.
        const Rgba &c0 = style.color;
        const Rgba &c1 = style.edge_color;
        for (int y = ay0; y <= ay1; y++)
        {
            size_t row = (size_t)(y - ay0) * mask_w;
            for (int x = ax0; x <= ax1; x++)
            {
                float sd = mask_sd[row + (x - ax0)];
                if (sd >= 0.5f)
                {
                    continue;
                }
                float coverage = fminf(1.0f, 0.5f - sd);
                float t = clamp01(fabsf(mask_pos[row + (x - ax0)]));
                // Interpolate premultiplied colors so a transparent edge color
                // fades the stroke instead of darkening it.
                float a = c0.a + (c1.a - c0.a) * t;
                float fade = mask_alpha[row + (x - ax0)];
                if (a <= 0 || fade <= 0)
                {
                    continue;
                }
                float w0 = c0.a * (1 - t) / a;
                float w1 = c1.a * t / a;
                plot(ctx, x, y,
                     c0.r * w0 + c1.r * w1,
                     c0.g * w0 + c1.g * w1,
                     c0.b * w0 + c1.b * w1,
                     a * coverage * fade);
            }
        }
    }
}
