"""indexed_image — a palettised pixel buffer: a blit source, never a target.

Nothing in the library can rasterise into an 8bpp buffer. Every brush and every
filter stores a four-byte pixel, so drawing into one would run four times past
the end of each row; the core refuses, but a type that advertises ``shape()``,
``pen`` and twenty-odd filters it cannot honour is still lying about what it is.

So this is ``image``'s read-only half. It wraps the same ``image_t`` and shares
its obj struct (see ``indexed_image_obj_t`` in ``runtime/pv_objs.hpp``), and
exposes only what an indexed buffer can answer: its size, its colour table, its
spritesheet grid and timings. Everything else is simply absent, so
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
    @cpp(get_raw="mp_obj_new_bytearray_by_ref(self->image->buffer_size(), self->image->ptr(0, 0))")
    def raw(self) -> None:
        ("The pixels as a bytearray view (read-only, by reference). One byte per "
         "pixel here, each a palette index, so this is a quarter the size of the "
         "same picture as an image.")

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
    # The one thing that *is* writable: a single entry recolours every pixel
    # indexing it, which is how a sheet gets team colours without a second copy
    # of the pixels. A sprite() view shares the table, so this reaches through.
    @overload
    @cpp(args="index")
    def palette(self, index: Annotated[int, Range(0, 255)]) -> int:
        "The premultiplied colour at a palette index (compare color.p)."
    @overload
    @cpp(args="index c._p")
    def palette(self, index: Annotated[int, Range(0, 255)], c: color) -> None:
        "Set a palette entry, recolouring every pixel that indexes it."

    @cpp(error="invalid parameter, expected palette(index) or palette(index, color)")
    def palette(self, index, *args) -> None:
        "Read or set an entry in the colour table."

    # ── spritesheet grid and timing ─────────────────────────────────────────
    @property
    @cpp(get="self->image->cols()")
    def cols(self) -> int:
        ("Spritesheet columns (read-only), 1 unless spritesheet() set a grid or "
         "a GIF was loaded. The frame count of an animation.")

    @property
    @cpp(get="self->image->rows()")
    def rows(self) -> int:
        "Spritesheet rows (read-only), 1 unless a grid was set."

    @property
    @cpp(get_raw="self->frame_delays == MP_OBJ_NULL ? mp_const_none : self->frame_delays")
    def delays(self) -> None:
        ("Per-frame durations in ms for a sheet loaded from a GIF, else None "
         "(read-only). One entry per spritesheet column.")

    @property
    @cpp(get_raw="mp_obj_new_int(pv::image_total_delay(self))")
    def duration(self) -> int:
        ("How long one loop of the animation lasts in ms (read-only), or 0 if "
         "this image carries no frame timings.")

    @cpp(call="pv::image_frame_at", emit="free", args="self ms")
    def frame_at(self, ms: int) -> int:
        ("The frame covering ms into the animation, honouring the file's own "
         "per-frame delays. Wraps, so a free-running clock can be handed "
         "straight over: sheet.sprite(sheet.frame_at(badge.ticks), 0). Raises "
         "if the image has no timings; check duration first.")

    # ── views (procedural → native) ─────────────────────────────────────────
    # A view of an indexed image is indexed too: it shares both the pixels and
    # the colour table.
    @native
    def window(self, area: XYWH) -> indexed_image:
        "Return a sub-image view of this buffer (shares pixels and palette)."

    @native
    def spritesheet(self, cols: int, rows: int) -> indexed_image:
        ("Configure this image as a cols x rows spritesheet grid for sprite(). "
         "Returns self, so it chains off load().")

    @native
    def sprite(self, x: int, y: int) -> indexed_image:
        ("Return the spritesheet cell at grid (x, y) as a sub-image view (shares "
         "pixels and palette). x is the column, y the row.")

    # ── reading a pixel ─────────────────────────────────────────────────────
    @cpp(args="p.x p.y", box="pv::box_color_packed({0})")
    def get(self, p: XY) -> color:
        "Read the pixel colour at p, resolved through the palette."
