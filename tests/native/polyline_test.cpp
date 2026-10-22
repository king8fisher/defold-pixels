// Native tests for the polyline rasterizer (defold-pixels/src/polyline.cpp).
// They run without Defold. tests/native/run.sh builds and runs them, and leaves
// the binary in build/native-tests:
//
//   build/native-tests/polyline_test [preview.ppm]
//
// The optional argument writes a preview image of the test shapes over white.

#include "polyline.h"
#include "check.h"

#include <math.h>
#include <stdio.h>
#include <vector>

// Straight-alpha RGBA canvas that also counts how often each pixel is plotted.
struct Canvas
{
    int width;
    int height;
    std::vector<float> rgba;
    std::vector<int> plots;
    int outside;

    Canvas(int w, int h) : width(w), height(h), rgba((size_t)w * h * 4, 0.0f), plots((size_t)w * h, 0), outside(0) {}

    float alpha(int x, int y) const { return rgba[((size_t)y * width + x) * 4 + 3]; }
    float channel(int x, int y, int c) const { return rgba[((size_t)y * width + x) * 4 + c]; }

    int max_plots() const
    {
        int m = 0;
        for (size_t i = 0; i < plots.size(); i++)
        {
            m = plots[i] > m ? plots[i] : m;
        }
        return m;
    }

    float max_alpha() const
    {
        float m = 0;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                m = alpha(x, y) > m ? alpha(x, y) : m;
            }
        }
        return m;
    }

    // Covered area in pixels, for a stroke drawn with alpha 255.
    float area() const
    {
        float sum = 0;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                sum += alpha(x, y) / 255.0f;
            }
        }
        return sum;
    }
};

static void plot(void *ctx, int x, int y, float r, float g, float b, float a)
{
    Canvas *c = (Canvas *)ctx;
    if (x < 0 || y < 0 || x >= c->width || y >= c->height)
    {
        c->outside++;
        return;
    }
    size_t i = (size_t)y * c->width + x;
    c->plots[i]++;
    float sa = a / 255.0f;
    float da = c->rgba[i * 4 + 3] / 255.0f;
    float oa = sa + da * (1 - sa);
    float src[3] = {r, g, b};
    for (int k = 0; k < 3; k++)
    {
        float dc = c->rgba[i * 4 + k];
        c->rgba[i * 4 + k] = oa > 0 ? (src[k] * sa + dc * da * (1 - sa)) / oa : 0;
    }
    c->rgba[i * 4 + 3] = oa * 255.0f;
}

static polyline::Style solid(float width, float r, float g, float b, float a)
{
    polyline::Style s;
    s.width = width;
    s.color = {r, g, b, a};
    s.edge_color = s.color;
    return s;
}

static void draw(Canvas &c, const std::vector<float> &xy, const polyline::Style &style)
{
    polyline::Rect bounds = {0, 0, c.width, c.height};
    polyline::draw(xy.data(), (int)xy.size() / 2, style, bounds, plot, &c);
}

static void test_semi_transparent_overlaps()
{
    // Zigzag, hairpin, a self-crossing loop and a straight run with duplicate
    // and collinear points: none may plot a pixel twice or exceed the alpha.
    const std::vector<std::vector<float> > shapes = {
        {24, 40, 96, 216, 160, 64, 232, 200},
        {20, 120, 220, 120, 30, 132},
        {40, 40, 200, 200, 200, 40, 40, 200},
        {10, 10, 60, 10, 60, 10, 120, 10, 200, 10},
    };
    for (size_t i = 0; i < shapes.size(); i++)
    {
        Canvas c(256, 256);
        draw(c, shapes[i], solid(12, 40, 90, 200, 128));
        CHECK(c.max_plots() == 1, "shape %zu: a pixel was plotted %d times", i, c.max_plots());
        CHECK(fabsf(c.max_alpha() - 128) < 0.01f, "shape %zu: max alpha %.2f, expected 128", i, c.max_alpha());
    }
}

