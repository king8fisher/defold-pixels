// Native tests for the pixel blend formulas (defold-pixels/src/blend.cpp).
// Run them with tests/native/run.sh.

#include "blend.h"
#include "check.h"

#include <math.h>
#include <stdlib.h>

struct Pixel
{
    uint8_t c[4];
};

static Pixel rgba(int r, int g, int b, int a)
{
    Pixel p = {{(uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a}};
    return p;
}

static bool near(int actual, int expected, int tolerance)
{
    return abs(actual - expected) <= tolerance;
}

static void test_premultiplied_over_empty()
{
    // Half-transparent white over an empty pixel stores half of each channel.
    // The earlier code blended this pixel twice and stored (212, 212, 212, 191).
    Pixel p = rgba(0, 0, 0, 0);
    blend::premultiplied_over(p.c, 255, 255, 255, 128);
    for (int k = 0; k < 4; k++)
    {
        CHECK(p.c[k] == 128, "channel %d is %d, expected 128", k, p.c[k]);
    }

    Pixel q = rgba(0, 0, 0, 0);
    blend::premultiplied_over(q.c, 200, 100, 50, 128);
    CHECK(q.c[0] == 100 && q.c[1] == 50 && q.c[2] == 25 && q.c[3] == 128, "got (%d, %d, %d, %d), expected (100, 50, 25, 128)",
          q.c[0], q.c[1], q.c[2], q.c[3]);
}

static void test_premultiplied_over_limits()
{
    Pixel p = rgba(10, 20, 30, 40);
    blend::premultiplied_over(p.c, 255, 255, 255, 0);
    CHECK(p.c[0] == 10 && p.c[1] == 20 && p.c[2] == 30 && p.c[3] == 40, "alpha 0 changed the pixel to (%d, %d, %d, %d)",
          p.c[0], p.c[1], p.c[2], p.c[3]);

    blend::premultiplied_over(p.c, 200, 100, 50, 255);
    CHECK(p.c[0] == 200 && p.c[1] == 100 && p.c[2] == 50 && p.c[3] == 255, "alpha 255 gave (%d, %d, %d, %d)", p.c[0],
          p.c[1], p.c[2], p.c[3]);
}

static void test_premultiplied_over_stacks()
{
    // Two half-transparent layers cover 1 - 0.5 * 0.5 = 75% of the pixel.
    Pixel p = rgba(0, 0, 0, 0);
    blend::premultiplied_over(p.c, 255, 0, 0, 128);
    blend::premultiplied_over(p.c, 255, 0, 0, 128);
    CHECK(near(p.c[3], 192, 1), "alpha %d, expected about 192", p.c[3]);
    CHECK(p.c[0] == p.c[3], "red %d differs from alpha %d for an opaque-red stroke", p.c[0], p.c[3]);
    CHECK(p.c[1] == 0 && p.c[2] == 0, "green %d, blue %d, expected 0", p.c[1], p.c[2]);
}

// A premultiplied color channel can never exceed its alpha.
static void test_premultiplied_stays_valid()
{
    for (int da = 0; da <= 255; da += 15)
    {
        for (int dc = 0; dc <= da; dc += 15)
        {
            for (int sa = 0; sa <= 255; sa += 15)
            {
                for (int sc = 0; sc <= 255; sc += 51)
                {
                    Pixel p = rgba(dc, dc, dc, da);
                    blend::premultiplied_over(p.c, sc, sc, sc, sa);
                    if (p.c[0] > p.c[3])
                    {
                        CHECK(false, "dst (%d, %d) + src (%d, %d): color %d exceeds alpha %d", dc, da, sc, sa, p.c[0],
                              p.c[3]);
                        return;
                    }
                }
            }
        }
    }
}

// Exact source-over in 0..255 units. dc is the premultiplied destination color;
// out_c is the premultiplied result color.
static void source_over(double dc, double da, double sc, double sa, double &out_c, double &out_a)
{
    double k = sa / 255.0;
    out_c = sc * k + dc * (1 - k);
    out_a = sa + da * (1 - k);
}

// Both formulas compute the same exact blend; they only store it differently.
// premultiplied_over rounds, so it stays within half a unit. straight_over
// truncates, so it can be up to one unit low.
static void test_formulas_match_exact_blend()
{
    for (int da = 0; da <= 255; da += 17)
    {
        for (int dc = 0; dc <= 255; dc += 51)
        {
            for (int sa = 0; sa <= 255; sa += 17)
            {
                for (int sc = 0; sc <= 255; sc += 51)
                {
                    double out_c, out_a;

                    int dp = dc * da / 255;
                    Pixel p = rgba(dp, dp, dp, da);
                    blend::premultiplied_over(p.c, sc, sc, sc, sa);
                    source_over(dp, da, sc, sa, out_c, out_a);
                    if (fabs(p.c[0] - out_c) > 0.501 || fabs(p.c[3] - out_a) > 0.501)
                    {
                        CHECK(false, "premultiplied dst (%d, %d) + src (%d, %d): got (%d, %d), exact (%.2f, %.2f)", dp, da,
                              sc, sa, p.c[0], p.c[3], out_c, out_a);
                        return;
                    }

                    Pixel s = rgba(dc, dc, dc, da);
                    blend::straight_over(s.c, sc, sc, sc, sa);
                    source_over(dc * da / 255.0, da, sc, sa, out_c, out_a);
                    double straight_c = out_a > 0 ? out_c / out_a * 255 : 0;
                    if (fabs(s.c[0] - straight_c) > 1.001 || fabs(s.c[3] - out_a) > 1.001)
                    {
                        CHECK(false, "straight dst (%d, %d) + src (%d, %d): got (%d, %d), exact (%.2f, %.2f)", dc, da, sc,
                              sa, s.c[0], s.c[3], straight_c, out_a);
                        return;
                    }
                }
            }
        }
    }
}

static void test_straight_over()
{
    // Over an empty pixel the source is stored as is.
    Pixel p = rgba(0, 0, 0, 0);
    blend::straight_over(p.c, 200, 100, 50, 128);
    CHECK(near(p.c[0], 200, 1) && near(p.c[1], 100, 1) && near(p.c[2], 50, 1) && near(p.c[3], 128, 1),
          "got (%d, %d, %d, %d), expected (200, 100, 50, 128)", p.c[0], p.c[1], p.c[2], p.c[3]);

    // Over an opaque pixel the colors mix and the pixel stays opaque.
    Pixel q = rgba(0, 0, 0, 255);
    blend::straight_over(q.c, 255, 255, 255, 128);
    CHECK(near(q.c[0], 128, 1) && q.c[3] == 255, "got (%d, %d, %d, %d), expected (128, 128, 128, 255)", q.c[0], q.c[1],
          q.c[2], q.c[3]);
}

static void test_rgb_over()
{
    uint8_t p[3] = {10, 20, 30};
    blend::rgb_over(p, 255, 255, 255, 0);
    CHECK(p[0] == 10 && p[1] == 20 && p[2] == 30, "alpha 0 changed the pixel to (%d, %d, %d)", p[0], p[1], p[2]);

    blend::rgb_over(p, 200, 100, 50, 255);
    CHECK(p[0] == 200 && p[1] == 100 && p[2] == 50, "alpha 255 gave (%d, %d, %d)", p[0], p[1], p[2]);

    uint8_t q[3] = {0, 100, 200};
    blend::rgb_over(q, 200, 100, 0, 128);
    CHECK(near(q[0], 100, 1) && q[1] == 100 && near(q[2], 100, 1), "alpha 128 gave (%d, %d, %d), expected (100, 100, 100)",
          q[0], q[1], q[2]);
}

static const blend::Format RGB = {3, false};
static const blend::Format STRAIGHT = {4, false};
static const blend::Format PREMULTIPLIED = {4, true};

// Checks the first `channels` bytes of p against the expected values.
static void expect(const char *label, const uint8_t *p, int channels, int r, int g, int b, int a, int tolerance)
{
    const int expected[4] = {r, g, b, a};
    for (int k = 0; k < channels; k++)
    {
        if (!near(p[k], expected[k], tolerance))
        {
            CHECK(false, "%s: got (%d, %d, %d, %d), expected (%d, %d, %d, %d)", label, p[0], p[1], p[2],
                  channels == 4 ? p[3] : -1, r, g, b, channels == 4 ? a : -1);
            return;
        }
    }
}

static void test_copy_pixel()
{
    Pixel p;
    Pixel straight = rgba(200, 100, 50, 128);
    Pixel premultiplied = rgba(100, 50, 25, 128);
    uint8_t rgb[4] = {10, 20, 30, 77};

    p = rgba(1, 2, 3, 4);
    blend::copy_pixel(p.c, PREMULTIPLIED, straight.c, STRAIGHT);
    expect("straight to premultiplied", p.c, 4, 100, 50, 25, 128, 0);

    p = rgba(1, 2, 3, 4);
    blend::copy_pixel(p.c, STRAIGHT, premultiplied.c, PREMULTIPLIED);
    expect("premultiplied to straight", p.c, 4, 199, 100, 50, 128, 0);

    p = rgba(1, 2, 3, 4);
    blend::copy_pixel(p.c, RGB, premultiplied.c, PREMULTIPLIED);
    expect("premultiplied to rgb", p.c, 4, 199, 100, 50, 4, 0);

    p = rgba(1, 2, 3, 4);
    blend::copy_pixel(p.c, PREMULTIPLIED, rgb, RGB);
    expect("rgb to premultiplied", p.c, 4, 10, 20, 30, 255, 0);

    p = rgba(1, 2, 3, 4);
    blend::copy_pixel(p.c, STRAIGHT, rgb, RGB);
    expect("rgb to straight", p.c, 4, 10, 20, 30, 255, 0);

    Pixel empty = rgba(0, 0, 0, 0);
    p = rgba(1, 2, 3, 4);
    blend::copy_pixel(p.c, STRAIGHT, empty.c, PREMULTIPLIED);
    expect("transparent premultiplied to straight", p.c, 4, 0, 0, 0, 0, 0);

    // Copying into the same format never changes a pixel.
    for (int a = 0; a <= 255; a++)
    {
        for (int c = 0; c <= a; c += 3)
        {
            Pixel src = rgba(c, a - c, c / 2, a);
            blend::copy_pixel(p.c, PREMULTIPLIED, src.c, PREMULTIPLIED);
            if (p.c[0] != src.c[0] || p.c[1] != src.c[1] || p.c[2] != src.c[2] || p.c[3] != src.c[3])
            {
                expect("premultiplied to premultiplied", p.c, 4, src.c[0], src.c[1], src.c[2], src.c[3], 0);
                return;
            }
            Pixel straight_src = rgba(c, 255 - c, a, a);
            blend::copy_pixel(p.c, STRAIGHT, straight_src.c, STRAIGHT);
            if (p.c[0] != straight_src.c[0] || p.c[1] != straight_src.c[1] || p.c[2] != straight_src.c[2] ||
                p.c[3] != straight_src.c[3])
            {
                expect("straight to straight", p.c, 4, c, 255 - c, a, a, 0);
                return;
            }
        }
    }
}

static void test_blend_pixel()
{
    Pixel p;
    Pixel half_red = rgba(64, 0, 0, 128); // premultiplied: straight (127.5, 0, 0, 128)

    // Half red over half blue, both premultiplied. Treating them as straight
    // colors, as copy_pixels did before, gave (42, 0, 42, 191).
    p = rgba(0, 0, 128, 128);
    blend::blend_pixel(p.c, PREMULTIPLIED, half_red.c, PREMULTIPLIED);
    expect("premultiplied over premultiplied", p.c, 4, 64, 0, 64, 192, 0);

    uint8_t blue[3] = {0, 0, 255};
    blend::blend_pixel(blue, RGB, half_red.c, PREMULTIPLIED);
    expect("premultiplied over rgb", blue, 3, 64, 0, 127, 0, 1);

    p = rgba(0, 0, 255, 255);
    blend::blend_pixel(p.c, STRAIGHT, rgba(255, 0, 0, 128).c, STRAIGHT);
    expect("straight over opaque straight", p.c, 4, 128, 0, 127, 255, 1);

    // Nothing shows through: the source is stored exactly.
    p = rgba(9, 9, 9, 9);
    blend::blend_pixel(p.c, PREMULTIPLIED, rgba(200, 100, 50, 255).c, STRAIGHT);
    expect("opaque over premultiplied", p.c, 4, 200, 100, 50, 255, 0);
    p = rgba(9, 9, 9, 0);
    blend::blend_pixel(p.c, STRAIGHT, rgba(200, 100, 50, 128).c, STRAIGHT);
    expect("straight over empty straight", p.c, 4, 200, 100, 50, 128, 0);

    // A transparent source leaves every kind of dst unchanged.
    const blend::Format formats[3] = {RGB, STRAIGHT, PREMULTIPLIED};
    for (int i = 0; i < 3; i++)
    {
        p = rgba(10, 20, 30, 40);
        blend::blend_pixel(p.c, formats[i], rgba(255, 255, 255, 0).c, STRAIGHT);
        expect("transparent source", p.c, formats[i].channels, 10, 20, 30, 40, 0);
    }
}

// A premultiplied source blends like the straight source it stands for.
static void test_blend_pixel_source_formats_agree()
{
    const blend::Format dst_formats[2] = {RGB, PREMULTIPLIED};
    for (int f = 0; f < 2; f++)
    {
        for (int da = 0; da <= 255; da += 51)
        {
            for (int sa = 1; sa <= 255; sa += 17)
            {
                for (int sc = 0; sc <= 255; sc += 15)
                {
                    int dc = da / 2;
                    Pixel from_straight = rgba(dc, dc, dc, da);
                    Pixel from_premultiplied = from_straight;
                    int sp = (sc * sa + 127) / 255;
                    blend::blend_pixel(from_straight.c, dst_formats[f], rgba(sc, sc, sc, sa).c, STRAIGHT);
                    blend::blend_pixel(from_premultiplied.c, dst_formats[f], rgba(sp, sp, sp, sa).c, PREMULTIPLIED);
                    if (!near(from_premultiplied.c[0], from_straight.c[0], 1) ||
                        !near(from_premultiplied.c[3], from_straight.c[3], 1))
                    {
                        CHECK(false, "dst format %d (%d, %d) + src (%d, %d): premultiplied source (%d, %d), straight (%d, %d)",
                              f, dc, da, sc, sa, from_premultiplied.c[0], from_premultiplied.c[3], from_straight.c[0],
                              from_straight.c[3]);
                        return;
                    }
                }
            }
        }
    }
}

int main()
{
    test_premultiplied_over_empty();
    test_premultiplied_over_limits();
    test_premultiplied_over_stacks();
    test_premultiplied_stays_valid();
    test_formulas_match_exact_blend();
    test_straight_over();
    test_rgb_over();
    test_copy_pixel();
    test_blend_pixel();
    test_blend_pixel_source_formats_agree();
    return report("blend");
}
