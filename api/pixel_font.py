"""pixel_font — a bitmap font loaded from a .ppf file.

Instances are produced by ``font.load()`` (which sniffs the file and returns a
``vector_font`` or a ``pixel_font``); there is no ``pixel_font.load``. Parsing
the .ppf container is procedural file/heap work in
``native/pixel_font_native.cpp``; the attributes and registration are declarative.
"""

from __future__ import annotations

from pv import api, cpp


@api("pixel_font_t", field="font", ptr=True,
     print=('pixel_font(\\"%s\\")', "self->path"),
     arg_read="((pixel_font_obj_t *)MP_OBJ_TO_PTR({0}))->font", arg_type="pixel_font_t *")
class pixel_font:
    """A bitmap font loaded from a .ppf file.

    Pixel fonts render at integer sizes only. Assign one with ``screen.font =``
    then pass an integer scale as the ``size`` argument to :meth:`image.text` /
    :meth:`image.measure_text` — 1 (default) is native size, 2 doubles every
    glyph pixel to a 2x2 block (nearest-neighbour), and so on. Layout (advance,
    spacing and reported height) scales to match, so ``measure_text`` with the
    same scale gives the box you'll actually draw. For fractional/point sizing
    use a vector_font instead.
    """

    @property
    @cpp(get="self->font->height")
    def height(self) -> int:
        "Glyph cell height in pixels (read-only)."

    @property
    @cpp(get_raw="mp_obj_new_str(self->font->name, strlen(self->font->name))")
    def name(self) -> str:
        "Font name (read-only)."