static void test_constant_width()
{
    // A single segment covers length x width at any angle.
    const float length = 150, width = 12;
    for (int deg = 0; deg < 180; deg += 15)
    {
        float rad = deg * 3.14159265f / 180.0f;
        float dx = cosf(rad) * length * 0.5f, dy = sinf(rad) * length * 0.5f;
        Canvas c(256, 256);
        draw(c, {128 - dx, 128 - dy, 128 + dx, 128 + dy}, solid(width, 0, 0, 0, 255));
        float expected = length * width;
        CHECK(fabsf(c.area() - expected) / expected < 0.01f, "%d deg: area %.1f, expected %.1f", deg, c.area(), expected);
    }

    // A horizontal stroke covers exactly `width` pixels in each column.
    Canvas c(64, 64);
    draw(c, {8, 32, 56, 32}, solid(12, 0, 0, 0, 255));
    float column = 0;
    for (int y = 0; y < 64; y++)
    {
        column += c.alpha(32, y) / 255.0f;
    }
    CHECK(fabsf(column - 12) < 0.01f, "column coverage %.3f, expected 12", column);
}

static void test_bevel()
{
    // Right turn at (100, 100): east, then south (y grows downward). The
    // outer corner of the turn points north-east.
    Canvas c(200, 200);
    polyline::Style style = solid(20, 0, 0, 0, 255);
    style.color = {255, 0, 0, 255};
    style.edge_color = {0, 0, 255, 255};
    draw(c, {20, 100, 100, 100, 100, 180}, style);

    // A miter would fill the outer corner (110, 90); the bevel cuts it off.
    CHECK(c.alpha(109, 91) == 0, "outer corner is covered (alpha %.1f)", c.alpha(109, 91));
    // Just inside the bevel edge the stroke is covered and has the edge color.
    // The bevel edge runs from (100, 90) to (110, 100); its midpoint is (105, 95).
    CHECK(c.alpha(104, 96) == 255, "inside of bevel not covered (alpha %.1f)", c.alpha(104, 96));
    CHECK(c.channel(104, 96, 2) > 180, "bevel edge is not edge colored (blue %.1f)", c.channel(104, 96, 2));
    // The vertex itself has the center color.
    CHECK(c.channel(100, 100, 0) > 250, "vertex is not center colored (red %.1f)", c.channel(100, 100, 0));
    // The straight edge next to the join has the same edge color as the bevel.
    CHECK(fabsf(c.channel(60, 91, 2) - c.channel(104, 96, 2)) < 40, "edge color differs: straight %.1f, bevel %.1f",
          c.channel(60, 91, 2), c.channel(104, 96, 2));
}

