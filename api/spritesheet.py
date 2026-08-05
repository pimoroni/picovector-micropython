"""spritesheet — a grid over an image.

``spritesheet.load(path, cols, rows)`` builds one, as does
``image.spritesheet(cols, rows)`` from an image you already have. A sheet owns no
pixels; it describes how to carve someone else's buffer into cells.

The grid numbers its cells ``0..cols*rows-1``, and that number is what a frame
number already is::

    sheet = spritesheet.load("chicken.png", 7, 2)   # cells 0..13
    sheet.sprite(9)          # a cell, by number
    sheet.sprite(2, 1)       # ...or by coordinate. The same cell

**A sheet keeps no clock.** Playing a sequence is a ``tween`` over cell numbers -
the tween carries the range, the duration, ``start``, ``done`` and the easing
curves, and the sheet resolves the cell::

    dying = tween(7, 12, duration=500)   # end exclusive, so the last frame
    dying.start()                        # gets its full share of the time
    if dying.done:
        self.done_dying = True
    else:
        screen.blit(sheet.sprite(int(dying.now)), pos)

A loop needs no start time at all::

    screen.blit(sheet.sprite(badge.ticks // 100 % sheet.frames), pos)
"""

from __future__ import annotations

from pv import api, cpp, native, const


@api("spritesheet_t", field="sheet",
     print=("spritesheet(%d x %d, %d cells)",
            "self->sheet.cols()", "self->sheet.rows()", "self->sheet.frames()"))
class spritesheet:
    """A grid over an image, numbering its cells so they can be addressed."""

    # How the grid numbers its cells, which is what sprite(n) reads.
    ROWS = const("SHEET_ROWS", "Number the cells along each row in turn (default).")
    COLUMNS = const("SHEET_COLUMNS", "Number the cells down each column in turn.")

    @staticmethod
    @native
    def load(path_or_bytes, cols: int = 0, rows: int = 0) -> spritesheet:
        ("Load a PNG, JPEG or GIF and return a sheet over it, which is what "
         "image.load(...).spritesheet(...) spells the long way. Omit the grid for "
         "a GIF: it already declared its frames. The image keeps whatever format "
         "the file was in, so an indexed one stays a byte a pixel and its cells "
         "come back as indexed_image.")

    # ── the grid ────────────────────────────────────────────────────────────
    @property
    @cpp(get="self->sheet.cols()")
    def cols(self) -> int: "Grid columns (read-only)."

    @property
    @cpp(get="self->sheet.rows()")
    def rows(self) -> int: "Grid rows (read-only)."

    @property
    @cpp(get="self->sheet.frames()")
    def frames(self) -> int:
        ("Cells in the grid (read-only), and so the frame count of an animation - "
         "what you hand a tween as its endpoint. Always at least 1.")

    @property
    @cpp(get="self->sheet.direction()", set="self->sheet.direction((sheet_direction_t){0})")
    def direction(self) -> int:
        ("How the grid numbers its cells: ROWS (default) counts along each row in "
         "turn, COLUMNS down each column - which is what makes a column of a tall "
         "sheet a contiguous run of numbers.")

    # ── cells ───────────────────────────────────────────────────────────────
    @native
    def sprite(self, x: int, y: int = None) -> image:
        ("A cell of the grid, as a sub-image view sharing the sheet's pixels. "
         "sprite(n) takes a cell number, sprite(x, y) a column and a row. A "
         "negative number counts from the end, and anything outside the grid "
         "clamps into it, so no caller has to bounds-check first. Returns an "
         "indexed_image when the source is palettised.")

    # ── what the file said ──────────────────────────────────────────────────
    @property
    @cpp(get_raw="spritesheet_box_timings(self)")
    def timings(self) -> None:
        ("The per-frame durations a GIF declared, in milliseconds, or None for any "
         "other file (read-only). Reported rather than acted on: the sheet keeps "
         "no clock, so this is the information you need to build a tween that "
         "matches how the animation was authored - sum() it for the loop length, "
         "or walk it if the varying holds matter.")

    # ── the image underneath ────────────────────────────────────────────────
    @property
    @cpp(get_raw="MP_OBJ_FROM_PTR(self->source)")
    def source(self) -> None:
        ("The image these cells are carved from (read-only). A sheet keeps it "
         "alive, so holding the sheet is enough.")
