"""image — an RGBA pixel buffer: the screen, off-screen canvases and sub-views.

The bulk of the drawing surface is generated; the members that build nested
Python results, dispatch by name, decode files or track a parent view
(``load``/``load_into``/``window``/``text``/``measure_text``/``shapes``/``batch``)
are inherently procedural and live in ``native/image.cpp`` — the DSL still owns
their signatures, docstrings and registration.
"""

from __future__ import annotations

from pv import (api, cpp, native, overload, const,
                XY, XYWH, Filter, Buffer, ShapeOrList, NEAREST)


@api("image_t", field="image", ptr=True, buffer=True,
     print=("image(%d x %d)", "int(self->image->bounds().w)", "int(self->image->bounds().h)"),
     arg_read="((image_obj_t *)MP_OBJ_TO_PTR({0}))->image", arg_type="image_t *")
class image:
    """An RGBA pixel buffer you can draw into and blit around."""

    # antialias / fill-rule / texture-filter constants
    X4 = const("antialias_t::X4", "Anti-aliasing: 4× supersample.")
    X2 = const("antialias_t::X2", "Anti-aliasing: 2× supersample.")
    OFF = const("antialias_t::OFF", "Anti-aliasing disabled.")
    EVEN_ODD = const("fill_rule_t::EVEN_ODD", "Fill rule: even-odd winding.")
    NON_ZERO = const("fill_rule_t::NON_ZERO", "Fill rule: non-zero winding.")
    NEAREST = const("filter_t::NEAREST", "Texture filter: nearest-neighbour (fastest).")
    BILINEAR = const("filter_t::BILINEAR", "Texture filter: bilinear (smooth).")
    BICUBIC = const("filter_t::BICUBIC", "Texture filter: bicubic (highest quality).")

    @cpp(emit="image")
    def __init__(self, width: int, height: int, buffer: Buffer = None):
        "image(width, height) allocates a buffer; pass an existing buffer to wrap it."

    # ── properties ──────────────────────────────────────────────────────────
    @property
    @cpp(get="self->image->bounds().w")
    def width(self) -> int: "Width in pixels (read-only)."

    @property
    @cpp(get="self->image->bounds().h")
    def height(self) -> int: "Height in pixels (read-only)."

    @property
    @cpp(get_raw="mp_obj_new_bytearray_by_ref(self->image->buffer_size(), self->image->ptr(0, 0))")
    def raw(self) -> None: "The framebuffer as a bytearray view (read-only, by reference)."

    @property
    @cpp(get="self->image->clip()", set="self->image->clip({0})")
    def clip(self) -> rect: "Clip rect; assign to restrict drawing."

    @property
    @cpp(get="self->image->has_palette()")
    def has_palette(self) -> bool: "True if this image is palettised (read-only)."

    @property
    @cpp(get="self->image->antialias()", set="self->image->antialias((antialias_t){0})")
    def antialias(self) -> int: "Anti-aliasing mode: OFF, X2 or X4."

    @property
    @cpp(get="self->image->fill_rule()", set="self->image->fill_rule((fill_rule_t){0})")
    def fill_rule(self) -> int: "Winding fill rule: EVEN_ODD or NON_ZERO."

    @property
    @cpp(get="self->image->alpha()", set="self->image->alpha({0})")
    def alpha(self) -> int: "Global layer alpha 0–255."

    @property
    @cpp(emit="pen")
    def pen(self) -> brush | None: "Current fill brush/colour (assign a brush or color)."

    @property
    @cpp(emit="font")
    def font(self) -> vector_font | pixel_font | None:
        "Current font. Assign a vector_font or pixel_font (typically from font.load())."

    @property
    @cpp(get="self->image->text_cursor()", set="self->image->text_cursor({0})")
    def cursor(self) -> vec2:
        "Text caret as a vec2. text() leaves it at the start of the next line "
        "(each call ends with an implicit newline). Set it to position the next "
        "text() (which may omit its x, y); newlines return to the x it was set to."

    # ── construction / IO (procedural → native) ─────────────────────────────
    @staticmethod
    @native
    def load(path_or_bytes, width: int = 0, height: int = 0) -> image:
        "Load a PNG/JPEG from a path or buffer. Returns an image."

    @native
    def load_into(self, path_or_bytes) -> None:
        "Load a PNG/JPEG into this buffer in place."

    @native
    def window(self, area: XYWH) -> image:
        "Return a sub-image view of this canvas (shares pixels)."

    @native
    def spritesheet(self, cols: int, rows: int) -> image:
        "Configure this image as a cols x rows spritesheet grid for sprite(). "
        "Returns self, so it chains off load()/image()."

    @native
    def sprite(self, x: int, y: int) -> image:
        "Return the spritesheet cell at grid (x, y) as a sub-image view (shares "
        "pixels). x is the column, y the row; cell size is the sheet divided by "
        "its cols x rows layout."

    # ── raster primitives ───────────────────────────────────────────────────
    def clear(self) -> None: "Fill the whole image with the current pen."

    @cpp(args="r")
    def rectangle(self, r: XYWH) -> None: "Filled rectangle (rect, or x, y, w, h)."

    @cpp(args="x y w")
    def hspan(self, x: int, y: int, w: int) -> None: "Horizontal 1px-tall run in the current pen; skips the span buffer (fast path)."

    @cpp(args="x y h")
    def vspan(self, x: int, y: int, h: int) -> None: "Vertical 1px-wide run in the current pen; skips the span buffer (fast path)."

    @cpp(args="a b")
    def line(self, a: XY, b: XY) -> None: "Line between two points."

    @cpp(args="c (int)r")
    def circle(self, c: XY, r: float) -> None: "Filled circle at c with radius r."

    @cpp(args="a b c")
    def triangle(self, a: XY, b: XY, c: XY) -> None: "Filled triangle."

    def blur(self, radius: float) -> None: "Box-blur the whole image in place."
    def bloom(self, threshold: int = 180, intensity: int = 150, radius: float = 4.0) -> None: "Bloom: soft halo around pixels brighter than `threshold`, added back scaled by `intensity`."
    def dither(self) -> None: "Dither the whole image in place."
    def onebit(self) -> None: "Convert to 1-bit black/white in place."
    def monochrome(self) -> None: "Convert to monochrome in place."
    def invert(self) -> None: "Photonegative the whole image in place."
    def threshold(self, level: int, lo: color, hi: color) -> None: "Two-level luminance threshold to colours lo/hi."
    def saturation(self, amount: int) -> None: "Adjust saturation (amount>0 boosts, -256 = greyscale)."
    def contrast(self, amount: int) -> None: "Adjust contrast around mid-grey (amount>0 more)."
    def duotone(self, shadow: color, highlight: color) -> None: "Map luminance onto a shadow→highlight ramp."
    def crt(self, spacing: int, darkness: int) -> None: "CRT tube: darken every `spacing`-th row by `darkness`, with a rounded corner falloff."
    def grid(self, spacing: int, darkness: int) -> None: "Gentle pixel grid: darken every `spacing`-th row and column by `darkness`."
    def vignette(self, strength: int) -> None: "Darken by distance from the image centre."
    def gameboy(self) -> None: "Map the whole image to the 4 Game Boy greens."
    def noise(self, amount: int, interval: int = 0) -> None: "Add per-pixel film grain (+/- amount). interval = refresh period in ms (0 = static)."
    def glitch(self, amount: int) -> None: "VHS channel-shift glitch bands."
    def oilpaint(self, radius: int, strength: int = 255) -> None: "Oil paint: dominant colour in a radius, eased back toward the original by strength (0-255)."
    def cga(self) -> None: "CGA 4-colour palette."

    @cpp(args="palette palette_n strength")
    def palette_dither(self, palette: list, strength: int = 64) -> None:
        "Map to a restricted palette (list of colours). strength is the dither "
        "amount: 0 clamps every pixel to its solid nearest colour, ~64 is subtle, "
        "128 medium, 255 heavy."
    def phosphor(self, tint: color) -> None: "CRT phosphor glow toward tint."
    def synthwave(self) -> None: "Synthwave: neon cyan/magenta/white palette dither with a bloom glow."
    def c64(self) -> None: "Commodore 64 16-colour palette."
    def nightvision(self) -> None: "Green amplify + grain + edge darkening."
    def chromatic(self, offset: int) -> None: "Chromatic aberration RGB split."

    @cpp(args="p.x p.y", box="pv::box_color_packed({0})")
    def get(self, p: XY) -> color: "Read the pixel colour at p."

    @cpp(args="p.x p.y")
    def put(self, p: XY) -> None: "Set the pixel at p to the current pen."

    def shape(self, s: ShapeOrList) -> None: "Draw a shape, or a list of shapes."

    @native
    def shapes(self, entries) -> None:
        "Batched shape draw: each entry is a shape or (shape, brush/color)."

    # ── text (font branching → native) ──────────────────────────────────────
    @native
    def text(self, text: str, at: XY = None, size: float = 0) -> None:
        "Draw text with the current font, advancing an internal cursor. Each "
        "call ends with an implicit newline, so a following text() with no "
        "position starts on the next line (print-style). at (a vec2 or x, y) is "
        "optional: omit it to continue at the cursor; a '\\n' in the text also "
        "starts a new line at the x it was last positioned at. size (sentinel 0 "
        "= the font's default): point size for vector fonts (default 12), or the "
        "integer nearest-neighbour scale for pixel fonts (default 1; 2 = double "
        "size, 3 = triple, ...)."

    @native
    def measure_text(self, text: str, size: float = 0) -> tuple[float, float]:
        "Measure text in the current font. Returns (width, height). "
        "size (sentinel 0 = the font's default): point size for vector fonts "
        "(default 12), or the integer scale for pixel fonts (default 1) — pass "
        "the same value you draw with so layout matches."

    # ── blitting ─────────────────────────────────────────────────────────────
    @overload
    @cpp(args="self->image at")
    def blit(self, src: image, at: vec2) -> None: "Blit src at a point."
    @overload
    @cpp(args="self->image vec2_t(x,y)")
    def blit(self, src: image, x: float, y: float) -> None: "Blit src at (x, y)."
    @overload
    @cpp(args="self->image source dst filter")
    def blit(self, src: image, source: rect, dst: rect, filter: Filter = NEAREST) -> None:
        "Blit a source rect of src into a destination rect."
    @overload
    @cpp(args="self->image dst filter")
    def blit(self, src: image, dst: rect, filter: Filter = NEAREST) -> None:
        "Blit all of src into a destination rect."

    @cpp(recv="src", error="invalid parameter, expected blit(image, point), "
         "blit(image, rect) or blit(image, source_rect, dest_rect)")
    def blit(self, src, *args) -> None:
        "Blit another image onto this one (point, rect, or src-rect → dst-rect)."

    @cpp(recv="src", args="self->image p len uv0 uv1 filter")
    def blit_vspan(self, src: image, p: XY, len: float, uv0: XY, uv1: XY,
                   filter: Filter = NEAREST) -> None:
        "Blit a vertical texture span (advanced; used by raycasters)."

    @cpp(recv="src", args="self->image p len uv0 uv1 filter")
    def blit_hspan(self, src: image, p: XY, len: float, uv0: XY, uv1: XY,
                   filter: Filter = NEAREST) -> None:
        "Blit a horizontal texture span (advanced)."

    @native
    def batch(self, commands) -> None:
        "Run a list of (method, *args) draw commands in one C loop."