static void test_miter()
{
    // The same right turn as test_bevel: east, then south. The miter fills the
    // outer corner up to its tip at (110, 90).
    Canvas c(200, 200);
    polyline::Style style = solid(20, 0, 0, 0, 255);
    style.color = {255, 0, 0, 255};
    style.edge_color = {0, 0, 255, 255};
    style.join = polyline::JOIN_MITER;
    draw(c, {20, 100, 100, 100, 100, 180}, style);

    CHECK(c.alpha(109, 91) == 255, "outer corner not covered (alpha %.1f)", c.alpha(109, 91));
    CHECK(fabsf(c.alpha(110, 90) - 127.5f) < 1, "tip: alpha %.1f, expected 127.5", c.alpha(110, 90));
    CHECK(c.alpha(111, 89) == 0, "beyond the tip is covered (alpha %.1f)", c.alpha(111, 89));
    // The corner continues the edge color of both straight edges.
    CHECK(c.channel(109, 91, 2) > 180, "miter corner is not edge colored (blue %.1f)", c.channel(109, 91, 2));
    CHECK(c.channel(100, 100, 0) > 250, "vertex is not center colored (red %.1f)", c.channel(100, 100, 0));
    // The miter adds the triangle between the bevel edge and the corner:
    // (100, 90), (110, 90), (110, 100), an area of 50.
    Canvas b(200, 200);
    style.join = polyline::JOIN_BEVEL;
    draw(b, {20, 100, 100, 100, 100, 180}, style);
    CHECK(fabsf(c.area() - b.area() - 50) < 1, "miter adds %.2f pixels over the bevel, expected 50",
          c.area() - b.area());

    // A sharp turn: east, then back west-south-west. Its tip lies about 8 half
    // widths from the vertex, so the limit decides how far the corner reaches.
    const std::vector<float> sharp = {20, 100, 100, 100, 20, 120};
    const float limits[] = {2, 4, 100};
    float reach[3];
    for (int k = 0; k < 3; k++)
    {
        Canvas s(200, 200);
        polyline::Style miter = solid(10, 0, 0, 0, 255);
        miter.join = polyline::JOIN_MITER;
        miter.miter_limit = limits[k];
        draw(s, sharp, miter);
        reach[k] = 0;
        for (int y = 0; y < 200; y++)
        {
            for (int x = 0; x < 200; x++)
            {
                reach[k] = s.alpha(x, y) > 0 && x - 100 > reach[k] ? x - 100 : reach[k];
            }
        }
        CHECK(s.max_plots() == 1, "limit %.0f: a pixel was plotted %d times", limits[k], s.max_plots());
    }
    // The miter is cut at limit x half width along its direction, which points
    // almost straight east here.
    CHECK(reach[0] <= 2 * 5 + 1, "limit 2 reaches %.0f pixels, expected at most 11", reach[0]);
    CHECK(reach[1] <= 4 * 5 + 1 && reach[1] > reach[0], "limit 4 reaches %.0f pixels, expected 11..21", reach[1]);
    CHECK(reach[2] > 35, "unlimited miter reaches %.0f pixels, expected the tip past 35", reach[2]);

    // Semi-transparent miter strokes still plot each pixel once.
    const std::vector<std::vector<float> > shapes = {
        {24, 40, 96, 216, 160, 64, 232, 200},
        {20, 120, 220, 120, 30, 132},
        {40, 40, 200, 200, 200, 40, 40, 200},
    };
    for (size_t i = 0; i < shapes.size(); i++)
    {
        Canvas o(256, 256);
        polyline::Style miter = solid(12, 40, 90, 200, 128);
        miter.join = polyline::JOIN_MITER;
        draw(o, shapes[i], miter);
        CHECK(o.max_plots() == 1, "miter shape %zu: a pixel was plotted %d times", i, o.max_plots());
        CHECK(fabsf(o.max_alpha() - 128) < 0.01f, "miter shape %zu: max alpha %.2f, expected 128", i, o.max_alpha());
    }

    // A full reversal has no tip; the limit alone cuts it, 4 x 4 pixels past
    // the vertex at x = 40.
    Canvas r(64, 64);
    polyline::Style reversal = solid(8, 0, 0, 0, 255);
    reversal.join = polyline::JOIN_MITER;
    draw(r, {10, 32, 40, 32, 20, 32}, reversal);
    CHECK(r.max_plots() == 1, "miter reversal plotted a pixel %d times", r.max_plots());
    CHECK(r.alpha(55, 32) == 255, "miter reversal: alpha %.1f inside its limit", r.alpha(55, 32));
    CHECK(fabsf(r.alpha(56, 32) - 127.5f) < 1, "miter reversal: alpha %.1f at its limit, expected 127.5",
          r.alpha(56, 32));
    CHECK(r.alpha(57, 32) == 0, "miter reversal reaches past its limit");
}

