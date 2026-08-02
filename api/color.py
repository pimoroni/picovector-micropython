"""color — an RGBA / HSV / OKLCH colour with a built-in palette."""

from __future__ import annotations

from pv import api, cpp, Palette


# Dawnbringer-16 based palette (+ Badger e-ink greys). black/white differ on
# Tufty. Order matches the original color.cpp locals dict.
PALETTE = [
    Palette("black", (0x00, 0x00, 0x00, 0xff), tufty=(0x14, 0x1e, 0x28, 0xff), doc="Black."),
    Palette("grape", (0x44, 0x24, 0x34, 0xff), doc="Grape / purple."),
    Palette("navy", (0x30, 0x34, 0x6d, 0xff), doc="Navy blue."),
    Palette("grey", (0x4e, 0x4a, 0x4e, 0xff), doc="Grey."),
    Palette("brown", (0x85, 0x4c, 0x30, 0xff), doc="Brown."),
    Palette("green", (0x34, 0x65, 0x24, 0xff), doc="Green."),
    Palette("red", (0xd0, 0x46, 0x48, 0xff), doc="Red."),
    Palette("taupe", (0x75, 0x71, 0x61, 0xff), doc="Taupe."),
    Palette("blue", (0x59, 0x7d, 0xce, 0xff), doc="Blue."),
    Palette("orange", (0xd2, 0x7d, 0x2c, 0xff), doc="Orange."),
    Palette("smoke", (0x85, 0x95, 0xa1, 0xff), doc="Smoke (pale grey-blue)."),
    Palette("lime", (0x6d, 0xaa, 0x2c, 0xff), doc="Lime green."),
    Palette("latte", (0xd2, 0xaa, 0x99, 0xff), doc="Latte (warm beige)."),
    Palette("cyan", (0x6d, 0xc2, 0xca, 0xff), doc="Cyan."),
    Palette("yellow", (0xda, 0xd4, 0x5e, 0xff), doc="Yellow."),
    Palette("white", (0xff, 0xff, 0xff, 0xff), tufty=(0xde, 0xee, 0xd6, 0xff), doc="White."),
    Palette("transparent", (0x00, 0x00, 0x00, 0x00), doc="Fully transparent (alpha 0)."),
    Palette("light_grey", (0xc0, 0xc0, 0xc0, 0xff), doc="Light grey (Badger e-ink)."),
    Palette("dark_grey", (0x40, 0x40, 0x40, 0xff), doc="Dark grey (Badger e-ink)."),
]


@api("color_t", field="c", box="pv::box_color({0})",
     arg_read="(((color_obj_t *)MP_OBJ_TO_PTR({0}))->c)", arg_type="color_t",
     includes=("blend.hpp",), palette=PALETTE,
     # Reads back as the call that would rebuild it: the space name is also the
     # constructor name, and the three components are whatever it was authored
     # with. See color_space_qstr in pv_bindings.hpp.
     print=("color.%q(%d, %d, %d, %d)", "pv::color_space_qstr(self->c)",
            "self->c.component(0)", "self->c.component(1)", "self->c.component(2)",
            "self->c.a()"))
