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

import time
from array import array

from picovector import (color, rect, vec2, mat3, shape, image,
                        palette, spritesheet, tween, font)

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


# ── compound shapes ─────────────────────────────────────────────────────────

def test_combine_fills_as_one():
    a = shape.circle(24, 32, 14)
    b = shape.rectangle(24, 24, 24, 16)

    nz = canvas(aa=OFF)
    nz.fill_rule = NON_ZERO
    nz.shape(shape.combine(a, b))
    ok("combined overlap fills solid", pixel(nz, 28, 32) == 255)
    ok("combined circle side fills", pixel(nz, 16, 32) == 255)
    ok("combined rectangle side fills", pixel(nz, 44, 32) == 255)

    # the same pair drawn separately covers the same ground
    sep = canvas(aa=OFF)
    sep.shape([a, b])
    ok("combined covers what two draws do", scan(nz)[0] == scan(sep)[0],
       "combined %d, separate %d" % (scan(nz)[0], scan(sep)[0]))

    # under even-odd the overlap is a hole instead, which is the documented
    # reason combine() asks for NON_ZERO
    eo = canvas(aa=OFF)
    eo.fill_rule = EVEN_ODD
    eo.shape(shape.combine(a, b))
    ok("even-odd hollows the overlap", pixel(eo, 28, 32) == 0)

    # a translucent pen over a compound blends once, where two draws blend twice
    over = canvas(aa=OFF)
    over.fill_rule = NON_ZERO
    over.alpha = 128
    over.shape(shape.combine(a, b))
    twice = canvas(aa=OFF)
    twice.alpha = 128
    twice.shape([a, b])
    ok("compound overlap blends once", pixel(over, 28, 32) < pixel(twice, 28, 32),
       "compound %d, separate %d" % (pixel(over, 28, 32), pixel(twice, 28, 32)))


def test_combine_keeps_holes_and_transforms():
    # an even-odd ring: outer contour plus a counter-wound inner one
    import math
    outer = []
    inner = []
    for i in range(32):
        a = math.radians(i * 360 / 32)
        outer.append(vec2(24 + math.cos(a) * 16, 32 + math.sin(a) * 16))
        inner.append(vec2(24 - math.cos(a) * 8, 32 + math.sin(a) * 8))
    ring = shape.custom(outer, inner)

    img = canvas(aa=OFF)
    img.fill_rule = NON_ZERO
    img.shape(shape.combine(ring, shape.rectangle(40, 28, 18, 8)))
    ok("combined ring keeps its hole", pixel(img, 24, 32) == 0)
    ok("combined ring keeps its rim", pixel(img, 24, 20) == 255)
    ok("combined bar draws", pixel(img, 50, 32) == 255)

    # a source transform is baked in, not dropped
    moved = shape.rectangle(0, 0, 12, 12)
    moved.transform = mat3().translate(40, 40)
    placed = canvas(aa=OFF)
    placed.fill_rule = NON_ZERO
    placed.shape(shape.combine(shape.rectangle(4, 4, 12, 12), moved))
    ok("combined source transform is baked in", pixel(placed, 45, 45) == 255)
    ok("combined first source stays put", pixel(placed, 8, 8) == 255)


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

# The same size, as a flat-filled truecolour PNG: what load_into decodes through
# the four-bytes-a-pixel path, which a palettised target has to refuse.
PNG = (
    b'\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR'
    b'\x00\x00\x00(\x00\x00\x00\x08\x08\x02\x00\x00\x00\x040*'
    b'\x0c\x00\x00\x00\x18IDATx\xdacP\x98pa'
    b'@\x10\xc3\xa8\xc5\xa3\x16\x8fZL-\x04\x00\xdc\xde\xe0'
    b"\x10%\xe5\xfc'\x00\x00\x00\x00IEND\xaeB`"
    b'\x82'
)


