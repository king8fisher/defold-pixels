// Ambient TypeScript types for the defold-pixels native extension, for projects
// written with defold-typescript (https://github.com/defold-typescript/toolchain).
// Copy this file into a folder that tsconfig.json includes, then import the module:
//   import * as pixels from "pixels";
// The extension registers its table in package.loaded, so require("pixels") needs
// no Lua file; @noResolution keeps TypeScriptToLua from looking for one, and
// @noSelfInFile makes calls use `.` instead of passing a `self` argument.
// Signatures follow the C++ argument handling in defold-pixels/src/pixels.cpp.
// Keep this file, defold-pixels/api/pixels.script_api, and the API section of
// README.md in sync with the API.

/** @noSelfInFile */
/** @noResolution */
declare module "pixels" {
  /** Target buffer passed to every drawing call. */
  export interface PixelsBufferInfo {
    buffer: ReturnType<typeof buffer.create>;
    /** Buffer width in pixels; must match the texture width. */
    width: number;
    /** Buffer height in pixels; must match the texture height. */
    height: number;
    /** 3 for RGB, 4 for RGBA. */
    channels: 3 | 4;
    /** Blend using premultiplied alpha. Defaults to false. */
    premultiply_alpha?: boolean;
  }

  /** [r, g, b, a] in 0..255; alpha defaults to 255. */
  export type PixelsColor = [number, number, number] | [number, number, number, number];

  export interface PixelsPolylineOptions {
    /** Stroke width in pixels, measured across the line. Optional when `widths` is given. */
    width?: number;
    /** One stroke width per point, replacing `width`; changes linearly along each segment. */
    widths?: number[];
    /** One alpha factor in 0..1 per point; scales both colors' alpha, linearly along each segment. */
    alphas?: number[];
    /** Color on the center line. */
    color: PixelsColor;
    /** Color at both edges. Defaults to `color` for a solid stroke. */
    edge_color?: PixelsColor;
    /** Corner style: "bevel" cuts corners flat, "miter" extends the outer edges until they meet. Defaults to "bevel". */
    join?: "bevel" | "miter";
    /** Longest miter as a multiple of the half width, at least 1; longer miters are cut flat. Defaults to 4. */
    miter_limit?: number;
    /** [x, y, w, h]: only pixels inside this rectangle change. */
    clip?: [number, number, number, number];
  }

  /** Start recording drawn pixels as the boundary for `fill_area`. */
  export function start_fill(): void;
  /** Stop recording the `fill_area` boundary. */
  export function end_fill(): void;
  /** Flood-fill the area containing (x, y). */
  export function fill_area(buffer_info: PixelsBufferInfo, x: number, y: number, red: number, green: number, blue: number, alpha: number): void;

  /** `width` applies only when `antialiasing` is true. */
  export function circle(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, diameter: number, red: number, green: number, blue: number, alpha: number, antialiasing?: boolean, width?: number): void;
  export function filled_circle(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, diameter: number, red: number, green: number, blue: number, alpha: number, antialiasing?: boolean): void;

  export function rect(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, rect_width: number, rect_height: number, red: number, green: number, blue: number, alpha: number): void;
  /** `angle` is in degrees. */
  export function filled_rect(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, rect_width: number, rect_height: number, red: number, green: number, blue: number, alpha: number, angle?: number): void;
  /** Fill the whole buffer with one color. */
  export function fill(buffer_info: PixelsBufferInfo, red: number, green: number, blue: number, alpha: number): void;

  /** `width` applies only when `antialiasing` is true. */
  export function line(buffer_info: PixelsBufferInfo, x0: number, y0: number, x1: number, y1: number, red: number, green: number, blue: number, alpha: number, antialiasing?: boolean, width?: number): void;
  /** Blends from color 1 at (x0, y0) to color 2 at (x1, y1). */
  export function gradient_line(buffer_info: PixelsBufferInfo, x0: number, y0: number, x1: number, y1: number, red1: number, green1: number, blue1: number, red2: number, green2: number, blue2: number, alpha: number, antialiasing?: boolean, width?: number): void;
  /**
   * Draws `points` ([x1, y1, x2, y2, ...]) as one stroke with bevel or miter
   * joins and a gradient from the center line to the edges. Each pixel is
   * blended once, so a semi-transparent stroke does not darken at corners or
   * crossings.
   */
  export function polyline(buffer_info: PixelsBufferInfo, points: number[], options: PixelsPolylineOptions): void;

  /** Angles are in radians; `from` < `to` draws counterclockwise. */
  export function arc(buffer_info: PixelsBufferInfo, x: number, y: number, radius: number, from: number, to: number, red: number, green: number, blue: number, alpha: number): void;
  export function filled_arc(buffer_info: PixelsBufferInfo, x: number, y: number, radius: number, from: number, to: number, red: number, green: number, blue: number, alpha: number): void;
  export function gradient_arc(buffer_info: PixelsBufferInfo, x: number, y: number, radius: number, from: number, to: number, red1: number, green1: number, blue1: number, red2: number, green2: number, blue2: number, alpha: number): void;

  export function pixel(buffer_info: PixelsBufferInfo, x: number, y: number, red: number, green: number, blue: number, alpha: number): void;
  /** Returns r, g, b (and a for 4-channel buffers); nothing outside the buffer. */
  export function color(buffer_info: PixelsBufferInfo, x: number, y: number): LuaMultiReturn<[number, number, number, number?]>;

  /** Quadratic Bezier curve with control point (xc, yc). */
  export function bezier(buffer_info: PixelsBufferInfo, x0: number, y0: number, xc: number, yc: number, x1: number, y1: number, red: number, green: number, blue: number, alpha: number): void;

  /** Copy a rectangle from `src` into `dest` at (dest_x, dest_y). */
  export function copy_pixels(dest: PixelsBufferInfo, src: PixelsBufferInfo, rect_x: number, rect_y: number, rect_width: number, rect_height: number, dest_x: number, dest_y: number, merge_alpha?: boolean): void;
}
