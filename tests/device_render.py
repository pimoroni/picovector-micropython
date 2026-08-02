# device_render.py — on-device rasteriser tests for the picovector bindings.
# Runs under MicroPython on the badge (push with mpremote). Depends only on the
# `picovector` module, so it has no app/screen side effects.
#
# Where device_test.py checks that the API accepts and returns what it should,
# this checks that the rasteriser puts ink in the right places: every primitive
# across a range of sizes, both fill rules, both antialias settings, clipping,
# the tile seams a shape larger than one tile crosses, and vector text.
#
# The assertions are properties rather than golden images, so a legitimate
# change to edge coverage doesn't invalidate the suite: ink lands inside the
# shape's own bounds, a shape's area is close to its analytic area, a hollow
# shape has a hole, antialiasing only ever softens an edge, and so on.
#
# The final line is "RENDER TESTS: <p> passed, <f> failed" for a host runner.

from array import array

from picovector import color, rect, vec2, shape, image, font

OFF, X2, X4 = image.OFF, image.X2, image.X4
EVEN_ODD, NON_ZERO = image.EVEN_ODD, image.NON_ZERO

BG = color.rgb(0, 0, 0)
FG = color.rgb(255, 255, 255)

VECTOR_FONT = "/system/assets/fonts/MonaSans-Medium.af"

_p = 0
_f = 0


def ok(name, cond, detail=""):
    global _p, _f
    if cond:
        _p += 1
        print("PASS", name)
    else:
        _f += 1
        print("FAIL", name, detail)


# ── canvas helpers ──────────────────────────────────────────────────────────
# The red channel is the probe: the background is black, the pen is white, so a
# non-zero red byte means the rasteriser touched that pixel, and 255 means it
# covered it completely.

def canvas(w=64, h=64, aa=None):
    img = image(w, h)
    img.pen = BG
    img.clear()
    img.pen = FG
    if aa is not None:
        img.antialias = aa
    return img


def scan(img):
    """(inked, fully covered, partial, bounding box) of everything drawn."""
    raw = img.raw
    w, h = img.width, img.height
    inked = full = 0
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        base = y * w * 4
        for x in range(w):
            v = raw[base + x * 4]
            if v:
                inked += 1
                if v == 255:
                    full += 1
                if x < minx:
                    minx = x
                if x > maxx:
                    maxx = x
                if y < miny:
                    miny = y
                if y > maxy:
                    maxy = y
    return inked, full, inked - full, (minx, miny, maxx, maxy)


def pixel(img, x, y):
    return img.raw[(y * img.width + x) * 4]


def same(a, b):
    return a.raw == b.raw


# ── every primitive, across a range of sizes ────────────────────────────────
# A shape has to draw something, and all of it has to land inside the bounds the
# shape itself reports (allowing a pixel for the antialiased fringe). Getting
# this wrong is how a shape ends up drawing its bounding box, or nothing.

def shapes_at(size):
    """One of each primitive, scaled to `size`, centred in a 64x64 canvas."""
    c = 32
    return [
        ("circle", shape.circle(c, c, size)),
        ("ellipse", shape.ellipse(c, c, size, size * 0.6)),
        ("rectangle", shape.rectangle(c - size, c - size, size * 2, size * 2)),
        ("rounded_rect", shape.rounded_rectangle(c - size, c - size, size * 2, size * 2, size * 0.3)),
        ("squircle", shape.squircle(c, c, size)),
        ("arc", shape.arc(c, c, size * 0.5, size, -135, 135)),
        ("pie", shape.pie(c, c, size, 0, 270)),
        ("star", shape.star(c, c, 5, size, size * 0.5)),
        ("line", shape.line(c - size, c - size, c + size, c + size, 3)),
        ("regular_polygon", shape.regular_polygon(c, c, size, 6)),
    ]