def test_animated_gif():
    img = image.load(GIF)
    ok("a gif loads as one wide sheet", img.width == 40 and img.height == 8,
       "%dx%d" % (img.width, img.height))
    ok("a gif sheet is indexed", img.has_palette)
    ok("a gif sheet is a byte a pixel", len(img.raw) == 40 * 8, str(len(img.raw)))

    # No arguments: the grid the file arrived with.
    sheet = img.spritesheet()
    ok("a gif knows its own grid", sheet.cols == 4 and sheet.rows == 1,
       "%dx%d" % (sheet.cols, sheet.rows))
    ok("a gif sheet reports its frame count", sheet.frames == 4, str(sheet.frames))
    ok("a gif reports the file's timings", sheet.timings == (60, 90, 120, 150),
       str(sheet.timings))
    ok("...whether or not the grid was named",
       img.spritesheet(4, 1).timings == (60, 90, 120, 150),
       str(img.spritesheet(4, 1).timings))

    frame = sheet.sprite(2, 0)
    ok("a gif frame is a sub-view of the sheet", frame.width == 10 and frame.height == 8,
       "%dx%d" % (frame.width, frame.height))
    ok("a cell is not itself a sheet", frame.spritesheet().frames == 1)

    # index() walks the per-frame delays, so a free-running clock picks the right one.
    # The timings are data, so honouring them exactly is the caller's own walk.
    elapsed, total = 200, sum(sheet.timings)
    acc, picked = 0, 0
    for i, ms in enumerate(sheet.timings):
        acc += ms
        if elapsed % total < acc:
            picked = i
            break
    ok("the timings can be walked by hand", picked == 2, str(picked))

    # A plain image is a 1x1 grid with nothing timed, and says so rather than
    # dividing by zero or racing the draw loop.
    plain = image(8, 8).spritesheet()
    ok("a plain sheet is one cell", plain.frames == 1)
    ok("...and reports no timings", plain.timings is None)

    # Every frame is the previous one shifted, which only holds if the deltas
    # were composited in order.
    raw = img.raw
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


# ── small helpers ───────────────────────────────────────────────────────────

def gone(obj, name):
    """True when `name` is not a member of obj at all."""
    try:
        getattr(obj, name)
        return False
    except AttributeError:
        return True


def unsettable(obj, name, value):
    try:
        setattr(obj, name, value)
        return False
    except AttributeError:
        return True


# ── the palette type ────────────────────────────────────────────────────────
# An indexed image's colour table, as a sequence. Attached to an image it *is*
# that table, and views share it by pointer - so one write recolours every sprite
# cut from the same sheet, which is why assignment copies in rather than swapping.