class color:
    """An RGBA / HSV / OKLCH colour with a built-in palette.

    A colour is immutable: it remembers the components and space it was authored
    in, reports them back through read-only properties, and every operation
    returns a new colour. That is what makes the palette entries (color.red and
    friends, which live in read-only storage) safe to hand out and reuse.

    Colours compare and hash by the colour they render as, not by identity, so
    two equal colours are one dict key.
    """

    @staticmethod
    @cpp(call="rgb_color_t", emit="free")
    def rgb(r: int, g: int, b: int, a: int = 255) -> color:
        "Create a colour from RGB values (0-255 each). Optional alpha (0-255)."

    @staticmethod
    @cpp(call="hsv_color_t", emit="free", args="(h&0xff) s v a")
    def hsv(h: int, s: int, v: int, a: int = 255) -> color:
        ("Create a colour from HSV components (0-255 each; hue wraps). "
         "Optional alpha (0-255).")

    @staticmethod
    @cpp(call="oklch_color_t", emit="free")
    def oklch(l: int, c: int, h: int, a: int = 255) -> color:
        ("Create a colour from OKLCH components (0-255 each, not CSS units: l "
         "spans 0-1 lightness, c spans 0-0.35 chroma, and h is 256 counts to a "
         "full turn, so 250 is 352 degrees). Optional alpha (0-255).")

    # ── resolved sRGB: available whatever the authoring space ────────────────
    @property
    @cpp(get="self->c.r()")
    def r(self) -> int:
        "Red channel, 0-255 (read-only)."

    @property
    @cpp(get="self->c.g()")
    def g(self) -> int:
        "Green channel, 0-255 (read-only)."

    @property
    @cpp(get="self->c.b()")
    def b(self) -> int:
        "Blue channel, 0-255 (read-only)."

    @property
    @cpp(get="self->c.a()")
    def a(self) -> int:
        "Alpha, 0-255 (read-only)."

    # ── authored components: only in the space they belong to ────────────────
    @property
    @cpp(get="pv::color_require_hue(self->c).h()")
    def h(self) -> int:
        "Hue as authored, 0-255. HSV and OKLCH colours only (read-only)."

    @property
    @cpp(get="pv::color_require(self->c, COLOR_HSV).s()")
    def s(self) -> int:
        "Saturation as authored, 0-255. HSV colours only (read-only)."

    @property
    @cpp(get="pv::color_require(self->c, COLOR_HSV).v()")
    def v(self) -> int:
        "Value as authored, 0-255. HSV colours only (read-only)."

    @property
    @cpp(get="pv::color_require(self->c, COLOR_OKLCH).l()")
    def l(self) -> int:
        "Lightness as authored, 0-255. OKLCH colours only (read-only)."

    @property
    @cpp(get="pv::color_require(self->c, COLOR_OKLCH).c()")
    def c(self) -> int:
        "Chroma as authored, 0-255. OKLCH colours only (read-only)."

    @property
    @cpp(get_raw="MP_OBJ_NEW_QSTR(pv::color_space_qstr(self->c))")
    def space(self) -> str:
        "How the colour was authored: 'rgb', 'hsv' or 'oklch' (read-only)."

    @property
    @cpp(get="self->c._p")
    def p(self) -> int:
        "Premultiplied packed RGBA word (read-only)."

    # ── arithmetic (a new colour; the original is unchanged) ─────────────────
    def lighten(self, amount: int) -> color:
        ("Return this colour lightened by amount (0-255). Moves v on an HSV "
         "colour and l on an OKLCH one; on an RGB colour it moves all three "
         "channels. Clamps.")

    def darken(self, amount: int) -> color:
        "Return this colour darkened by amount (0-255). The inverse of lighten."

    def scale(self, percent: int) -> color:
        ("Return this colour with its lightness scaled by percent (100 is "
         "unchanged). Acts on the same component lighten does. Alpha is "
         "untouched.")

    def with_alpha(self, a: int) -> color:
        "Return this colour at a different alpha (0-255)."

    def mix(self, other: color, t: int) -> color:
        ("Return this colour blended toward other; t 0 is this colour, 255 is "
         "other. Interpolates the authored components when both share a space, "
         "taking a hue the short way round; otherwise interpolates sRGB.")

    def over(self, background: color) -> color:
        ("Return this colour as it lands over background, weighted by its own "
         "alpha. The same composite the renderer performs.")

    # ── one authored component replaced, in the authoring space ──────────────
    @cpp(call="pv::color_require(self->c, COLOR_RGB).with_component", emit="free",
         args="0 value")
    def with_r(self, value: int) -> color:
        "Return this colour with a different red channel. RGB colours only."

    @cpp(call="pv::color_require(self->c, COLOR_RGB).with_component", emit="free",
         args="1 value")
    def with_g(self, value: int) -> color:
        "Return this colour with a different green channel. RGB colours only."

    @cpp(call="pv::color_require(self->c, COLOR_RGB).with_component", emit="free",
         args="2 value")
    def with_b(self, value: int) -> color:
        "Return this colour with a different blue channel. RGB colours only."

    @cpp(call="pv::color_with_hue", emit="free", args="self->c value")
    def with_h(self, value: int) -> color:
        "Return this colour at a different hue. HSV and OKLCH colours only."

    @cpp(call="pv::color_require(self->c, COLOR_HSV).with_component", emit="free",
         args="1 value")
    def with_s(self, value: int) -> color:
        "Return this colour at a different saturation. HSV colours only."

    @cpp(call="pv::color_require(self->c, COLOR_HSV).with_component", emit="free",
         args="2 value")
    def with_v(self, value: int) -> color:
        "Return this colour at a different value. HSV colours only."

    @cpp(call="pv::color_require(self->c, COLOR_OKLCH).with_component", emit="free",
         args="0 value")
    def with_l(self, value: int) -> color:
        "Return this colour at a different lightness. OKLCH colours only."

    @cpp(call="pv::color_require(self->c, COLOR_OKLCH).with_component", emit="free",
         args="1 value")
    def with_c(self, value: int) -> color:
        "Return this colour at a different chroma. OKLCH colours only."

    # ── operators ───────────────────────────────────────────────────────────
    # No inplace variants: colours are immutable, and mp_binary_op falls back to
    # the plain operator (py/runtime.c), so `c += 10` rebinds to a new colour.
    @cpp(result="lhs->c.lighten((int)v)")
    def __add__(self, other: int) -> color: "Lighten by an amount, as lighten()."

    @cpp(result="lhs->c.darken((int)v)")
    def __sub__(self, other: int) -> color: "Darken by an amount, as darken()."

    @cpp(result="lhs->c.scale((int)(v * 100.0f + 0.5f))")
    def __mul__(self, other: float) -> color: "Scale lightness by a factor, as scale()."

    def __eq__(self, other: color) -> bool:
        "True when both render as the same colour, whatever space each was authored in."

    def __ne__(self, other: color) -> bool: "Inequality."

    # Folded so it fits a small int; agrees with __eq__ because both look only at
    # the premultiplied word.
    @cpp(result="(self->c._p ^ (self->c._p >> 30)) & 0x3fffffff")
    def __hash__(self) -> int:
        "Hash of the rendered colour, so equal colours are one dict key."
