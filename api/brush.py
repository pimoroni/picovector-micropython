"""brush — advanced fill brushes (gradient, image, pattern, effects).

Assign a brush to ``image.pen`` before drawing.  All factories are static and
construct a GC-heap brush (``m_new_class``).
"""

from __future__ import annotations

from typing import Annotated

from pv import api, cpp, const, overload, Range, ColorStops, Pattern8


@api("brush_t", field="brush", ptr=True,
     arg_read="((brush_obj_t *)MP_OBJ_TO_PTR({0}))->brush", arg_type="brush_t *")
class brush:
    """Advanced fill brush factory."""

    LINEAR = const("GRADIENT_LINEAR", "Linear gradient (along the p1→p2 axis).")
    RADIAL = const("GRADIENT_RADIAL", "Radial gradient (outward from p1).")

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

    # image(img, transform=None) --------------------------------------------
    @overload
    def image(img: image) -> brush: "Image brush at the origin."
    @overload
    @cpp(args="img &transform")
    def image(img: image, transform: mat3) -> brush: "Image brush positioned by a mat3."

    @staticmethod
    @cpp(call="image_brush_t", emit="mnew",
         error="invalid parameter, expected brush.image(image, [mat3])")
    def image(img, transform=None) -> brush:
        "Image brush. Args: img (image). Optional: transform (mat3)."

    # gradient(type, x1, y1, x2, y2, stops, transform=None) -----------------
    @staticmethod
    @cpp(call="gradient_brush_t", emit="mnew",
         args="(gradient_type_t)type x1 y1 x2 y2 stops_positions stops_colors stops_n transform")
    def gradient(type: int, x1: float, y1: float, x2: float, y2: float,
                 stops: ColorStops, transform: mat3 | None = None) -> brush:
        "Gradient brush. type: brush.LINEAR or brush.RADIAL; stops: list of "
        "(position 0–1, color), up to 16."

    # erase(color=None) ------------------------------------------------------
    @overload
    def erase() -> brush: "Erase: punch fully-transparent holes (dst-out) with AA edges."
    @overload
    def erase(c: color) -> brush: "Punch a translucent window of colour c with AA edges."

    @staticmethod
    @cpp(call="transparent_brush_t", emit="mnew",
         error="invalid parameter, expected brush.erase([color])")
    def erase(c=None) -> brush:
        "Erase/window brush. No args erases (dst-out); pass a color for a translucent "
        "window that lerps the destination toward that colour by shape coverage."

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
        "Per-pixel film grain, +/- up to `amount`. interval is the refresh period "
        "in ms (0 = static)."

    @staticmethod
    @cpp(call="glitch_brush_t", emit="mnew")
    def glitch(amount: int) -> brush:
        "VHS channel-shift glitch bands; `amount` sets how many bands."

    # ── artwork ────────────────────────────────────────────────────────────
    @staticmethod
    @cpp(call="oilpaint_brush_t", emit="mnew")
    def oilpaint(radius: int, strength: int = 255) -> brush:
        "Oil paint: dominant colour in a `radius` neighbourhood, eased back toward "
        "the original by `strength` (0-255)."

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
