# Defold Pixels

[![Build Status](https://github.com/king8fisher/defold-pixels/actions/workflows/bob.yml/badge.svg)](https://github.com/king8fisher/defold-pixels/actions/workflows/bob.yml)

![15f0e5a685b5c7f16ef78f612c377e4cab08ce8a](https://github.com/user-attachments/assets/4c9d1ead-74e2-4675-ab51-a9ddf63c15da)

[Defold engine](http://www.defold.com) native extension for drawing pixels and simple geometry into texture buffer.

Based on [drawpixels](https://github.com/AGulev/drawpixels) by Alexey Gulev, released under the MIT license. The Lua module is named `pixels` instead of `drawpixels`.

Feel free to contribute!

>Please, pay attension!
The size of the image you manipulate with Defold Pixels must match that of the atlas, not the sprite image. Otherwise you need to know where the sprite is the atlas and update that region of the atlas.

## Installation
To use this library in your Defold project, add the needed version URL to your `game.project` dependencies from [Releases](https://github.com/king8fisher/defold-pixels/releases)

<img width="401" alt="image" src="https://user-images.githubusercontent.com/2209596/202223571-c77f0304-5202-4314-869d-7a90bbeec5ec.png">


## Example
![screenshot](https://user-images.githubusercontent.com/2209596/45321616-7f24e100-b54e-11e8-9821-00b1b2e36a97.jpg)

Main code example is [here](example/canvas.script)

## Lua API
First of all you need to create a table with a buffer information that contain next fields:

`buffer` - buffer<br />
`width` - buffer width, same as your texture width<br />
`height` - buffer height, same as your texture height<br />
`channels` - 3 for rgb, 4 for rgba<br />
`premultiply_alpha` - alpha value will be premultiplied into the RGB color values. Optional parameter. Default is false.<br />

For example:
```lua
local buffer_info = {
    buffer = buffer.create(1024 * 2048, -- size of the buffer width*height
    {{
      name = hash("my_buffer"),
      type = buffer.VALUE_TYPE_UINT8,
      count = 4 -- same as channels
    }}),
    width = 1024,
    height = 2048,
    channels = 4,
    premultiply_alpha = true
  }
```
Then when you have a buffer info, you can use next methods:

#### pixels.circle(buffer_info, pox_x, pox_y, diameter, red, green, blue, alpha, antialiasing, width)
Method for drawing circle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the circle<br />
`pox_y` - y position center of the circle<br />
`diameter` - diameter of the circle<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />
`width` - indicates the circle width. Only for circle with anti-aliasing. Optional parameter.<br />

#### pixels.filled_circle(buffer_info, pos_x, pos_y, diameter, red, green, blue, alpha, antialiasing)
Method for drawing filled circle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the circle<br />
`pox_y` - y position center of the circle<br />
`diameter` - diameter of the circle<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />

#### pixels.rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha)
Method for drawing rectangle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the rectangle<br />
`pox_y` - y position center of the rectangle<br />
`rect_width` - rectangle width<br />
`rect_height` - rectangle height<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### pixels.filled_rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha, angle)
Method for drawing filled rectangle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the rectangle<br />
`pox_y` - y position center of the rectangle<br />
`rect_width` - rectangle width<br />
`rect_height` - rectangle height<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`angle` - rotation angle in degrees. Optional.<br />

#### pixels.fill(buffer_info, red, green, blue, alpha)
Fill buffer with the color:

`buffer_info` - buffer information<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### pixels.line(buffer_info, x0, y0, x1, y1, red, green, blue, alpha, antialiasing, width)
Draw a line between two points:

`buffer_info` - buffer information<br />
`x0` - x position of one end of the line<br />
`y0` - y position of one end of the line<br />
`x1` - x position of the other end of the line<br />
`y1` - y position of the other end of the line<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />
`width` - indicates the line width. Only for line with anti-aliasing. Optional parameter.<br />

#### pixels.gradient_line(buffer_info, x0, y0, x1, y1, red1, green1, blue1, red2, green2, blue2, alpha, antialiasing, width)
Draw a gradient line between two points:

`buffer_info` - buffer information<br />
`x0` - x position of one end of the line<br />
`y0` - y position of one end of the line<br />
`x1` - x position of the other end of the line<br />
`y1` - y position of the other end of the line<br />
`red1` - first red channel of the color 0..255<br />
`green1` - first green channel of the color 0..255<br />
`blue1` - first blue channel of the color 0..255<br />
`red2` - second red channel of the color 0..255<br />
`green2` - second green channel of the color 0..255<br />
`blue2` - second blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />
`width` - indicates the line width. Only for line with anti-aliasing. Optional parameter.<br />

#### pixels.polyline(buffer_info, points, options)
Draw a polyline with bevel joins and an optional color gradient from the center line to the edges. Width and alpha can be set per point, so one stroke can taper and fade out like a trail. The whole polyline is drawn as one shape: each pixel is blended once, so a semi-transparent stroke does not darken at its corners or where it crosses itself.

`buffer_info` - buffer information<br />
`points` - flat list of coordinates `{x1, y1, x2, y2, ...}`. Coordinates address pixel centers and may be fractional. Fewer than two distinct points draw nothing.<br />
`options` - table with the fields below<br />
`options.width` - stroke width in pixels, measured across the line. Optional when `widths` is given.<br />
`options.widths` - `{w1, w2, ...}`, one stroke width per point. Optional; replaces `width`. The width changes linearly along each segment.<br />
`options.alphas` - `{a1, a2, ...}`, one alpha factor in 0..1 per point. Optional. It scales the alpha of both colors and changes linearly along each segment.<br />
`options.color` - `{r, g, b, a}` on the center line, 0..255. Alpha is optional and defaults to 255.<br />
`options.edge_color` - `{r, g, b, a}` at both edges. Optional; defaults to `color` for a solid stroke.<br />
`options.join` - corner style. Optional; only `"bevel"` is supported.<br />
`options.clip` - `{x, y, w, h}`. Optional; only pixels inside this rectangle change. Use it to redraw part of a canvas.<br />

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
```

#### pixels.arc(buffer_info, x, y, radius, from, to, red, green, blue, alpha)
Draw a arc between two corners. If from < to the arc is drawn counterclockwise. If from > to the arc is drawn clockwise. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position center of the circle<br />
`y` - y position center of the circle<br />
`radius` - radius of the circle<br />
`from` - first arc angle in radians. May be negative<br />
`to` - second arc angle in radians. May be negative<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

#### pixels.filled_arc(buffer_info, x, y, radius, from, to, red, green, blue, alpha)
Draw a filled arc between two corners. If from < to the arc is drawn counterclockwise. If from > to the arc is drawn clockwise. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position center of the circle<br />
`y` - y position center of the circle<br />
`radius` - radius of the circle<br />
`from` - first arc angle in radians. May be negative<br />
`to` - second arc angle in radians. May be negative<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

#### pixels.gradient_arc(buffer_info, x, y, radius, from, to, red1, green1, blue1, red2, green2, blue2, alpha)
Draw a gradient arc between two corners. If from < to the arc is drawn counterclockwise. If from > to the arc is drawn clockwise. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position center of the circle<br />
`y` - y position center of the circle<br />
`radius` - radius of the circle<br />
`from` - first arc angle in radians. May be negative<br />
`to` - second arc angle in radians. May be negative<br />
`red1` - first red channel of the color 0..255<br />
`green1` - first green channel of the color 0..255<br />
`blue1` - first blue channel of the color 0..255<br />
`red2` - second red channel of the color 0..255<br />
`green2` - second green channel of the color 0..255<br />
`blue2` - second blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

#### pixels.pixel(buffer_info, x, y, red, green, blue, alpha)
Draw a pixel:

`buffer_info` - buffer information<br />
`x` - x position of pixel<br />
`y` - y position of pixel<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### pixels.color(buffer_info, x, y)
Read color from a position in the buffer:

`buffer_info` - buffer information<br />
`x` - x position to get color from<br />
`y` - y position to get color from<br />

RETURNS:

`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### pixels.bezier(buffer_info, x0, y0, xc, yc, x1, y1, red, green, blue, alpha)
Draw a bezier line between two points and one control point:

`buffer_info` - buffer information<br />
`x0` - x position of the first point<br />
`y0` - y position of the first point<br />
`xc` - x position of the control point<br />
`yc` - y position of the control point<br />
`x1` - x position of the second point<br />
`y1` - y position of the second point<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

You can fill in a specific area. To do this, you need to identify the borders with `start_fill` method and call the `fill_area` method to fill the area. Borders work with: lines, gradient lines, circles, filled circles with anti-aliasing, pixels, arcs. <b>NOT WORK WITH FILLED ARCS AND GRADIENT ARCS.</b> The arcs themselves use this method, so the fill may not be predictable. Do not create arcs until you are done with the fill. Be sure to call the `end_fill` method when you stop working with the fill.

#### pixels.start_fill()
Indicates the start of border preservation.

#### pixels.end_fill()
Indicates the stop of border preservation.

#### pixels.fill_area(buffer_info, x, y, red, green, blue, alpha)
Fills an area at specified boundaries. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position of the start point<br />
`y` - y position of the start point<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

In order to draw this into a sprite, you need a separate atlas with power of two texture. Then we can use `resource.set_texture`:

```lua
resource.set_texture(go.get("#sprite", "texture0"), self.header, self.buffer_info.buffer)
```

In order to render this in gui, we just need to create a box and create a new texture.
We can use `gui.new_texture`. Then you need to set this texture to the box using `gui.set_texture`:

```lua
local data = buffer.get_bytes(self.buffer_info.buffer, hash("rgba"))
gui.new_texture("name", width, height, image.TYPE_RGBA, data)
gui.set_texture(gui.get_node("box"), "name")
gui.set_size(gui.get_node("box"), vmath.vector3(width, height, 0))
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

Both suites run in CI (`.github/workflows/tests.yml`).

--------

If you have any questions or suggestions contact me: me@agulev.com
