"""indexed_image — a palettised pixel buffer: a blit source, never a target.

Nothing in the library can rasterise into an 8bpp buffer. Every brush and every
filter stores a four-byte pixel, so drawing into one would run four times past
the end of each row; the core refuses, but a type that advertises ``shape()``,
``pen`` and twenty-odd filters it cannot honour is still lying about what it is.

So this is ``image``'s read-only half. It wraps the same ``image_t`` and shares
its obj struct (see ``indexed_image_obj_t`` in ``runtime/pv_objs.hpp``), and
exposes only what an indexed buffer can answer: its size, its colour table and
the views cut out of it. Everything else is simply absent, so
``gif.oilpaint()`` is an ``AttributeError`` naming the thing that is wrong.

Read-only here means *not a render target*. The colour table stays writable —
recolouring a sheet by writing its palette is most of the reason to hold one.
"""

from __future__ import annotations

from pv import api, cpp, native, overload, XY, XYWH


@api("image_t", field="image", ptr=True, buffer=True,
     print=("indexed_image(%d x %d, %d colours)",
            "int(self->image->bounds().w)", "int(self->image->bounds().h)",
            "int(self->image->palette_size())"))
class indexed_image:
    """A palettised pixel buffer: one byte per pixel, indexing a colour table.

    You do not construct one. ``image.load()`` returns an indexed_image for a
    GIF or an indexed PNG, and ``window()``/``sprite()`` of one are indexed too.
    """

    # ── properties ──────────────────────────────────────────────────────────
    @property
    @cpp(get="self->image->bounds().w")
    def width(self) -> int: "Width in pixels (read-only)."

    @property
    @cpp(get="self->image->bounds().h")
    def height(self) -> int: "Height in pixels (read-only)."

    @property
    @cpp(get_raw="mp_obj_new_bytearray_by_ref(self->image->buffer_extent(), self->image->ptr(0, 0))")
    def raw(self) -> None:
        ("The pixels as a bytearray view (read-only, by reference). One byte per "
         "pixel here, each a palette index, so this is a quarter the size of the "
         "same picture as an image.")

    @property
    @cpp(get="self->image->row_stride()")
    def stride(self) -> int:
        ("Bytes from the start of one row of raw to the next (read-only). One byte "
         "a pixel here, and a view inherits its parent's stride, so index raw as "
         "y * stride + x, never y * width.")

    @property
    @cpp(get="self->image->alpha()", set="self->image->alpha({0})")
    def alpha(self) -> int:
        ("Layer alpha 0-255. Read from the *source* side of a blit, so set it "
         "before blitting this image, not on the target.")

    @property
    @cpp(get="self->image->has_palette()")
    def has_palette(self) -> bool: "Always True (read-only). See image.has_palette."

    @property
    @cpp(get="self->image->palette_size()")
    def palette_size(self) -> int: "Entries in the colour table (read-only)."

    # ── the colour table ────────────────────────────────────────────────────
    # A sequence of colours, and the one thing on an indexed image that *is*
    # writable: one entry recolours every pixel indexing it. A view shares the
    # table by pointer, so it reaches every sprite cut from the same sheet.
    @property
    @cpp(get_raw="palette_box(self)", set="palette_assign(self, {0})")
    def palette(self) -> None:
        ("The colour table as a mutable sequence, or None when this image is not "
         "palettised - so `if img.palette:` reads as well as has_palette. Index "
         "it, slice it, len() it, list() it or take a memoryview of its bytes. "
         "Assign a palette or a sequence of colours to overwrite the table; that "
         "copies the entries in, so every existing view follows.")

    # ── views (procedural → native) ─────────────────────────────────────────
    # A view of an indexed image is indexed too: it shares both the pixels and
    # the colour table. So is a cell of a sheet cut from it.
    @native
    def window(self, area: XYWH) -> indexed_image:
        "Return a sub-image view of this buffer (shares pixels and palette)."

    @native
    def spritesheet(self, cols: int = 0, rows: int = 0) -> spritesheet:
        ("A cols x rows grid over this image, as a spritesheet - which is what "
         "addresses individual cells. Omit the grid to use the one the image "
         "already carries: a loaded GIF has its frames there.")

    # ── reading a pixel ─────────────────────────────────────────────────────
    @cpp(args="p.x p.y", box="pv::box_color_packed({0})")
    def get(self, p: XY) -> color:
        "Read the pixel colour at p, resolved through the palette."
