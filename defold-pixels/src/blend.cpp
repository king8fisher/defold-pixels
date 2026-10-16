#include "blend.h"

namespace blend
{
    static void record(uint8_t *dst, float v)
    {
        *dst = v > 1 ? 255 : v * 255.0;
    }

    static float mix_rgb(float dc, float da, float sc, float sa, float oa)
    {
        if (oa == 0)
        {
            return 0.0;
        }
        return (sc * sa + dc * da * (1 - sa)) / oa;
    }

    void straight_over(uint8_t *dst, float r, float g, float b, float a)
    {
        float ba = dst[3] / 255.0;
        a = a / 255.0;
        float oa = a + ba * (1 - a);
        r = mix_rgb(dst[0] / 255.0, ba, r / 255.0, a, oa);
        g = mix_rgb(dst[1] / 255.0, ba, g / 255.0, a, oa);
        b = mix_rgb(dst[2] / 255.0, ba, b / 255.0, a, oa);
        record(dst, r);
        record(dst + 1, g);
        record(dst + 2, b);
        record(dst + 3, oa);
    }

    void premultiplied_over(uint8_t *dst, float r, float g, float b, float a)
    {
        float k = a / 255.0;
        dst[0] = r * k + dst[0] * (1 - k) + 0.5;
        dst[1] = g * k + dst[1] * (1 - k) + 0.5;
        dst[2] = b * k + dst[2] * (1 - k) + 0.5;
        dst[3] = a + dst[3] * (1 - k) + 0.5;
    }

    void rgb_over(uint8_t *dst, float r, float g, float b, float a)
    {
        float k = a / 255.0f;
        dst[0] = (uint8_t)(r * k + dst[0] * (1 - k) + 0.5f);
        dst[1] = (uint8_t)(g * k + dst[1] * (1 - k) + 0.5f);
        dst[2] = (uint8_t)(b * k + dst[2] * (1 - k) + 0.5f);
    }

    static uint8_t to_byte(float v)
    {
        if (v <= 0)
        {
            return 0;
        }
        return v >= 255 ? 255 : (uint8_t)(v + 0.5f);
    }

    static bool is_premultiplied(const Format &format)
    {
        return format.channels == 4 && format.premultiplied;
    }

    // Reads a pixel as a straight color and its alpha. The color is kept as a
    // float so a premultiplied color survives the round trip unchanged.
    static void read_straight(const uint8_t *src, const Format &format, float &r, float &g, float &b, float &a)
    {
        r = src[0];
        g = src[1];
        b = src[2];
        // No alpha in the source: fully opaque.
        a = format.channels == 4 ? src[3] : 255;
        if (is_premultiplied(format) && a < 255)
        {
            float k = a > 0 ? 255.0f / a : 0;
            r *= k;
            g *= k;
            b *= k;
        }
    }

    void copy_pixel(uint8_t *dst, const Format &dst_format, const uint8_t *src, const Format &src_format)
    {
        float r, g, b, a;
        read_straight(src, src_format, r, g, b, a);
        if (is_premultiplied(dst_format))
        {
            float k = a / 255.0f;
            r *= k;
            g *= k;
            b *= k;
        }
        dst[0] = to_byte(r);
        dst[1] = to_byte(g);
        dst[2] = to_byte(b);
        // An RGB dst has no alpha channel, so the alpha is dropped.
        if (dst_format.channels == 4)
        {
            dst[3] = to_byte(a);
        }
    }

    void blend_pixel(uint8_t *dst, const Format &dst_format, const uint8_t *src, const Format &src_format)
    {
        float r, g, b, a;
        read_straight(src, src_format, r, g, b, a);
        if (a <= 0)
        {
            return; // fully transparent source pixel, skip
        }
        if (dst_format.channels == 3)
        {
            // dst has no alpha: blend onto an opaque background.
            rgb_over(dst, r, g, b, a);
        }
        else if (dst_format.premultiplied)
        {
            premultiplied_over(dst, r, g, b, a);
        }
        else if (a >= 255 || dst[3] == 0)
        {
            // Opaque source or empty straight dst: nothing shows through, so
            // store the source exactly.
            copy_pixel(dst, dst_format, src, src_format);
        }
        else
        {
            straight_over(dst, r, g, b, a);
        }
    }
}