static void test_gradient()
{
    // Horizontal stroke, 20 pixels wide, centered on row 32. The color moves
    // linearly from the center color to the edge color, the same on both sides.
    Canvas c(64, 64);
    polyline::Style style = solid(20, 0, 0, 0, 255);
    style.color = {255, 0, 0, 255};
    style.edge_color = {0, 0, 255, 255};
    draw(c, {8, 32, 56, 32}, style);
    for (int d = 0; d < 10; d++)
    {
        float t = d / 10.0f;
        for (int side = -1; side <= 1; side += 2)
        {
            int y = 32 + side * d;
            CHECK(fabsf(c.channel(32, y, 0) - 255 * (1 - t)) < 1, "row %d: red %.1f, expected %.1f", y, c.channel(32, y, 0),
                  255 * (1 - t));
            CHECK(fabsf(c.channel(32, y, 2) - 255 * t) < 1, "row %d: blue %.1f, expected %.1f", y, c.channel(32, y, 2),
                  255 * t);
            CHECK(c.alpha(32, y) == 255, "row %d: alpha %.1f inside the stroke", y, c.alpha(32, y));
        }
    }

    // A transparent edge color fades the stroke out without darkening it.
    Canvas f(64, 64);
    style.color = {255, 0, 0, 255};
    style.edge_color = {255, 0, 0, 0};
    draw(f, {8, 32, 56, 32}, style);
    CHECK(fabsf(f.alpha(32, 37) - 127.5f) < 1, "half way out: alpha %.1f, expected 127.5", f.alpha(32, 37));
    CHECK(fabsf(f.channel(32, 37, 0) - 255) < 1, "half way out: red %.1f, expected 255", f.channel(32, 37, 0));
}

static void test_point_alphas()
{
    // Alpha factors 1 -> 0 along a horizontal stroke: the alpha on the center
    // row falls linearly from 255 to 0.
    Canvas c(64, 64);
    polyline::Style style = solid(12, 0, 0, 0, 255);
    const float alphas[] = {1, 0};
    style.alphas = alphas;
    draw(c, {8, 32, 56, 32}, style);
    for (int x = 12; x <= 52; x += 8)
    {
        float expected = 255 * (1 - (x - 8) / 48.0f);
        CHECK(fabsf(c.alpha(x, 32) - expected) < 1, "column %d: alpha %.1f, expected %.1f", x, c.alpha(x, 32),
              expected);
    }

    // A faded stroke still plots each pixel once, through joins and crossings.
    Canvas l(256, 256);
    polyline::Style faded = solid(12, 40, 90, 200, 128);
    const float loop_alphas[] = {1, 0.75f, 0.5f, 0.25f};
    faded.alphas = loop_alphas;
    draw(l, {40, 40, 200, 200, 200, 40, 40, 200}, faded);
    CHECK(l.max_plots() == 1, "faded loop plotted a pixel %d times", l.max_plots());
    CHECK(l.max_alpha() < 128.01f, "faded loop: max alpha %.2f, expected at most 128", l.max_alpha());
}

static void test_point_widths()
{
    // Widths 4 -> 20 along a horizontal stroke: each column covers the width
    // interpolated at that column.
    Canvas c(64, 64);
    polyline::Style style = solid(0, 0, 0, 0, 255);
    const float widths[] = {4, 20};
    style.widths = widths;
    draw(c, {8, 32, 56, 32}, style);
    for (int x = 12; x <= 52; x += 8)
    {
        float expected = 4 + 16 * (x - 8) / 48.0f;
        float column = 0;
        for (int y = 0; y < 64; y++)
        {
            column += c.alpha(x, y) / 255.0f;
        }
        CHECK(fabsf(column - expected) < 0.1f, "column %d: coverage %.3f, expected %.3f", x, column, expected);
    }

    // A tapered stroke still plots each pixel once through its joins.
    Canvas z(256, 256);
    polyline::Style tapered = solid(0, 40, 90, 200, 128);
    const float taper[] = {16, 12, 6, 0};
    tapered.widths = taper;
    draw(z, {24, 40, 96, 216, 160, 64, 232, 200}, tapered);
    CHECK(z.max_plots() == 1, "tapered stroke plotted a pixel %d times", z.max_plots());
    CHECK(fabsf(z.max_alpha() - 128) < 0.01f, "tapered stroke: max alpha %.2f, expected 128", z.max_alpha());
    for (size_t i = 0; i < z.rgba.size(); i++)
    {
        if (z.rgba[i] != z.rgba[i])
        {
            CHECK(false, "tapered stroke wrote NaN at channel %zu", i);
            break;
        }
    }

    // Per-point values belong to the input points, so merged duplicates keep
    // the rest aligned: the duplicate's width is dropped, not shifted along.
    Canvas d(64, 64);
    polyline::Style merged = solid(0, 0, 0, 0, 255);
    const float merged_widths[] = {8, 30, 8};
    merged.widths = merged_widths;
    draw(d, {8, 32, 8, 32, 56, 32}, merged);
    float column = 0;
    for (int y = 0; y < 64; y++)
    {
        column += d.alpha(32, y) / 255.0f;
    }
    CHECK(fabsf(column - 8) < 0.1f, "merged duplicate: coverage %.3f, expected 8", column);
}