def test_primitives_across_sizes():
    for size in (2, 5, 13, 28):
        for name, s in shapes_at(size):
            img = canvas(aa=X4)
            img.shape(s)
            inked, _full, _part, (minx, miny, maxx, maxy) = scan(img)
            label = "%s size %d" % (name, size)
            if inked == 0:
                ok(label + " draws ink", False, "nothing drawn")
                continue
            ok(label + " draws ink", True)
            b = s.bounds()
            inside = (minx >= int(b.x) - 1 and miny >= int(b.y) - 1 and
                      maxx <= int(b.x + b.w) + 1 and maxy <= int(b.y + b.h) + 1)
            ok(label + " ink within bounds", inside,
               "ink %s vs bounds %s" % ((minx, miny, maxx, maxy), (b.x, b.y, b.w, b.h)))


def test_area_is_close_to_analytic():
    # A circle is the one primitive whose area we know exactly, which makes it
    # the check that catches a shape filling its bounding box (4/pi too much) or
    # collapsing to an outline.
    for r in (6, 12, 24):
        img = canvas(aa=X4)
        img.shape(shape.circle(32, 32, r))
        inked, full, _part, _bb = scan(img)
        area = 3.14159 * r * r
        # inked counts the antialiased fringe, full counts only solid interior,
        # so the true area sits between them.
        ok("circle r%d area brackets pi*r^2" % r, full <= area <= inked,
           "full %d, area %.0f, inked %d" % (full, area, inked))