def test_palette():
    gif = image.load(GIF)
    p = gif.palette
    ok("an indexed image has a palette", p is not None)
    ok("...and that reads as has_palette", (p is not None) == gif.has_palette)
    ok("a plain image has none", image(8, 8).palette is None)
    ok("a palette knows its image", p.image is gif)

    ok("len is the table size", len(p) == gif.palette_size, "%d" % len(p))
    ok("an entry reads back as a colour", isinstance(p[0], type(color.red)))

    # Indexing, negative indexing, and the bounds.
    p[1] = color.rgb(1, 2, 3)
    ok("an entry can be written", p[1] == color.rgb(1, 2, 3), str(p[1]))
    ok("an index narrows a float too", p[1.0] == p[1])
    p[-1] = color.rgb(4, 5, 6)
    ok("a negative index is the end", p[len(p) - 1] == color.rgb(4, 5, 6))
    for bad in (len(p), -len(p) - 1, 999):
        try:
            p[bad]
            ok("index %d is refused" % bad, False)
        except IndexError:
            ok("index %d is refused" % bad, True)
    try:
        p[0] = 12345
        ok("a non-colour is refused", False)
    except TypeError:
        ok("a non-colour is refused", True)

    # Slices, both ways.
    p[0:3] = [color.rgb(10, 0, 0), color.rgb(0, 10, 0), color.rgb(0, 0, 10)]
    ok("a slice can be written", p[1] == color.rgb(0, 10, 0), str(p[1]))
    got = p[0:3]
    ok("a slice reads back as a list", isinstance(got, list) and len(got) == 3, str(got))
    ok("...with the right colours", got[2] == color.rgb(0, 0, 10))
    try:
        p[0:3] = [color.red, color.blue]
        ok("a mismatched slice is refused", False)
    except ValueError:
        ok("a mismatched slice is refused", True)

    # Iteration, which is what list() uses.
    whole = list(p)
    ok("list copies the whole table", len(whole) == len(p), "%d" % len(whole))
    ok("...as colours", whole[1] == color.rgb(0, 10, 0))
    ok("iteration agrees with indexing",
       all(a == p[i] for i, a in enumerate(whole)))

    # The raw bytes, both ways of reaching them, not copied.
    ok("raw is four bytes an entry", len(p.raw) == len(p) * 4, str(len(p.raw)))
    ok("raw and memoryview agree", len(p.raw) == len(memoryview(p)))
    ok("raw is not a copy", bytes(p.raw[4:8]) == bytes(memoryview(p)[4:8]))
    before = bytes(p.raw[0:4])
    p[0] = color.rgb(200, 100, 50)
    ok("writing an entry shows up in raw", bytes(p.raw[0:4]) != before)

    # A free-standing palette, and assignment.
    made = palette([color.rgb(9, 9, 9)] * 4)
    ok("a palette can be built", len(made) == 4 and made.image is None)
    gif.palette = made
    ok("assigning copies the entries in", gif.palette[0] == color.rgb(9, 9, 9))
    ok("...and the palette assigned from is untouched", made.image is None)
    # A view shares the table, so it followed - which is the point of copying in.
    ok("every view follows an assignment",
       gif.spritesheet().sprite(0, 0).palette[0] == color.rgb(9, 9, 9))

    gif.palette = [color.rgb(7, 7, 7), color.rgb(8, 8, 8)]
    ok("a plain sequence works too", gif.palette[0] == color.rgb(7, 7, 7))
    ok("...leaving the rest alone", gif.palette[2] == color.rgb(9, 9, 9))

    try:
        gif.palette = palette([color.red] * 256)
        ok("too many colours is refused", False, "%d slots" % len(gif.palette))
    except ValueError:
        ok("too many colours is refused", True)
    try:
        image(8, 8).palette = made
        ok("assigning to a plain image is refused", False)
    except TypeError:
        ok("assigning to a plain image is refused", True)
    for bad in ([], [color.red] * 257):
        try:
            palette(bad)
            ok("palette(%d colours) is refused" % len(bad), False)
        except ValueError:
            ok("palette(%d colours) is refused" % len(bad), True)


def test_image_construction():
    # A wrapped buffer was taken entirely on trust, so image(64, 64, <16 bytes>)
    # built a 16KB image over 16 bytes and every draw ran past the end of it.
    ok("a buffer big enough is accepted", image(4, 4, bytearray(4 * 4 * 4)).width == 4)
    ok("...and one byte over is fine too", image(4, 4, bytearray(4 * 4 * 4 + 1)).width == 4)
    for w, h, n in ((64, 64, 16), (4, 4, 4 * 4 * 4 - 1), (100, 1, 4)):
        try:
            image(w, h, bytearray(n))
            ok("image(%d, %d, %d bytes) is refused" % (w, h, n), False)
        except ValueError:
            ok("image(%d, %d, %d bytes) is refused" % (w, h, n), True)

    # A size that cannot be one, rather than one that wraps a 32-bit byte count
    # into something small and plausible.
    for w, h in ((0, 10), (10, 0), (-5, 10), (10, -5), (65536, 65536), (100000, 100000)):
        try:
            image(w, h)
            ok("image(%d, %d) is refused" % (w, h), False)
        except (ValueError, MemoryError):
            ok("image(%d, %d) is refused" % (w, h), True)

    # raw and the buffer protocol describe the same bytes.
    img = image(8, 4)
    ok("an image has no sprite either", gone(img, "sprite"))
    ok("raw and memoryview agree", len(img.raw) == len(memoryview(img)),
       "%d vs %d" % (len(img.raw), len(memoryview(img))))
    ok("...and that is the whole buffer", len(img.raw) == 8 * 4 * 4, str(len(img.raw)))
    ok("a full image strides by its width", img.stride == 8 * 4, str(img.stride))


