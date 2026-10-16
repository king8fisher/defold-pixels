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
    Canvas c(512, 256);
    polyline::Style gradient = solid(16, 0, 0, 0, 255);
    gradient.color = {250, 220, 80, 255};
    gradient.edge_color = {200, 40, 40, 255};
    draw(c, {24, 40, 96, 216, 160, 64, 232, 200}, gradient);
    draw(c, {280, 40, 360, 216, 420, 60, 490, 200}, solid(20, 40, 90, 200, 128));
    draw(c, {300, 120, 480, 120, 310, 140}, solid(10, 20, 140, 60, 160));

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
    test_gradient();
    test_bounds();
    test_degenerate();
    if (argc > 1)
    {
        write_preview(argv[1]);
    }
    return report("polyline");
}
