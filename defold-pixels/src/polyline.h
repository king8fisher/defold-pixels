#ifndef DEFOLD_PIXELS_POLYLINE_H
#define DEFOLD_PIXELS_POLYLINE_H

// Polyline rasterizer with bevel or miter joins and a color gradient across the stroke.
// It has no Defold dependencies so it can be compiled and tested on its own
// (see tests/native/polyline_test.cpp).
//
// The whole polyline is treated as one shape: every part (segment, join) is
// first combined into a mask, then each covered pixel is emitted exactly once.
// Semi-transparent strokes therefore never darken where parts overlap.

namespace polyline
{
    // Channel values in the 0..255 range.
    struct Rgba
    {
        float r;
        float g;
        float b;
        float a;
    };

    // Pixel rectangle; x/y is the top-left pixel in buffer coordinates.
    struct Rect
    {
        int x;
        int y;
        int w;
        int h;
    };

    enum JoinStyle
    {
        // The corner is cut flat between the two outer edges.
        JOIN_BEVEL,
        // The outer edges are extended until they meet, up to `miter_limit`.
        JOIN_MITER,
    };

    struct Style
    {
        float width;
        // Color along the center line of the stroke.
        Rgba color;
        // Color at both edges of the stroke. Equal to `color` for a solid stroke.
        Rgba edge_color;
        // Optional stroke width at each input point, replacing `width`. The width
        // changes linearly along each segment; gentle tapers stay accurate.
        const float *widths = nullptr;
        // Optional alpha factor in 0..1 at each input point. It scales both colors'
        // alpha and changes linearly along each segment.
        const float *alphas = nullptr;
        // Corner style at interior points.
        JoinStyle join = JOIN_BEVEL;
        // Longest miter, as a multiple of the half width at the corner. Longer
        // miters are cut flat at this distance from the point (miter-clip).
        // Values below 1 count as 1.
        float miter_limit = 4;
    };

    // Receives each covered pixel once. `a` already includes edge coverage.
    typedef void (*PlotFn)(void *ctx, int x, int y, float r, float g, float b, float a);

    // Draws a polyline through `count` points stored as x0, y0, x1, y1, ...
    // Point coordinates address pixel centers. Only pixels inside `bounds` are
    // emitted. Fewer than two distinct points or a non-positive width draw nothing.
    // `style.widths` and `style.alphas`, when set, hold `count` values each.
    void draw(const float *xy, int count, const Style &style, const Rect &bounds, PlotFn plot, void *ctx);
}

#endif