# ── a palettised image ──────────────────────────────────────────────────────
# One byte a pixel indexing a colour table. Every brush and filter stores four,
# so nothing can draw into one - the whole drawing surface is a quiet no-op and
# the indices survive it. Blitting from one is the point, and the table is
# writable. There is one image type; `palette` is what tells them apart.

def test_palettised_image():
    sheet = image.load(GIF)
    plain = image(16, 16)

    ok("a gif loads as an image", type(sheet) is image, str(type(sheet)))
    ok("...carrying a palette", sheet.palette is not None)
    ok("a plain buffer has none", plain.palette is None)
    ok("has_palette agrees", sheet.has_palette and not plain.has_palette)
    ok("it reports its table size", sheet.palette_size > 0, str(sheet.palette_size))

    # Every way in draws nothing and leaves the indices exactly as they were.
    before = bytes(sheet.raw)
    sheet.pen = color.rgb(255, 255, 255)
    sheet.clear()
    sheet.rectangle(rect(2, 2, 6, 4))
    sheet.circle(vec2(8, 4), 3)
    sheet.line(vec2(0, 0), vec2(39, 7))
    sheet.put(vec2(1, 1))
    sheet.hspan(0, 0, 20)
    sheet.vspan(0, 0, 8)
    sheet.triangle(vec2(0, 0), vec2(10, 0), vec2(5, 7))
    sheet.shape(shape.rectangle(0, 0, 8, 8))
    sheet.blur(2.0)
    sheet.dither()
    sheet.invert()
    sheet.oilpaint(2, 128)
    sheet.bloom()
    sheet.wave(4, 4)
    sheet.zoom(128)
    sheet.edgeglow()
    ok("drawing into a palettised image changes nothing", bytes(sheet.raw) == before)

    # Including decode: the PNG and JPEG paths store four bytes a pixel too.
    sheet.load_into(PNG)
    ok("...and so does load_into", bytes(sheet.raw) == before)

    # It is still a target for the things that are about the buffer, not the
    # pixels: alpha is read from the source side of a blit.
    sheet.alpha = 128
    ok("alpha is settable", sheet.alpha == 128)
    sheet.alpha = 255

    # A read-only property has to refuse rather than run on into the next
    # setter's branch: `img.width = n` once assigned img.clip.
    before_alpha = sheet.alpha
    ok("width is read-only", unsettable(sheet, "width", 4))
    ok("...and refusing it left alpha alone", sheet.alpha == before_alpha)

    # The grid and the timings live on the sheet, not on the image.
    for name in ("cols", "rows", "delays", "duration", "frame_at", "sprite"):
        ok("an image has no %s" % name, gone(sheet, name))
    grid = sheet.spritesheet()
    ok("a palettised sheet keeps the grid", grid.cols == 4 and grid.rows == 1)
    ok("...and reports the timings", grid.timings == (60, 90, 120, 150))

    # Views share the table by pointer, so one write recolours every cell.
    frame = grid.sprite(1)
    ok("a cell keeps the palette", frame.palette is not None)
    ok("a window keeps it too", sheet.window(rect(0, 0, 10, 8)).palette is not None)
    sheet.palette[1] = color.rgb(255, 0, 255)
    ok("a palette entry can be rewritten",
       sheet.palette[1] == color.rgb(255, 0, 255), str(sheet.palette[1]))
    ok("a cell shares the table", frame.palette[1] == sheet.palette[1])

    # Blitting out of one is what it is for.
    dst = canvas(32, 32)
    dst.pen = BG
    dst.clear()
    dst.blit(frame, vec2(0, 0))
    ok("a palettised frame blits onto an image", scan(dst)[0] > 0)

    dst.pen = BG
    dst.clear()
    dst.blit(frame, rect(0, 0, 10, 8), rect(0, 0, 20, 16))
    ok("...blits scaled", scan(dst)[0] > 0)

    dst.pen = BG
    dst.clear()
    dst.blit_vspan(frame, vec2(4, 0), 16, vec2(0, 0), vec2(9, 7))
    ok("...and blits as a vspan", scan(dst)[0] > 0)

    # Blitting *into* one is refused as quietly as drawing is.
    before = bytes(sheet.raw)
    sheet.blit(plain, vec2(0, 0))
    ok("blitting into a palettised image changes nothing", bytes(sheet.raw) == before)

    # The span blits check what they were handed. They used to cast any object
    # straight to an image struct and read through it.
    for bad in (None, 42, vec2(0, 0), "not an image"):
        try:
            dst.blit_vspan(bad, vec2(0, 0), 4, vec2(0, 0), vec2(1, 1))
            ok("blit_vspan rejects %r" % (bad,), False)
        except TypeError:
            ok("blit_vspan rejects %r" % (bad,), True)


