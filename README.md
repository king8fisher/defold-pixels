# defold-pixels

[![Build](https://img.shields.io/github/actions/workflow/status/king8fisher/defold-pixels/bob.yml?branch=master&label=build)](https://github.com/king8fisher/defold-pixels/actions/workflows/bob.yml)
[![Tests](https://img.shields.io/github/actions/workflow/status/king8fisher/defold-pixels/tests.yml?branch=master&label=tests)](https://github.com/king8fisher/defold-pixels/actions/workflows/tests.yml)
[![GitHub repo](https://img.shields.io/badge/github-repo-181717?logo=github&logoColor=white)](https://github.com/king8fisher/defold-pixels)
[![Upstream](https://img.shields.io/badge/upstream-AGulev%2Fdrawpixels-1f6feb)](https://github.com/AGulev/drawpixels)
[![License](https://img.shields.io/github/license/king8fisher/defold-pixels)](LICENSE)

> [!NOTE]
> **This is a fork.** Its aim is to experiment with a TypeScript API workflow for [defold-typescript](https://github.com/defold-typescript/toolchain) projects: typed declarations that ship with a native extension and are imported like a module. defold-pixels is based on [drawpixels](https://github.com/AGulev/drawpixels) by Alexey Gulev, released under the MIT license. The Lua module is named `pixels` instead of `drawpixels`. The fork adds `polyline`, fixes premultiplied-alpha blending, ships TypeScript types, and adds native and engine tests. Report issues with this fork in [its own tracker](https://github.com/king8fisher/defold-pixels/issues), not upstream.

A [Defold](https://defold.com/) native extension that draws pixels and simple geometry into a texture buffer, which you then show on a sprite or a GUI node.

- **Shapes and lines** — circles, rectangles, arcs, lines, gradient lines, and quadratic Bezier curves, with optional anti-aliasing and stroke width.
- **Polylines** — one stroke through many points with bevel or miter joins, a center-to-edge color gradient, and per-point width and alpha, so a single call draws a tapering, fading trail.
- **Correct blending** — each polyline pixel is blended once, so a semi-transparent stroke does not darken at corners or crossings; premultiplied-alpha buffers are supported throughout.
- **Flood fill** — record drawn shapes as a boundary and fill the area inside it.
- **Buffer to buffer copies** — copy a rectangle between buffers, converting between RGB, straight alpha, and premultiplied alpha, with optional alpha blending.
- **Typed for TypeScript** — [`types/pixels.d.ts`](types/pixels.d.ts) declares a `pixels` module for [defold-typescript](https://github.com/defold-typescript/toolchain) projects, imported with `import * as pixels from "pixels"`.

> [!IMPORTANT]
> The buffer must match the size of the atlas texture, not the sprite image. Otherwise you need to know where the sprite sits in the atlas and update that region of the atlas.

## Installation

Add the archive URL to the `[project] dependencies` of your `game.project`, then run **Project ▸ Fetch Libraries** in the Defold editor:

```ini
[project]
dependencies#0 = https://github.com/king8fisher/defold-pixels/archive/refs/heads/master.zip
```

To pin a version, replace `refs/heads/master` with a commit SHA.

### TypeScript

Copy [`types/pixels.d.ts`](types/pixels.d.ts) into a folder that `tsconfig.json` includes, for example:

```sh
curl -o src/pixels.d.ts https://raw.githubusercontent.com/king8fisher/defold-pixels/master/types/pixels.d.ts
```

Then import the module:

```ts
import * as pixels from "pixels";
import type { PixelsBufferInfo } from "pixels";
```

This compiles to `local pixels = require("pixels")`. The extension stores its table in `package.loaded`, so the `require` needs no Lua file. The file is marked `@noResolution`, so TypeScriptToLua does not look for one, and `@noSelfInFile`, so calls compile to `pixels.fill(...)` rather than `pixels:fill(...)`.

The engine also registers `pixels` as a global. To call it without an import, copy [`types/pixels.global.d.ts`](types/pixels.global.d.ts) next to `pixels.d.ts` as well:

```sh
curl -o src/pixels.global.d.ts https://raw.githubusercontent.com/king8fisher/defold-pixels/master/types/pixels.global.d.ts
```

## Usage

Every call takes a buffer info table that describes the target buffer:

```lua
local buffer_info = {
  buffer = buffer.create(1024 * 2048, { -- width * height
    {
      name = hash("rgba"),
      type = buffer.VALUE_TYPE_UINT8,
      count = 4, -- same as channels
    },
  }),
  width = 1024,
  height = 2048,
  channels = 4,
  premultiply_alpha = true,
}

pixels.fill(buffer_info, 0, 0, 0, 255)
pixels.circle(buffer_info, 512, 1024, 300, 250, 220, 80, 255, true, 4)
```

To show the buffer on a sprite, give the sprite its own atlas with a power-of-two texture and call `resource.set_texture`:

```lua
resource.set_texture(go.get("#sprite", "texture0"), self.header, self.buffer_info.buffer)
```

To show it in a GUI, create a texture from the buffer bytes and set it on a box node:

```lua
local data = buffer.get_bytes(self.buffer_info.buffer, hash("rgba"))
gui.new_texture("name", width, height, image.TYPE_RGBA, data)
gui.set_texture(gui.get_node("box"), "name")
gui.set_size(gui.get_node("box"), vmath.vector3(width, height, 0))
```

The full example lives in [`example/canvas.script`](example/canvas.script); smaller ones are in [`examples/`](examples).

## API

The declarations below are copied from the `declare module "pixels"` block in [`types/pixels.d.ts`](types/pixels.d.ts), which follows the argument handling in [`defold-pixels/src/pixels.cpp`](defold-pixels/src/pixels.cpp). From Lua, call the same functions with the same arguments: a tuple such as `PixelsColor` is a plain Lua table (`{r, g, b, a}`), an optional (`?`) argument or field may be left out, and `LuaMultiReturn` means multiple return values.

Colors are 0..255 per channel unless noted.

### Types

```ts
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
```

### Flood fill

```ts
/** Start recording drawn pixels as the boundary for `fill_area`. */
export function start_fill(): void;
/** Stop recording the `fill_area` boundary. */
export function end_fill(): void;
/** Flood-fill the area containing (x, y). */
export function fill_area(buffer_info: PixelsBufferInfo, x: number, y: number, red: number, green: number, blue: number, alpha: number): void;
```

The boundary works with lines, gradient lines, circles, anti-aliased filled circles, pixels, and arcs. It does not work with filled arcs or gradient arcs. Arcs use the same recording internally, so do not draw arcs until you finish the fill, and always call `end_fill` when you are done.

```lua
pixels.start_fill()
pixels.circle(buffer_info, 128, 128, 200, 255, 255, 255, 255)
pixels.fill_area(buffer_info, 128, 128, 40, 90, 200, 255)
pixels.end_fill()
```

### Circles and rectangles

```ts
/** `width` applies only when `antialiasing` is true. */
export function circle(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, diameter: number, red: number, green: number, blue: number, alpha: number, antialiasing?: boolean, width?: number): void;
export function filled_circle(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, diameter: number, red: number, green: number, blue: number, alpha: number, antialiasing?: boolean): void;

export function rect(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, rect_width: number, rect_height: number, red: number, green: number, blue: number, alpha: number): void;
/** `angle` is in degrees. */
export function filled_rect(buffer_info: PixelsBufferInfo, pos_x: number, pos_y: number, rect_width: number, rect_height: number, red: number, green: number, blue: number, alpha: number, angle?: number): void;
/** Fill the whole buffer with one color. */
export function fill(buffer_info: PixelsBufferInfo, red: number, green: number, blue: number, alpha: number): void;
```

### Lines and polylines

```ts
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
```

```lua
pixels.polyline(buffer_info, {24, 40, 96, 216, 160, 64, 232, 200}, {
  width = 16,
  color = {250, 220, 80},
  edge_color = {200, 40, 40},
})

-- A trail that thins out and fades toward its first point.
pixels.polyline(buffer_info, {20, 20, 120, 60, 220, 40}, {
  widths = {2, 8, 16},
  alphas = {0, 0.5, 1},
  color = {250, 220, 80},
})

-- Sharp corners.
pixels.polyline(buffer_info, {40, 40, 120, 200, 200, 40}, {
  width = 12,
  color = {40, 90, 200},
  join = "miter",
})
```

### Arcs

```ts
/** Angles are in radians; `from` < `to` draws counterclockwise. */
export function arc(buffer_info: PixelsBufferInfo, x: number, y: number, radius: number, from: number, to: number, red: number, green: number, blue: number, alpha: number): void;
export function filled_arc(buffer_info: PixelsBufferInfo, x: number, y: number, radius: number, from: number, to: number, red: number, green: number, blue: number, alpha: number): void;
export function gradient_arc(buffer_info: PixelsBufferInfo, x: number, y: number, radius: number, from: number, to: number, red1: number, green1: number, blue1: number, red2: number, green2: number, blue2: number, alpha: number): void;
```

### Pixels and curves

```ts
export function pixel(buffer_info: PixelsBufferInfo, x: number, y: number, red: number, green: number, blue: number, alpha: number): void;
/** Returns r, g, b (and a for 4-channel buffers); nothing outside the buffer. */
export function color(buffer_info: PixelsBufferInfo, x: number, y: number): LuaMultiReturn<[number, number, number, number?]>;

/** Quadratic Bezier curve with control point (xc, yc). */
export function bezier(buffer_info: PixelsBufferInfo, x0: number, y0: number, xc: number, yc: number, x1: number, y1: number, red: number, green: number, blue: number, alpha: number): void;
```

### Copying between buffers

```ts
/** Copy a rectangle from `src` into `dest` at (dest_x, dest_y). */
export function copy_pixels(dest: PixelsBufferInfo, src: PixelsBufferInfo, rect_x: number, rect_y: number, rect_width: number, rect_height: number, dest_x: number, dest_y: number, merge_alpha?: boolean): void;
```

## Tests

Native tests cover the code that has no Defold dependencies: the polyline drawing and the pixel blend formulas. They need only a C++11 compiler:

```sh
tests/native/run.sh
```

Engine tests call the Lua API in a headless engine and read the pixels back. They build the engine with bob and the Defold build server, so they need network access. On macOS the script uses the bob inside `/Applications/Defold.app`; elsewhere, set `BOB` to the path of `bob.jar`:

```sh
BOB=/path/to/bob.jar tests/api/run.sh
```

To run the engine tests in the editor, set `bootstrap.main_collection` to `/tests/api/api.collection`. The results are printed to the console.

Both suites run in CI ([`.github/workflows/tests.yml`](.github/workflows/tests.yml)).

## Keeping the docs in sync

When the Lua API changes, update [`defold-pixels/api/pixels.script_api`](defold-pixels/api/pixels.script_api), [`types/pixels.d.ts`](types/pixels.d.ts) (and [`types/pixels.global.d.ts`](types/pixels.global.d.ts) when a type is added), and the [API](#api) section of this README together. The README copies the declarations and JSDoc from `types/pixels.d.ts` as they are.

## Credits

Based on [drawpixels](https://github.com/AGulev/drawpixels) by Alexey Gulev, MIT License, Copyright (c) 2018 Alexey Gulev. Fork changes are MIT License, Copyright (c) 2026 king8fisher. See [LICENSE](LICENSE).
