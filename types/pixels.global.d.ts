// Optional global form of the types in pixels.d.ts, for code that calls the
// extension through the `pixels` global the engine also registers.
// Copy it next to pixels.d.ts, which it depends on; no import is needed.

declare const pixels: typeof import("pixels");
type PixelsBufferInfo = import("pixels").PixelsBufferInfo;
type PixelsColor = import("pixels").PixelsColor;
type PixelsPolylineOptions = import("pixels").PixelsPolylineOptions;
