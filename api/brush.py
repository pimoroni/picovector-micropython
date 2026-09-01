"""brush — advanced fill brushes (gradient, image, pattern, effects).

Assign a brush to ``image.pen`` before drawing.  All factories are static and
construct a GC-heap brush (``m_new_class``).
"""

from __future__ import annotations

from typing import Annotated

from pv import (api, cpp, const, overload, Range, ColorStops, Pattern8, SourceImage,
                XY, Filter, NEAREST)


@api("brush_t", field="brush", ptr=True,
     arg_read="((brush_obj_t *)MP_OBJ_TO_PTR({0}))->brush", arg_type="brush_t *")
class brush:
    """Advanced fill brush factory."""

    LINEAR = const("GRADIENT_LINEAR", "Linear gradient (along the p1→p2 axis).")
    RADIAL = const("GRADIENT_RADIAL", "Radial gradient (outward from p1).")
    CONICAL = const("GRADIENT_CONICAL",
                    "Conical gradient (sweeping around p1, from the p1→p2 direction).")

    # pattern(c1, c2, index | tuple[8]) -------------------------------------
    @overload
    def pattern(c1: color, c2: color, index: Annotated[int, Range(0, 37)]) -> brush:
        "Pre-baked checkerboard pattern by index (0–37)."
    @overload
    def pattern(c1: color, c2: color, pattern: Pattern8) -> brush:
        "Custom pattern from an 8-element row-bitmask tuple."

    @staticmethod
    @cpp(call="pattern_brush_t", emit="mnew",
         error="invalid parameter, expected brush.pattern(color, color, index | tuple[8])")
    def pattern(c1, c2, p) -> brush:
        "Checkerboard pattern brush. Args: c1, c2 (colors), index (0–37) or 8-tuple."

    # image(img, transform=None, filter=NEAREST) -----------------------------
    # the mat3 overload is declared first: dispatch guards only cover required
    # params, so the shorter one would otherwise swallow brush.image(img, mat3).
    @overload
    @cpp(args="img &transform filter")
    def image(img: SourceImage, transform: mat3, filter: Filter = NEAREST) -> brush:
        "Image brush positioned by a mat3."
    @overload
    def image(img: SourceImage, filter: Filter = NEAREST) -> brush:
        "Image brush at the origin."

    @staticmethod
    @cpp(call="image_brush_t", emit="mnew",
         error="invalid parameter, expected brush.image(image, [mat3], [filter])")
    def image(img, transform=None, filter=NEAREST) -> brush:
        ("Image brush. Args: img (image). Optional: transform (mat3), filter "
         "(image.NEAREST, image.BILINEAR or image.BICUBIC). A filtered brush "
         "samples 4 texels per pixel (bilinear) or 16 (bicubic).")

    # gradient(type, x1, y1, x2, y2, stops, transform=None) -----------------
    @staticmethod
    @cpp(call="gradient_brush_t", emit="mnew",
         args="type x1 y1 x2 y2 stops_positions stops_colors stops_n transform")
    def gradient(type: int, x1: float, y1: float, x2: float, y2: float,
                 stops: ColorStops, transform: mat3 | None = None) -> brush:
        ("Gradient brush. stops: list of (position 0–1, color), up to 16. "
         "type: brush.LINEAR, where p1 and p2 are the ends of the axis; "
         "brush.RADIAL, where p1 is the centre and |p2-p1| the radius; or "
         "brush.CONICAL, where p1 is the centre and the p1→p2 direction is where "
         "the ramp starts. A conical's stop positions are fractions of a full "
         "turn, clockwise, so a 270° gauge puts its stops in 0–0.75.")

    # geometry(x1, y1, x2, y2, transform=None) -------------------------------
    @cpp(call="pv::brush_geometry", emit="free",
         args="self->brush x1 y1 x2 y2 transform")
    def geometry(self, x1: float, y1: float, x2: float, y2: float,
                 transform: mat3 | None = None) -> None:
        ("Move a gradient brush without rebuilding it. The stops are unchanged, "
         "so an animated gradient can be built once and repositioned each frame. "
         "Raises TypeError on any other kind of brush.")

    # fractal(scale, octaves, persistence, transform) ------------------------
    @staticmethod
    @cpp(call="fractal_brush_t", emit="mnew")
    def fractal(scale: float = 64.0,
                octaves: Annotated[int, Range(1, 4, clamp=True)] = 3,
                persistence: float = 0.4, repeat: int = 0, seed: int = 0,
                transform: mat3 | None = None) -> brush:
        ("Fractal (fBm) value noise brush - clouds, smoke, fire, terrain, marbling. "
         "scale: device pixels per cell of the coarsest pass, which sets feature "
         "size independently of the area filled. octaves: 1-4 passes, each double "
         "the frequency of the last. persistence: how much each pass contributes "
         "relative to the one below it, 0.05 smooth masses to 0.95 wispy detail. "
         "repeat: tile period in cells, rounded down to a power of two, so the "
         "field repeats every repeat * scale pixels and a translation of that much "
         "is seamless; the maximum, and the default, is 256 >> (octaves - 1). "
         "Starts black-to-white; call ramp() to colour it. Optional: transform "
         "(mat3).")

    # ramp(stops) ------------------------------------------------------------
    @cpp(call="pv::brush_ramp", emit="free",
         args="self->brush stops_positions stops_colors stops_n")
    def ramp(self, stops: ColorStops) -> None:
        ("Recolour a fractal brush. stops: list of (position 0-1, color), up to 16. "
         "Positions are area fractions of the field, so a stop at 0.6 sits where "
         "60% of the field is below it. Two stops sharing a "
         "position are a hard edge, spaced stops a soft one; transparent stops lay "
         "the field over existing content. Raises TypeError on any other kind of "
         "brush.")

    # scale -------------------------------------------------------------------
    @property
    @cpp(get="pv::brush_cell(self->brush)", set="pv::brush_resize(self->brush, {0})")
    def scale(self) -> float:
        ("A fractal brush's cell size in device pixels - its feature size. Composed "
         "inside transform, so changing one leaves the other alone. Raises "
         "TypeError on any other kind of brush.")

    # seed --------------------------------------------------------------------
    @property
    @cpp(get="pv::brush_seed(self->brush)")
    def seed(self) -> int:
        ("The seed this fractal brush's field was built from. Raises TypeError on "
         "any other kind of brush.")

    # repeat ------------------------------------------------------------------
    @property
    @cpp(get="pv::brush_repeat(self->brush)")
    def repeat(self) -> int:
        ("A fractal brush's tile period in cells, after rounding down to a power of "
         "two and capping at 256 >> (octaves - 1). Multiply by scale for the "
         "seamless translation distance in pixels. Raises TypeError on any other "
         "kind of brush.")

    # transform ---------------------------------------------------------------
    @property
    @cpp(get="pv::brush_placement(self->brush)", set="pv::brush_place(self->brush, {0})")
    def transform(self) -> mat3:
        ("A fractal brush's placement, applied outside its scale: an identity "
         "transform gives cells of scale pixels, and one that only rotates or "
         "translates leaves feature size alone. Translation is in device pixels. "
         "The ramp is unchanged, so this is what an animated fill assigns each "
         "frame. Raises TypeError on any other kind of brush.")

    # erase(color=None) ------------------------------------------------------
    @overload
    def erase() -> brush: "Erase: punch fully-transparent holes (dst-out) with AA edges."
    @overload
    def erase(c: color) -> brush: "Punch a translucent window of colour c with AA edges."

    @staticmethod
    @cpp(call="transparent_brush_t", emit="mnew",
         error="invalid parameter, expected brush.erase([color])")
    def erase(c=None) -> brush:
        ("Erase/window brush. No args erases (dst-out); pass a color for a translucent "
         "window that lerps the destination toward that colour by shape coverage.")

    # single-arg effect brushes ---------------------------------------------
    @staticmethod
    @cpp(call="pixelate_brush_t", emit="mnew")
    def pixelate(size: Annotated[int, Range(1, None, clamp=True)]) -> brush:
        "Mosaic the shape's area, block size in pixels."

    @staticmethod
    @cpp(call="blur_brush_t", emit="mnew")
    def blur(radius: Annotated[int, Range(1, None, clamp=True)]) -> brush:
        "Box-blur the shape's area from the target."

    @staticmethod
    @cpp(call="brightness_brush_t", emit="mnew")
    def lighten(amount: int) -> brush:
        "Add amount (0–255) to each channel of the backdrop."

    @staticmethod
    @cpp(call="brightness_brush_t", emit="mnew", args="-amount")
    def darken(amount: int) -> brush:
        "Subtract amount (0–255) from each channel of the backdrop."

    @staticmethod
    @cpp(call="monochrome_brush_t", emit="mnew")
    def monochrome() -> brush:
        "Greyscale the shape's area (per-pixel green-biased luminance)."

    @staticmethod
    @cpp(call="dither_brush_t", emit="mnew")
    def dither() -> brush:
        "Ordered-dither the shape's area to a 4-level palette (screen-aligned)."

    @staticmethod
    @cpp(call="palette_dither_brush_t", emit="mnew", args="palette palette_n strength")
    def palette_dither(palette: list, strength: int = 64) -> brush:
        ("Ordered-dither the shape's area to a restricted palette (list of colours). "
         "strength is the dither amount (0 = solid nearest, ~64 subtle, 255 heavy).")

    @staticmethod
    @cpp(call="invert_brush_t", emit="mnew")
    def invert() -> brush:
        "Photonegative the shape's area."

    @staticmethod
    @cpp(call="threshold_brush_t", emit="mnew")
    def threshold(level: int, lo: color, hi: color) -> brush:
        "Two-level threshold on luminance: <= level -> colour lo, else hi."

    @staticmethod
    @cpp(call="saturation_brush_t", emit="mnew")
    def saturation(amount: int) -> brush:
        "Saturation: amount>0 boosts, <0 desaturates (-256 = greyscale)."

    @staticmethod
    @cpp(call="contrast_brush_t", emit="mnew")
    def contrast(amount: int) -> brush:
        "Contrast around mid-grey: amount>0 more, <0 less."

    @staticmethod
    @cpp(call="duotone_brush_t", emit="mnew")
    def duotone(shadow: color, highlight: color) -> brush:
        "Map luminance onto a shadow->highlight two-colour ramp (e.g. sepia)."

    @staticmethod
    @cpp(call="crt_brush_t", emit="mnew")
    def crt(spacing: int, darkness: int) -> brush:
        "CRT tube: darken every `spacing`-th row by `darkness` (0-255) with a rounded corner falloff."

    @staticmethod
    @cpp(call="grid_brush_t", emit="mnew")
    def grid(spacing: int, darkness: int) -> brush:
        "Gentle pixel grid: darken every `spacing`-th row and column by `darkness` (0-255)."

    @staticmethod
    @cpp(call="vignette_brush_t", emit="mnew")
    def vignette(strength: int) -> brush:
        "Darken by distance from the centre (strength 0-255)."

    # ── playful / retro ────────────────────────────────────────────────────

    @staticmethod
    @cpp(call="noise_brush_t", emit="mnew")
    def noise(amount: int, interval: int = 0) -> brush:
        ("Per-pixel film grain, +/- up to `amount`. interval is the refresh period "
         "in ms (0 = static).")

    @staticmethod
    @cpp(call="glitch_brush_t", emit="mnew")
    def glitch(amount: int) -> brush:
        "VHS channel-shift glitch bands; `amount` sets how many bands."

    # ── artwork ────────────────────────────────────────────────────────────
    @staticmethod
    @cpp(call="oilpaint_brush_t", emit="mnew")
    def oilpaint(radius: int, strength: int = 255) -> brush:
        ("Oil paint: dominant colour in a `radius` neighbourhood, eased back toward "
         "the original by `strength` (0-255).")

    # ── retro ──────────────────────────────────────────────────────────────
    @staticmethod
    @cpp(call="phosphor_brush_t", emit="mnew")
    def phosphor(tint: color) -> brush:
        "CRT phosphor glow toward `tint` (e.g. green or amber)."

    # ── futuristic ─────────────────────────────────────────────────────────
    @staticmethod
    @cpp(call="nightvision_brush_t", emit="mnew")
    def nightvision() -> brush:
        "Night vision: green amplify + grain + edge darkening."

    @staticmethod
    @cpp(call="chromatic_brush_t", emit="mnew")
    def chromatic(offset: int) -> brush:
        "Chromatic aberration: shift R left / B right by `offset` px."
