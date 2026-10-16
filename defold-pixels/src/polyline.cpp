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

        struct Segment
        {
            Vec a;
            Vec b;
            // Unit direction from a to b.
            Vec dir;
            // Unit normal on the left of dir.
            Vec normal;
            float length;
        };

        // Bevel geometry at an interior vertex.
        struct Join
        {
            Vec p;
            // Unit normal of the bevel edge, pointing away from the turn.
            Vec m;
            // Distance from p to the bevel edge along m.
            float h;
            // +1 when the outer side of the turn is left of the path, -1 when right.
            float side;
        };

        struct Shape
        {
            std::vector<Segment> segments;
            // Indexed by vertex; only interior vertices 1 .. segments.size() - 1 are set.
            std::vector<Join> joins;
            float half_width;
        };

        void build_shape(const float *xy, int count, float half_width, Shape &shape)
        {
            shape.half_width = half_width;

            std::vector<Vec> points;
            points.reserve(count);
            for (int i = 0; i < count; i++)
            {
                Vec p = {xy[i * 2], xy[i * 2 + 1]};
                if (points.empty() || length(sub(p, points.back())) >= MIN_SEGMENT_LENGTH)
                {
                    points.push_back(p);
                }
            }
            if (points.size() < 2)
            {
                return;
            }

            for (size_t i = 0; i + 1 < points.size(); i++)
            {
                Segment s;
                s.a = points[i];
                s.b = points[i + 1];
                Vec d = sub(s.b, s.a);
                s.length = length(d);
                s.dir = scale(d, 1.0f / s.length);
                s.normal = {-s.dir.y, s.dir.x};
                shape.segments.push_back(s);
            }

            shape.joins.resize(points.size());
            for (size_t v = 1; v + 1 < points.size(); v++)
            {
                const Segment &in = shape.segments[v - 1];
                const Segment &out = shape.segments[v];
                Join &j = shape.joins[v];
                j.p = points[v];
                // A left turn has its outer side on the right.
                j.side = cross(in.dir, out.dir) > 0 ? -1.0f : 1.0f;
                Vec n_in = scale(in.normal, j.side);
                Vec n_out = scale(out.normal, j.side);
                Vec sum = add(n_in, n_out);
                float sum_length = length(sum);
                if (sum_length < 1e-6f)
                {
                    // The path reverses: the bevel is a flat cut through the vertex.
                    j.m = in.dir;
                    j.h = 0;
                }
                else
                {
                    j.m = scale(sum, 1.0f / sum_length);
                    j.h = half_width * dot(n_in, j.m);
                }
            }
        }

        // Signed distance from q to the part of the stroke owned by segment i
        // (negative inside), and the signed position across the stroke: 0 on the
        // center line, -1/+1 at the right/left edge.
        void evaluate(const Shape &shape, size_t i, Vec q, float &sd, float &pos)
        {
            const Segment &s = shape.segments[i];
            const float hw = shape.half_width;
            Vec d = sub(q, s.a);
            float u = dot(d, s.dir);
            float v = dot(d, s.normal);
            if (u >= 0 && u <= s.length)
            {
                sd = fabsf(v) - hw;
                pos = v / hw;
                return;
            }

            bool before_start = u < 0;
            size_t vertex = before_start ? i : i + 1;
            if (vertex == 0 || vertex == shape.segments.size())
            {
                // Butt cap: the stroke ends flat at the end point.
                float beyond = before_start ? -u : u - s.length;
                sd = fmaxf(fabsf(v) - hw, beyond);
                pos = v / hw;
                return;
            }

            // Bevel join: the round area around the vertex, cut by the bevel edge.
            const Join &j = shape.joins[vertex];
            Vec r = sub(q, j.p);
            float dist = length(r);
            float along = dot(r, j.m);
            sd = fmaxf(dist - hw, along - j.h);
            // Measure against the bevel edge instead of the half width, so the
            // bevel edge gets the same edge color as the straight segments.
            float t = j.h > 1e-6f ? along / j.h : dist / hw;
            pos = j.side * clamp01(t);
        }

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
        if (count < 2 || !(style.width > 0) || bounds.w <= 0 || bounds.h <= 0)
        {
            return;
        }

        Shape shape;
        build_shape(xy, count, style.width * 0.5f, shape);
        if (shape.segments.empty())
        {
            return;
        }
        const float reach = shape.half_width + AA_MARGIN;

        // Mask area: the boxes of all segments, limited to bounds.
        float min_x = FLT_MAX, min_y = FLT_MAX, max_x = -FLT_MAX, max_y = -FLT_MAX;
        for (size_t i = 0; i < shape.segments.size(); i++)
        {
            Vec corners[4];
            segment_box(shape.segments[i], reach, corners);
            for (int k = 0; k < 4; k++)
            {
                min_x = fminf(min_x, corners[k].x);
                max_x = fmaxf(max_x, corners[k].x);
                min_y = fminf(min_y, corners[k].y);
                max_y = fmaxf(max_y, corners[k].y);
            }
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

        // Pass 1: combine all parts. The union of shapes keeps the smallest
        // signed distance, so overlaps are resolved before anything is drawn.
        for (size_t i = 0; i < shape.segments.size(); i++)
        {
            Vec corners[4];
            segment_box(shape.segments[i], reach, corners);
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
                    float sd, pos;
                    evaluate(shape, i, {(float)x, (float)y}, sd, pos);
                    size_t idx = row + (x - ax0);
                    if (sd < mask_sd[idx])
                    {
                        mask_sd[idx] = sd;
                        mask_pos[idx] = pos;
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
                if (a <= 0)
                {
                    continue;
                }
                float w0 = c0.a * (1 - t) / a;
                float w1 = c1.a * t / a;
                plot(ctx, x, y,
                     c0.r * w0 + c1.r * w1,
                     c0.g * w0 + c1.g * w1,
                     c0.b * w0 + c1.b * w1,
                     a * coverage);
            }
        }
    }
}
