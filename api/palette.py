"""palette — an indexed image's colour table, as a Python sequence.

``image.palette`` hands one back, or ``None`` when the image is not palettised,
so ``if img.palette:`` reads as well as ``img.has_palette``. It behaves as you
would expect a sequence of colours to::

    len(img.palette)                 # entries in the table
    img.palette[3]                   # -> color
    img.palette[3] = color.red       # one entry
    img.palette[0:4] = ramp          # several
    list(img.palette)                # the lot, copied
    memoryview(img.palette)          # the raw bytes, not copied

A palette attached to an image *is* that image's table, not a copy, and sub-views
share it by pointer - so writing one entry recolours every sprite cut from the
same sheet, which is most of the reason to hold an indexed image at all.

``palette(colors)`` builds a free-standing one, to assign from::

    img.palette = palette(color.ramp(stops, 16))

That **copies the entries in** rather than swapping the table. It has to: a view
holds its own copy of the table pointer, so a swap would leave every existing
sprite showing the old colours while the image showed the new ones.
"""

from __future__ import annotations

from pv import api, cpp, native


@api("uint32_t", field="entries", ptr=True, subscr=True,
     # memoryview(palette) is the table itself, four bytes an entry.
     buffer=("palette_entries(self)", "self->count * sizeof(uint32_t)"),
     print=("palette(%d colours)", "self->count"))
class palette:
    """A colour table: a mutable sequence of colours, indexed by a byte."""

    @cpp(emit="native")
    def __init__(self, colors):
        ("palette(colors) builds a free-standing table from a sequence of "
         "colours (1-256), to assign to an image's palette. Assigning copies the "
         "entries in, so every view of that image follows.")

    # ── sequence protocol ───────────────────────────────────────────────────
    # len() and the subscript arrive as slots rather than methods; __getitem__ is
    # a method as well because that is what MicroPython's iteration falls back to
    # (mp_getiter looks the name up, not the slot), and it is what makes
    # list(palette) and `for c in palette` work.
    @cpp(result="self->count")
    def __len__(self) -> int: "Entries in the table."

    @native
    def __getitem__(self, index):
        ("The colour at an index, or a list of colours for a slice. Also what "
         "iteration uses, so list(palette) copies the whole table out.")

    # ── the raw bytes ───────────────────────────────────────────────────────
    @property
    @cpp(get_raw="mp_obj_new_bytearray_by_ref(self->count * sizeof(uint32_t), palette_entries(self))")
    def raw(self) -> None:
        ("The table as a bytearray view (read-only property, writable bytes, by "
         "reference). Four bytes an entry, premultiplied, in the same order "
         "color.p packs them. Equivalent to memoryview(palette).")

    @property
    @cpp(get_raw="self->source ? MP_OBJ_FROM_PTR(self->source) : mp_const_none")
    def image(self) -> None:
        ("The image whose table this is, or None for a free-standing palette "
         "(read-only). A palette keeps its image alive.")