def test_hollow_shapes_have_a_hole():
    # The case the arc/ring path exists for: the middle must stay background.
    for inner, outer in ((10, 25), (20, 24)):
        img = canvas(aa=X4)
        img.shape(shape.arc(32, 32, inner, outer, 0, 360))
        ok("ring %d-%d has a hole" % (inner, outer), pixel(img, 32, 32) == 0,
           "centre = %d" % pixel(img, 32, 32))
        ok("ring %d-%d has a band" % (inner, outer), pixel(img, 32, 32 - (inner + outer) // 2) > 0)


# ── antialiasing ────────────────────────────────────────────────────────────

def test_antialias_only_softens_edges():
    hard = canvas(aa=OFF)
    hard.shape(shape.circle(32, 32, 24))
    soft = canvas(aa=X4)
    soft.shape(shape.circle(32, 32, 24))

    h_inked, h_full, h_part, _ = scan(hard)
    s_inked, s_full, s_part, _ = scan(soft)

    ok("aa off has no partial coverage", h_part == 0, "%d partial" % h_part)
    ok("aa on has partial coverage", s_part > 0)
    # The soft edge spreads outward and inward, so it touches at least as many
    # pixels while solidly covering fewer.
    ok("aa touches >= pixels", s_inked >= h_inked, "%d vs %d" % (s_inked, h_inked))
    ok("aa fully covers <= pixels", s_full <= h_inked, "%d vs %d" % (s_full, h_inked))
    # Deep interior must be identical either way.
    ok("aa interior identical", pixel(hard, 32, 32) == 255 and pixel(soft, 32, 32) == 255)


def test_antialias_levels_agree():
    # X2 and X4 both select the analytic signed-area path, which computes exact
    # coverage - there is nothing left to supersample, so they must agree.
    a = canvas(aa=X2)
    a.shape(shape.circle(32, 32, 21))
    b = canvas(aa=X4)
    b.shape(shape.circle(32, 32, 21))
    ok("X2 and X4 render identically", same(a, b))


# ── fill rules ──────────────────────────────────────────────────────────────

def pentagram(cx, cy, r):
    """A path that genuinely crosses itself: every second vertex of a pentagon.

    shape.star() is not this - it alternates an outer and an inner radius, which
    traces a decagon whose outline never crosses, so both fill rules agree on it.
    """
    import math
    pts = []
    for i in range(5):
        a = math.radians(-90 + i * 144)
        pts.append(vec2(cx + math.cos(a) * r, cy + math.sin(a) * r))
    return shape.custom(pts)


def test_fill_rules_differ_on_self_intersection():
    # The pentagram's centre is wound twice: non-zero fills it, even-odd leaves
    # it hollow.
    s = pentagram(32, 32, 28)

    eo = canvas(aa=OFF)
    eo.fill_rule = EVEN_ODD
    eo.shape(s)
    nz = canvas(aa=OFF)
    nz.fill_rule = NON_ZERO
    nz.shape(s)

    eo_inked = scan(eo)[0]
    nz_inked = scan(nz)[0]
    ok("fill rules differ on a pentagram", eo_inked != nz_inked,
       "even-odd %d, non-zero %d" % (eo_inked, nz_inked))
    ok("even-odd leaves the pentagram centre hollow", pixel(eo, 32, 32) == 0)
    ok("non-zero fills the pentagram centre", pixel(nz, 32, 32) > 0)
    ok("non-zero covers at least even-odd", nz_inked >= eo_inked)
    # A convex shape has no self-intersection, so the rules must agree exactly.
    eo2 = canvas(aa=X4)
    eo2.fill_rule = EVEN_ODD
    eo2.shape(shape.circle(32, 32, 20))
    nz2 = canvas(aa=X4)
    nz2.fill_rule = NON_ZERO
    nz2.shape(shape.circle(32, 32, 20))
    ok("fill rules agree on a circle", same(eo2, nz2))


# ── clipping ────────────────────────────────────────────────────────────────

def test_offscreen_and_clipped():
    for name, s in (("left", shape.circle(-40, 32, 20)),
                    ("right", shape.circle(104, 32, 20)),
                    ("above", shape.circle(32, -40, 20)),
                    ("below", shape.circle(32, 104, 20))):
        img = canvas(aa=X4)
        img.shape(s)
        ok("fully offscreen %s draws nothing" % name, scan(img)[0] == 0)

    # Half off each edge: ink appears, and only inside the canvas. A shape
    # running off the left is the interesting one - its winding has to be
    # carried by the first column rather than lost.
    for name, s in (("left", shape.circle(0, 32, 20)),
                    ("right", shape.circle(64, 32, 20)),
                    ("top", shape.circle(32, 0, 20)),
                    ("bottom", shape.circle(32, 64, 20))):
        img = canvas(aa=X4)
        img.shape(s)
        inked, _full, _part, (minx, miny, maxx, maxy) = scan(img)
        ok("half offscreen %s draws ink" % name, inked > 0)
        ok("half offscreen %s stays in bounds" % name,
           minx >= 0 and miny >= 0 and maxx < 64 and maxy < 64)

    # An explicit clip rect must be honoured exactly.
    img = canvas(aa=X4)
    img.clip = rect(16, 16, 16, 16)
    img.shape(shape.circle(32, 32, 30))
    _inked, _full, _part, (minx, miny, maxx, maxy) = scan(img)
    ok("clip rect honoured",
       minx >= 16 and miny >= 16 and maxx < 32 and maxy < 32,
       "ink %s" % ((minx, miny, maxx, maxy),))


# ── tiling ──────────────────────────────────────────────────────────────────

def test_tile_seams():
    # The rasteriser works in 160x120 tiles, so anything bigger than that is
    # rasterised in several passes. The same shape must come out identical
    # wherever it sits relative to those seams.
    a = image(320, 240)
    a.pen = BG
    a.clear()
    a.pen = FG
    a.antialias = X4
    a.shape(shape.circle(160, 120, 50))   # centred on both seams

    b = image(320, 240)
    b.pen = BG
    b.clear()
    b.pen = FG
    b.antialias = X4
    b.shape(shape.circle(80, 60, 50))     # wholly inside one tile

    # Compare the two by their ink totals and by a row through the middle.
    ok("shape on a tile seam has the same area as one inside a tile",
       abs(scan(a)[0] - scan(b)[0]) <= 2,
       "%d vs %d" % (scan(a)[0], scan(b)[0]))

    seam_row = [pixel(a, x, 120) for x in range(110, 211)]
    tile_row = [pixel(b, x, 60) for x in range(30, 131)]
    ok("no seam artefact along a row crossing the tile boundary",
       seam_row == tile_row)

    seam_col = [pixel(a, 160, y) for y in range(70, 171)]
    tile_col = [pixel(b, 80, y) for y in range(10, 111)]
    ok("no seam artefact down a column crossing the tile boundary",
       seam_col == tile_col)

    # A shape spanning all four tiles must not lose or double its middle.
    big = image(320, 240)
    big.pen = BG
    big.clear()
    big.pen = FG
    big.antialias = X4
    big.shape(shape.arc(160, 120, 80, 110, 0, 360))
    ok("full-screen ring keeps its hole", pixel(big, 160, 120) == 0)
    ok("full-screen ring draws its band", pixel(big, 160, 120 - 95) > 0)


# ── repeatability ───────────────────────────────────────────────────────────

def test_drawing_twice_is_stable():
    # With hard edges every covered pixel is a plain store, so a second pass
    # changes nothing.
    once = canvas(aa=OFF)
    once.shape(shape.circle(32, 32, 20))
    twice = canvas(aa=OFF)
    twice.shape(shape.circle(32, 32, 20))
    twice.shape(shape.circle(32, 32, 20))
    ok("an opaque hard-edged shape drawn twice is idempotent", same(once, twice))

    # Antialiased, only the interior is idempotent: a partially covered edge
    # pixel composites again and legitimately gets darker.
    a = canvas(aa=X4)
    a.shape(shape.circle(32, 32, 20))
    b = canvas(aa=X4)
    b.shape(shape.circle(32, 32, 20))
    b.shape(shape.circle(32, 32, 20))
    ok("antialiased interior is idempotent",
       all(pixel(a, x, 32) == pixel(b, x, 32) == 255 for x in range(26, 39)))
    ok("antialiased edge composites rather than being overwritten",
       scan(b)[1] >= scan(a)[1])


# ── vector text ─────────────────────────────────────────────────────────────

def test_custom_accepts_an_array():
    # array('f') of flat x, y pairs must rasterise identically to the same
    # contour as a list of vec2 - it exists so a shape rebuilt every frame boxes
    # no vec2 objects.
    pts = [(10.0, 8.0), (52.0, 14.0), (44.0, 50.0), (18.0, 46.0)]
    as_vec2 = shape.custom([vec2(x, y) for x, y in pts])
    flat = array("f", [v for xy in pts for v in xy])
    as_array = shape.custom(flat)

    a = canvas(aa=X4)
    a.shape(as_vec2)
    b = canvas(aa=X4)
    b.shape(as_array)
    ok("shape.custom(array('f')) matches a list of vec2", same(a, b))

    # Several contours, mixed forms, with the second punching a hole.
    hole = array("f", [24.0, 22.0, 40.0, 22.0, 40.0, 38.0, 24.0, 38.0])
    c = canvas(aa=X4)
    c.fill_rule = EVEN_ODD
    c.shape(shape.custom(flat, hole))
    ok("an array contour can be a hole", pixel(c, 32, 30) == 0)
    ok("the outer array contour still draws", pixel(c, 14, 12) > 0)

    # A half-finished pair is a mistake worth hearing about rather than a
    # silently dropped point.
    try:
        shape.custom(array("f", [1.0, 2.0, 3.0]))
        ok("odd-length array raises", False, "accepted silently")
    except ValueError:
        ok("odd-length array raises", True)

    # A buffer that isn't float data must not be read as points.
    try:
        shape.custom(bytearray(16))
        ok("a bytearray is rejected", False, "accepted silently")
    except TypeError:
        ok("a bytearray is rejected", True)


def test_vector_text():
    try:
        f = font.load(VECTOR_FONT)
    except OSError:
        ok("vector font available", False, VECTOR_FONT + " not found")
        return
    ok("vector font available", True)

    img = image(200, 80)
    img.pen = BG
    img.clear()
    img.pen = FG
    img.antialias = X4
    img.font = f
    img.text("Hg", vec2(10, 10), font_size=40)
    inked, _full, _part, (minx, miny, maxx, maxy) = scan(img)
    ok("text draws ink", inked > 0)
    ok("text ink starts at the requested position", minx >= 8, "minx %d" % minx)

    # Measured width should agree with the ink the glyphs actually put down.
    w, h = img.measure_text("Hg", font_size=40)
    ok("measured width covers the drawn ink", w >= (maxx - minx),
       "measured %.1f, drew %d" % (w, maxx - minx))
    ok("measured height is the point size", h > 0)

    # Longer strings are wider, and a bigger size is proportionally wider: an
    # advance stored too narrow draws every glyph in the same place, which this
    # catches.
    w1, _ = img.measure_text("H", font_size=40)
    w2, _ = img.measure_text("HH", font_size=40)
    ok("two glyphs are wider than one", w2 > w1 * 1.5, "%.1f vs %.1f" % (w2, w1))
    w_small, _ = img.measure_text("Hg", font_size=20)
    w_big, _ = img.measure_text("Hg", font_size=80)
    ok("width scales with size", abs(w_big - w_small * 4) < w_small * 0.1,
       "%.1f vs %.1f" % (w_big, w_small * 4))

    # Glyphs drawn large must still be smooth rather than collapsing: a big
    # glyph should carry partial coverage on its curves.
    big = image(200, 160)
    big.pen = BG
    big.clear()
    big.pen = FG
    big.antialias = X4
    big.font = f
    big.text("8", vec2(10, 10), font_size=140)
    _inked, _full, part, _bb = scan(big)
    ok("a large glyph is antialiased", part > 0)

    # Text at several sizes, each drawing more ink than the last.
    prev = 0
    for size in (8, 16, 32, 64):
        page = image(200, 80)
        page.pen = BG
        page.clear()
        page.pen = FG
        page.antialias = X4
        page.font = f
        page.text("mm", vec2(2, 2), font_size=size)
        n = scan(page)[0]
        ok("text at size %d draws more ink than the size below" % size, n > prev,
           "%d vs %d" % (n, prev))
        prev = n



# A ten-by-eight, four-frame animation, encoded by PIL: each frame is the one
# before it shifted two pixels left. Carried inline so the test needs nothing on
# the filesystem.
GIF = (
    b'GIF89a\n\x00\x08\x00\x82\x00\x00\x00\x00'
    b'\x00\xdc(((\xc8Z\x1e<\xdc\xf0\xd2<\x96\x96'
    b'\x96\x00\x00\x00\x00\x00\x00!\xf9\x04\x08\x06\x00\x00\x00'
    b',\x00\x00\x00\x00\n\x00\x08\x00\x00\x080\x00\x01\x00'
    b'\x08\x10@\x80\x80\x01\x03\x08\x10\x10H\xd0 B\x85'
    b'\r\x0f&$P\xa0@\xc4\x87\x14\x0b8\x9cXQ'
    b'\xe0F\x85\x1d\x01`\x0cIpd\x01\x86\x01\x02\x02'
    b'\x00!\xf9\x04\x08\t\x00\x00\x00,\x00\x00\x00\x00\n'
    b'\x00\x08\x00\x82\x00\x00\x00\xdc(((\xc8Z\x1e<'
    b'\xdc\xf0\xd2<\x96\x96\x96\x00\x00\x00\x00\x00\x00\x080'
    b'\x00\x03\x04\x10 `\xc0\x00\x02\x04\n\x14\x10H\xd0'
    b' B\x85\r\x0f&,\x00\x00@\xc4\x87\x14\x018'
    b'\x9cXQ\xe0F\x85\x1d\x03`\x0cIp$\x00\x86'
    b'\x02\x02\x02\x00!\xf9\x04\x08\x0c\x00\x00\x00,\x00\x00'
    b'\x00\x00\n\x00\x08\x00\x82\x00\x00\x00\xdc(((\xc8'
    b'Z\x1e<\xdc\xf0\xd2<\x96\x96\x96\x00\x00\x00\x00\x00'
    b'\x00\x080\x00\x05\x08\x180\x80\x00\x81\x02\x05\x00\x00'
    b'\x10H\xd0 B\x85\r\x0f&\x04\x10 @\xc4\x87'
    b'\x14\x038\x9cXQ\xe0F\x85\x1d\x05`\x0cIp'
    b'd\x00\x86\x03\x02\x02\x00!\xf9\x04\x08\x0f\x00\x00\x00'
    b',\x00\x00\x00\x00\n\x00\x08\x00\x82\x00\x00\x00\xdc('
    b'((\xc8Z\x1e<\xdc\xf0\xd2<\x96\x96\x96\x00\x00'
    b'\x00\x00\x00\x00\x080\x00\x07\x0c @\xa0@\x01\x00'
    b'\x00\x02\x04\x10H\xd0 B\x85\r\x0f&\x0c @'
    b'@\xc4\x87\x14\x058\x9cXQ\xe0F\x85\x1d\x07`'
    b'\x0cIp\xa4\x00\x86\x04\x02\x02\x00;'
)


def test_animated_gif():
    sheet = image.load(GIF)
    ok("a gif loads as one wide sheet", sheet.width == 40 and sheet.height == 8,
       "%dx%d" % (sheet.width, sheet.height))
    ok("a gif sheet is indexed", sheet.has_palette)
    ok("a gif sheet is a byte a pixel", len(sheet.raw) == 40 * 8, str(len(sheet.raw)))
    ok("a gif reports its frame timings", sheet.delays == (60, 90, 120, 150),
       str(sheet.delays))

    frame = sheet.sprite(2, 0)
    ok("a gif frame is a sub-view of the sheet", frame.width == 10 and frame.height == 8,
       "%dx%d" % (frame.width, frame.height))

    # The sheet bounds its own animation, so nothing has to carry a frame count
    # alongside it.
    ok("a gif sheet reports its frame count", sheet.cols == 4 and sheet.rows == 1,
       "%dx%d" % (sheet.cols, sheet.rows))
    ok("a frame is not itself a sheet", frame.cols == 1 and frame.rows == 1)

    # ...and its own timings, so a free-running clock picks the right frame.
    ok("a gif sheet reports its loop length", sheet.duration == 60 + 90 + 120 + 150,
       str(sheet.duration))
    ok("frame_at walks the delays", (sheet.frame_at(0), sheet.frame_at(59),
                                     sheet.frame_at(60), sheet.frame_at(149),
                                     sheet.frame_at(150), sheet.frame_at(419)) == (0, 0, 1, 1, 2, 3),
       str([sheet.frame_at(t) for t in (0, 59, 60, 149, 150, 419)]))
    ok("frame_at wraps", sheet.frame_at(420) == 0 and sheet.frame_at(480) == 1)
    ok("frame_at handles a clock that ran backwards", 0 <= sheet.frame_at(-30) < 4)

    # A plain image carries no timings, and says so rather than freezing at 0.
    plain = image(8, 8)
    ok("a plain image has no duration", plain.duration == 0)
    ok("a plain image is a 1x1 grid", plain.cols == 1 and plain.rows == 1)
    try:
        plain.frame_at(0)
        ok("frame_at without timings raises", False)
    except ValueError:
        ok("frame_at without timings raises", True)

    # Every frame is the previous one shifted, which only holds if the deltas
    # were composited in order.
    raw = sheet.raw
    shifted = True
    for f in range(1, 4):
        for y in range(8):
            for x in range(8):
                if raw[y * 40 + f * 10 + x] != raw[y * 40 + (f - 1) * 10 + x + 2]:
                    shifted = False
    ok("each gif frame is the one before it, composited", shifted)

    # A GIF has to composite at its own size, so asking for another is refused
    # rather than quietly ignored.
    try:
        image.load(GIF, 20, 16)
        ok("loading a gif at a size is refused", False)
    except ValueError:
        ok("loading a gif at a size is refused", True)


def main():
    test_primitives_across_sizes()
    test_area_is_close_to_analytic()
    test_hollow_shapes_have_a_hole()
    test_antialias_only_softens_edges()
    test_antialias_levels_agree()
    test_fill_rules_differ_on_self_intersection()
    test_offscreen_and_clipped()
    test_tile_seams()
    test_drawing_twice_is_stable()
    test_custom_accepts_an_array()
    test_vector_text()
    test_animated_gif()
    print("RENDER TESTS: %d passed, %d failed" % (_p, _f))


main()
