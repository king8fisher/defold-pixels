#ifndef DEFOLD_PIXELS_BLEND_H
#define DEFOLD_PIXELS_BLEND_H

// Source-over blending of one color into one pixel.
// It has no Defold dependencies so it can be compiled and tested on its own
// (see tests/native/blend_test.cpp).
//
// The source color r, g, b is always straight (not multiplied by alpha).
// All channel values are in the 0..255 range.

#include <stdint.h>

namespace blend
{
    // dst is an RGBA pixel that stores straight colors.
    void straight_over(uint8_t *dst, float r, float g, float b, float a);

    // dst is an RGBA pixel that stores premultiplied colors, which is what
    // Defold's alpha blend mode expects.
    void premultiplied_over(uint8_t *dst, float r, float g, float b, float a);

    // dst is an RGB pixel; it has no alpha and counts as opaque.
    void rgb_over(uint8_t *dst, float r, float g, float b, float a);

    // How a buffer stores its pixels. `premultiplied` only matters with 4 channels;
    // an RGB pixel has no alpha and counts as opaque.
    struct Format
    {
        int channels;
        bool premultiplied;
    };

    // Replaces dst with the src pixel, converted to dst's format. An RGB dst
    // keeps the straight color and drops the alpha.
    void copy_pixel(uint8_t *dst, const Format &dst_format, const uint8_t *src, const Format &src_format);

    // Blends the src pixel over dst (source-over), whatever the two formats are.
    void blend_pixel(uint8_t *dst, const Format &dst_format, const uint8_t *src, const Format &src_format);
}

#endif