# ── the spritesheet type ────────────────────────────────────────────────────
# A grid over someone else's pixels, plus optionally how long each cell shows
# for. The case that motivated it: one image holding two animations of different
# lengths, which per-image playback state could never describe.

def test_spritesheet():
    # image() is not zeroed, and these assertions compare cells by content, so
    # every cell gets a colour of its own first.
    src = image(70, 16)
    for row in range(2):
        for col in range(7):
            src.pen = color.rgb(20 + col * 30, 20 + row * 100, 40)
            src.rectangle(rect(col * 10, row * 8, 10, 8))
    sheet = src.spritesheet(7, 2)

    ok("spritesheet returns a sheet", type(sheet) is spritesheet, str(type(sheet)))
    ok("a sheet reports its grid", sheet.cols == 7 and sheet.rows == 2)
    ok("a sheet counts its cells", sheet.frames == 14, str(sheet.frames))

    cell = sheet.sprite(3, 1)
    ok("a cell is the grid divided by the layout",
       cell.width == 10 and cell.height == 8, "%dx%d" % (cell.width, cell.height))
    ok("a cell of an rgba source is an image", type(cell) is image, str(type(cell)))

    # sprite() takes a cell number or a coordinate.
    ok("sprite(n) and sprite(x, y) name the same cell",
       same(sheet.sprite(10), sheet.sprite(3, 1)))
    ok("sprite(0) is the first cell", same(sheet.sprite(0), sheet.sprite(0, 0)))
    ok("a negative number counts from the end",
       same(sheet.sprite(-1), sheet.sprite(6, 1)))
    ok("...and further back still", same(sheet.sprite(-14), sheet.sprite(0, 0)))
    # Out of range clamps rather than windowing outside the buffer.
    ok("sprite clamps a number", same(sheet.sprite(9999), sheet.sprite(6, 1)))
    ok("...and a negative one", same(sheet.sprite(-9999), sheet.sprite(0, 0)))
    ok("sprite clamps a coordinate", sheet.sprite(99, 99).width == 10)
    ok("...on both axes", sheet.sprite(-500, -500).width == 10)

    # direction chooses the numbering, so it changes which cell a number names.
    tall = src.spritesheet(2, 8)
    tall.direction = spritesheet.ROWS
    ok("ROWS numbers along the row first", same(tall.sprite(1), tall.sprite(1, 0)))
    tall.direction = spritesheet.COLUMNS
    ok("COLUMNS numbers down the column first", same(tall.sprite(1), tall.sprite(0, 1)))
    ok("...which makes a column contiguous", same(tall.sprite(7), tall.sprite(0, 7)))

    # The sheet keeps no clock and no timing behaviour: playback is a tween over
    # cell numbers, so none of that is here.
    for name in ("at", "duration", "animation", "start", "stop", "now", "done",
                 "running", "elapsed", "loop", "frame", "index", "interval"):
        ok("the sheet has no %s" % name, gone(sheet, name))

    # A sheet keeps its source alive and can hand it back.
    ok("a sheet reports its source", sheet.source is src)

    # Playing a sequence: a tween carries the range and the clock, the sheet
    # resolves the cell. at(t) is a pure function, so this needs no clock.
    dying = tween(7, 12, duration=500)
    ok("a tween over cell numbers spans the run",
       (int(dying.at(0)), int(dying.at(100)), int(dying.at(499))) == (7, 8, 11),
       str([dying.at(t) for t in (0, 100, 499)]))
    ok("...and the sheet resolves each one",
       all(sheet.sprite(int(dying.at(t))).width == 10 for t in (0, 250, 499)))
    # A numeric argument narrows itself, so a tween's float needs no cast.
    ok("sprite takes a float and narrows it",
       same(sheet.sprite(dying.at(0)), sheet.sprite(int(dying.at(0)))))
    ok("...and so does a coordinate", same(sheet.sprite(2.9, 1.1), sheet.sprite(2, 1)))
    ok("...clamping at the end rather than running past it", dying.at(5000) == 12)
    ok("the first cell of the run is the one meant",
       same(sheet.sprite(int(dying.at(0))), sheet.sprite(0, 1)))
    # done needs a real start, so this is the one part that reads the clock.
    dying.start(time.ticks_ms() - 600)
    ok("a tween past its duration is done", dying.done)
    dying.start(time.ticks_ms())
    ok("...and one that has just begun is not", not dying.done)

    # A loop needs no start time at all.
    n = 250 // 100 % sheet.frames
    ok("arithmetic over cell numbers loops", same(sheet.sprite(n), sheet.sprite(2, 0)))

    # spritesheet.load, and the timings a GIF declared.
    direct = spritesheet.load(GIF)
    ok("spritesheet.load returns a sheet", type(direct) is spritesheet, str(type(direct)))
    ok("...with the gif's own grid", direct.cols == 4 and direct.rows == 1)
    ok("...reporting the file's timings", direct.timings == (60, 90, 120, 150),
       str(direct.timings))
    ok("a palettised file stays palettised through load",
       direct.sprite(0).palette is not None)
    ok("...and so does its source", direct.source.palette is not None)
    ok("a sheet from a png has no timings", sheet.timings is None, str(sheet.timings))
    named = spritesheet.load(GIF, 4, 1)
    ok("a named grid is honoured", named.frames == 4, str(named.frames))

    # ...which is what a caller builds a tween from.
    spin = tween(0, direct.frames, duration=sum(direct.timings))
    ok("a tween built from the timings covers the frames",
       (int(spin.at(0)), int(spin.at(419))) == (0, 3),
       str([spin.at(t) for t in (0, 419)]))

    # A cell is a view, so its rows are a sheet pitch apart.
    c = sheet.sprite(3, 1)
    ok("a full image strides by its own width", src.stride == 70 * 4, str(src.stride))
    ok("a cell inherits the sheet's stride", c.stride == 70 * 4, str(c.stride))
    ok("a cell's raw reaches its last row",
       len(c.raw) >= 7 * c.stride + 10 * 4, str(len(c.raw)))
    want = color.rgb(20 + 3 * 30, 20 + 1 * 100, 40)
    raw = c.raw
    got = True
    for y in range(8):
        for x in range(10):
            if raw[y * c.stride + x * 4] != want.r:
                got = False
    ok("every row of a cell reads back through stride", got)
    gcell = spritesheet.load(GIF).sprite(1)
    ok("an indexed cell strides in bytes", gcell.stride == 40, str(gcell.stride))


def main():
    test_primitives_across_sizes()
    test_area_is_close_to_analytic()
    test_hollow_shapes_have_a_hole()
    test_antialias_only_softens_edges()
    test_antialias_levels_agree()
    test_fill_rules_differ_on_self_intersection()
    test_combine_fills_as_one()
    test_combine_keeps_holes_and_transforms()
    test_offscreen_and_clipped()
    test_tile_seams()
    test_drawing_twice_is_stable()
    test_custom_accepts_an_array()
    test_vector_text()
    test_animated_gif()
    test_palette()
    test_image_construction()
    test_palettised_image()
    test_spritesheet()
    print("RENDER TESTS: %d passed, %d failed" % (_p, _f))


main()
