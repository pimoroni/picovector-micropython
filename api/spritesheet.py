"""spritesheet — a grid over an image, and optionally how long each cell shows for.

``image.spritesheet(cols, rows)`` returns one of these. A sheet owns no pixels; it
describes how to carve someone else's buffer into cells, which cells make up a
sequence, and what that sequence's timings are. ``range()`` narrows it and gives
back another sheet, so one type covers an icon atlas, a whole-sheet animation and
two animations over one image.

Timing is optional and absent by default, because most sheets are atlases. Set an
``interval`` (or load a GIF, which brings its own per-frame delays) and the same
sheet answers ``at``/``now``/``done`` as well. The clock and the self-timing read
alike to ``tween``, deliberately.
"""

from __future__ import annotations

from pv import api, cpp, native, const, XY


@api("spritesheet_t", field="sheet", print=("spritesheet(%d x %d, %d frames)",
     "self->sheet.cols()", "self->sheet.rows()", "self->sheet.frames()"))
class spritesheet:
    """A grid over an image, and a timed sequence of cells within it.

    You do not construct one directly: ``image.spritesheet(cols, rows)`` and
    ``sheet.range(origin, dest)`` both return one.
    """

    # Which way a *rectangular* range is walked. A range with one coordinate
    # fixed is a single row or column and needs neither.
    ROWS = const("SHEET_ROWS", "Walk a rectangular range along each row in turn (default).")
    COLUMNS = const("SHEET_COLUMNS", "Walk a rectangular range down each column in turn.")

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
        ("How many cells this sheet's range covers (read-only). The frame count "
         "of an animation, and always at least 1.")

    # ── cells ───────────────────────────────────────────────────────────────
    @native
    def sprite(self, x: int, y: int) -> image:
        ("The cell at grid (x, y) as a sub-image view sharing the sheet's pixels. "
         "x is the column, y the row. Grid-absolute, so it means the same thing "
         "on a narrowed range as on the whole sheet. Returns an indexed_image if "
         "the source is palettised.")

    @native
    def frame(self, index: int) -> image:
        ("The cell at a position in this sheet's sequence, as a sub-image view. "
         "Where sprite() indexes the grid, this indexes the range, so frame(0) is "
         "the range's origin whichever cell that is. Clamps.")

    @cpp(native=True, kw=True)
    def range(self, origin: XY = None, dest: XY = None, interval=None,
              loop=None, direction=None) -> spritesheet:
        ("A new sheet over the same pixels, covering the inclusive cell range "
         "origin..dest. The two cells imply the axis whenever one coordinate is "
         "fixed, so vec2(0, 1) to vec2(4, 1) is five frames along row 1; "
         "direction (ROWS or COLUMNS) only decides the walk for a rectangular "
         "range, which has to wrap. A range given backwards plays backwards. "
         "interval, loop and direction default to this sheet's. Omit origin and "
         "dest for the whole grid.")

    # ── timing ──────────────────────────────────────────────────────────────
    @property
    @cpp(get_raw="mp_obj_new_int(self->sheet.duration())")
    def duration(self) -> int:
        ("How long one pass through the range takes in ms (read-only), or 0 when "
         "nothing is timed - which is what an atlas is.")

    # Read or set the per-frame timings. A method rather than a property because
    # it takes either a single value or a sequence, the same shape image.palette
    # has for the same reason.
    @native
    def interval(self, ms=None) -> None:
        ("interval() reads the timings back as a tuple, one entry per frame. "
         "interval(ms) sets a uniform rate; interval(sequence) sets one value per "
         "frame. A GIF arrives with the file's own delays already set, and an "
         "untimed sheet reads back as all zeroes.")

    @property
    @cpp(get="self->sheet.loop()", set="self->sheet.loop({0})")
    def loop(self) -> bool:
        ("Whether the sequence wraps (default True). A one-shot holds on its last "
         "frame when it runs out, rather than snapping back to the first.")

    @property
    @cpp(get="self->sheet.direction()", set="self->sheet.direction((sheet_direction_t){0})")
    def direction(self) -> int: "How a rectangular range is walked: ROWS or COLUMNS."

    @cpp(call="self->sheet.index_at", emit="free", args="ms")
    def index(self, ms: int) -> int:
        ("Which frame of the sequence covers ms, honouring the per-frame timings. "
         "Wraps while loop is set; holds on the last frame when it is not. Use "
         "this over at() when you want the number and no allocation.")

    @native
    def at(self, ms: int) -> image:
        ("The cell covering ms into the sequence, as a sub-image view - index() "
         "and frame() in one call. Wraps, so a free-running clock goes straight "
         "in: screen.blit(sheet.at(badge.ticks), pos).")

    # ── self-timing, as tween does it ───────────────────────────────────────
    @native
    def start(self, t: int = None) -> spritesheet:
        ("Start (or restart) self-timing from the current clock and return self "
         "for chaining. Pass t to start from an explicit clock value. elapsed, "
         "now and done then track time since this call.")

    def stop(self) -> None:
        "Stop self-timing. elapsed holds at 0 and done reads False until the next start()."

    @property
    @cpp(get="self->sheet.elapsed()")
    def elapsed(self) -> int:
        "Milliseconds since start(), or 0 before the first one (read-only)."

    @property
    @cpp(get_raw="spritesheet_box_now(self)")
    def now(self) -> None:
        "The cell at the current clock time, as a sub-image view; needs start()."

    @property
    @cpp(get="self->sheet.done()")
    def done(self) -> bool:
        ("True once a non-looping sequence has run out since start() (read-only). "
         "A looping sheet is never done, and neither is one that has not started.")

    @property
    @cpp(get="self->sheet.running()")
    def running(self) -> bool: "True between start() and stop() (read-only)."

    # ── the image underneath ────────────────────────────────────────────────
    @property
    @cpp(get_raw="MP_OBJ_FROM_PTR(self->source)")
    def source(self) -> None:
        ("The image these cells are carved from (read-only). A sheet keeps it "
         "alive, so holding the sheet is enough.")