static void test_bounds()
{
    Canvas c(100, 100);
    polyline::Rect clip = {20, 20, 30, 30};
    std::vector<float> xy = {-50, 35, 150, 35};
    polyline::draw(xy.data(), 2, solid(10, 0, 0, 0, 255), clip, plot, &c);
    CHECK(c.outside == 0, "%d pixels plotted outside the canvas", c.outside);
    for (int y = 0; y < 100; y++)
    {
        for (int x = 0; x < 100; x++)
        {
            bool inside = x >= 20 && x < 50 && y >= 20 && y < 50;
            if (!inside && c.plots[(size_t)y * 100 + x] != 0)
            {
                CHECK(false, "pixel (%d, %d) outside the clip was plotted", x, y);
                return;
            }
        }
    }
    CHECK(c.alpha(35, 35) == 255, "clipped stroke missing inside the clip");
}

static void test_degenerate()
{
    Canvas c(64, 64);
    draw(c, {10, 10}, solid(8, 0, 0, 0, 255));
    draw(c, {10, 10, 10, 10, 10, 10}, solid(8, 0, 0, 0, 255));
    draw(c, {10, 10, 50, 50}, solid(0, 0, 0, 0, 255));
    draw(c, {10, 10, 50, 50}, solid(-4, 0, 0, 0, 255));
    CHECK(c.max_plots() == 0, "degenerate input plotted pixels");

    // A full reversal must still plot each pixel once.
    Canvas r(64, 64);
    draw(r, {10, 32, 50, 32, 20, 32}, solid(8, 0, 0, 0, 128));
    CHECK(r.max_plots() == 1, "reversal plotted a pixel %d times", r.max_plots());
}

static void write_preview(const char *path)
{
    Canvas c(512, 320);
    polyline::Style gradient = solid(16, 0, 0, 0, 255);
    gradient.color = {250, 220, 80, 255};
    gradient.edge_color = {200, 40, 40, 255};
    draw(c, {24, 40, 96, 216, 160, 64, 232, 200}, gradient);
    draw(c, {280, 40, 360, 216, 420, 60, 490, 200}, solid(20, 40, 90, 200, 128));
    draw(c, {300, 120, 480, 120, 310, 140}, solid(10, 20, 140, 60, 160));

    // A wave that thins out and fades from left to right, like a trail.
    std::vector<float> wave, widths, alphas;
    const int steps = 48;
    for (int i = 0; i <= steps; i++)
    {
        float f = (float)i / steps;
        wave.push_back(24 + 464 * f);
        wave.push_back(276 + 24 * sinf(f * 4 * 3.14159265f));
        widths.push_back(16 * (1 - f));
        alphas.push_back(1 - f);
    }
    polyline::Style trail = gradient;
    trail.color.a = 200;
    trail.edge_color.a = 200;
    trail.widths = widths.data();
    trail.alphas = alphas.data();
    draw(c, wave, trail);

    FILE *f = fopen(path, "wb");
    if (!f)
    {
        printf("cannot write %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", c.width, c.height);
    for (int y = 0; y < c.height; y++)
    {
        for (int x = 0; x < c.width; x++)
        {
            float a = c.alpha(x, y) / 255.0f;
            for (int k = 0; k < 3; k++)
            {
                float v = c.channel(x, y, k) * a + 255.0f * (1 - a);
                fputc((int)(v + 0.5f), f);
            }
        }
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    test_semi_transparent_overlaps();
    test_constant_width();
    test_bevel();
    test_miter();
    test_gradient();
    test_point_alphas();
    test_point_widths();
    test_bounds();
    test_degenerate();
    if (argc > 1)
    {
        write_preview(argv[1]);
    }
    return report("polyline");
}
