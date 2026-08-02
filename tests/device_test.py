# device_test.py — on-device behavioural tests for the generated picovector
# bindings. Runs under MicroPython on the badge (push with mpremote, see
# tools/run_device_tests.sh). Depends only on the `picovector` module, so it has
# no app/screen side effects.
#
# Each test asserts a behaviour the API is expected to honour. Several are
# regression guards for issues found while bringing the generated bindings up on
# hardware (float args to int params, measure_text without a size, …). The final
# line is "DEVICE TESTS: <p> passed, <f> failed" which the host runner checks.

import picovector as pv
from picovector import vec2, rect, mat3, color, brush, shape, image

_p = 0
_f = 0


def ok(name, cond):
    global _p, _f
    if cond:
        _p += 1
        print("PASS", name)
    else:
        _f += 1
        print("FAIL", name)


def near(a, b, eps=0.01):
    return abs(a - b) < eps


def raises(name, excs, fn):
    try:
        fn()
        ok(name + " (no raise)", False)
    except excs:
        ok(name, True)
    except Exception as e:
        ok(name + " (wrong exc: " + type(e).__name__ + ")", False)


# ── vec2 ─────────────────────────────────────────────────────────────────────
v = vec2(3, 4)
ok("vec2 fields", v.x == 3 and v.y == 4)
ok("vec2 length", near(v.length(), 5))
ok("vec2 dot", near(v.dot(vec2(1, 0)), 3))
ok("vec2 add", (v + vec2(1, 1)).x == 4)
ok("vec2 scalar mul", (v * 2).y == 8)
ok("vec2 eq/ne", vec2(1, 2) == vec2(1, 2) and vec2(1, 2) != vec2(1, 3))
ok("vec2 normalized", near(v.normalized().length(), 1))
ok("vec2 + leaves its operands alone", v.x == 3 and v.y == 4)

# Augmented assignment mutates the receiver and allocates nothing, so an alias
# sees the change. Take a copy with `b = a * 1.0` if that is not wanted.
a = vec2(1, 2)
alias = a
a += vec2(4, 4)
ok("vec2 +=", a.x == 5 and a.y == 6)
ok("vec2 += mutates in place", alias is a and alias.x == 5)
a -= vec2(1, 1)
ok("vec2 -=", a.x == 4 and a.y == 5)
a *= 2
ok("vec2 *= scalar", a.x == 8 and a.y == 10)
a *= vec2(0.5, 0.5)
ok("vec2 *= vec2", a.x == 4 and a.y == 5)
a /= 2
ok("vec2 /= scalar", a.x == 2 and near(a.y, 2.5))
a /= vec2(2, 2)
ok("vec2 /= vec2", a.x == 1 and near(a.y, 1.25))


def _add_scalar():
    # +=/-= take a vec2 only, matching + and -; the scalar forms are *= and /=
    z = vec2(1, 2)
    z += 1.0
    return z


raises("vec2 += float", TypeError, _add_scalar)

# ── mat3 ─────────────────────────────────────────────────────────────────────
m = mat3().translate(5, 0)
p = vec2(1, 1)
p.transform(m)
ok("mat3 translate+vec2.transform", near(p.x, 6) and near(p.y, 1))
s = mat3().scale(2)
q = vec2(3, 4)
q.transform(s)
ok("mat3 uniform scale", near(q.x, 6) and near(q.y, 8))

# ── rect ─────────────────────────────────────────────────────────────────────
r = rect(10, 20, 30, 40)
ok("rect fields", r.x == 10 and r.w == 30)
ok("rect aliases l/t", r.l == 10 and r.t == 20)
ok("rect computed r/b", r.r == 40 and r.b == 60)
ok("rect contains(vec2) inside", rect(0, 0, 10, 10).contains(vec2(5, 5)))
ok("rect contains(vec2) outside", not rect(0, 0, 10, 10).contains(vec2(20, 20)))
ok("rect intersects", rect(0, 0, 10, 10).intersects(rect(5, 5, 10, 10)))

# ── color (incl. float-arg regression) ──────────────────────────────────────
ok("color.rgb int", isinstance(color.rgb(255, 128, 0).p, int))
# REGRESSION: float-valued args (e.g. 100 * pulse) must be accepted, not rejected
ok("color.rgb float alpha", isinstance(color.rgb(0, 0, 0, 100 * 0.85).p, int))
ok("color.hsv", isinstance(color.hsv(180, 200, 200).p, int))
ok("color palette", isinstance(color.red.p, int) and isinstance(color.white.p, int))

# channels resolve whatever the authoring space
ok("color.rgb channels", (lambda c: (c.r, c.g, c.b, c.a) == (200, 100, 50, 255))(color.rgb(200, 100, 50)))
ok("color.rgb alpha", color.rgb(200, 100, 50, 64).a == 64)
ok("color oklch resolves channels", 0 <= color.oklch(70, 40, 250).r <= 255)

# authored components come back exactly, and only in their own space
ok("color.oklch keeps l/c/h", (lambda c: (c.l, c.c, c.h) == (70, 40, 250))(color.oklch(70, 40, 250)))
ok("color.hsv keeps h/s/v", (lambda c: (c.h, c.s, c.v) == (180, 200, 200))(color.hsv(180, 200, 200)))
ok("color.space", (color.rgb(1, 2, 3).space, color.hsv(1, 2, 3).space,
                   color.oklch(1, 2, 3).space) == ("rgb", "hsv", "oklch"))
raises("color.l on an rgb colour raises", AttributeError, lambda: color.rgb(1, 2, 3).l)
raises("color.h on an rgb colour raises", AttributeError, lambda: color.rgb(1, 2, 3).h)
raises("color.s on an oklch colour raises", AttributeError, lambda: color.oklch(1, 2, 3).s)

# immutable: the palette lives in read-only storage, so no property may be set
raises("color channels are read-only", AttributeError,
       lambda: setattr(color.rgb(1, 2, 3), "r", 9))
raises("palette entries are read-only", AttributeError, lambda: setattr(color.red, "r", 9))

# arithmetic returns a new colour and leaves the receiver alone
BG = color.oklch(40, 30, 250)
ok("color.lighten moves l only", (lambda c: (c.l, c.c, c.h) == (50, 30, 250))(BG.lighten(10)))
ok("color.lighten leaves the original", BG.l == 40)
ok("color.darken", BG.darken(10).l == 30)
ok("color.scale", BG.scale(50).l == 20)
ok("color.with_alpha", BG.with_alpha(128).a == 128 and BG.with_alpha(128).l == 40)
ok("color.with_l", BG.with_l(200).l == 200)
ok("color.mix endpoints", BG.mix(color.oklch(80, 30, 250), 0) == BG)
ok("color.mix midpoint", BG.mix(color.oklch(80, 30, 250), 128).l == 60)
ok("color.over an opaque background", color.red.with_alpha(0).over(color.blue) == color.blue)
ok("color.over composites", 0 <= color.red.with_alpha(128).over(color.blue).r <= 255)

# operators
ok("color + int lightens", (BG + 10).l == 50)
ok("color - int darkens", (BG - 10).l == 30)
ok("color * float scales", (BG * 0.5).l == 20)

# value semantics: equal colours compare equal and are one dict key
ok("color == by value", color.rgb(10, 20, 30) == color.rgb(10, 20, 30))
ok("color != by value", color.rgb(10, 20, 30) != color.rgb(10, 20, 31))
_o = color.oklch(70, 40, 250)
ok("color == across spaces when it renders the same", _o == color.rgb(_o.r, _o.g, _o.b, _o.a))
ok("color hash agrees with ==", hash(color.rgb(10, 20, 30)) == hash(color.rgb(10, 20, 30)))
ok("color is one dict key", len({color.rgb(10, 20, 30): 1, color.rgb(10, 20, 30): 2}) == 1)
ok("colour keys survive reconstruction",
   {(1, color.rgb(10, 20, 30)): "hit"}.get((1, color.rgb(10, 20, 30))) == "hit")

# repr reads back as the call that would rebuild it
ok("color repr", str(color.oklch(70, 40, 250)) == "color.oklch(70, 40, 250, 255)")
ok("color repr rgb", str(color.rgb(200, 100, 50)) == "color.rgb(200, 100, 50, 255)")

# ── shape factories + overloads ─────────────────────────────────────────────
ok("shape.circle(x, y, r)", shape.circle(8, 8, 4).bounds().w > 0)
ok("shape.circle(vec2, r)  [XY overload]", shape.circle(vec2(8, 8), 4).bounds().w > 0)
ok("shape.rectangle(x, y, w, h)", shape.rectangle(0, 0, 10, 10).bounds().w > 0)
ok("shape.rectangle(rect)  [XYWH overload]", shape.rectangle(rect(0, 0, 10, 10)).bounds().w > 0)
ok("shape.arc (arg reorder)", shape.arc(8, 8, 2, 6, 0, 180).bounds().w > 0)
ok("shape.star (arg count)", shape.star(vec2(8, 8), 5, 8, 4).bounds().w > 0)
ok("shape.squircle (arg count)", shape.squircle(vec2(8, 8), 6).bounds().w > 0)
ok("shape.custom(points)", shape.custom([vec2(0, 0), vec2(10, 0), vec2(5, 10)]).bounds().w > 0)
ok("shape.stroke() returns shape", shape.circle(8, 8, 4).stroke(2).bounds().w > 0)

# ── image: raster + float args + draw + overloads ────────────────────────────
img = image(32, 32)
ok("image size", img.width == 32 and img.height == 32)
img.pen = color.rgb(255, 0, 0)
img.clear()
# REGRESSION: float w/h to a raster primitive
img.rectangle(1, 1, 4.0, 4.0)
ok("image.rectangle(float)", True)
img.line(vec2(0, 0), vec2(10, 10))     # vec2 form
img.line(0, 0, 5, 5)                   # x,y,x,y form
ok("image.line overloads", True)
img.circle(vec2(8, 8), 4)
img.circle(16, 16, 3)
ok("image.circle overloads", True)
img.put(vec2(2, 2))
got = img.get(vec2(2, 2))
ok("image.get returns color", isinstance(got.p, int))
img.shape(shape.circle(16, 16, 6))
img.shape([shape.circle(4, 4, 2), shape.rectangle(0, 0, 4, 4)])  # list form
ok("image.shape + list", True)
ok("image props antialias/fill_rule/alpha",
   (setattr(img, "antialias", img.X2), setattr(img, "fill_rule", img.NON_ZERO),
    setattr(img, "alpha", 200), img.alpha == 200)[-1])

# blit overloads (all four forms must dispatch)
src = image(8, 8)
src.pen = color.blue
src.clear()
img.blit(src, vec2(0, 0))
img.blit(src, 2, 2)
img.blit(src, rect(0, 0, 8, 8))
img.blit(src, rect(0, 0, 4, 4), rect(0, 0, 8, 8))
ok("image.blit (4 overloads)", True)

# REGRESSION: measure_text(text) with no size must reach the body (font check),
# i.e. raise OSError "no font" — NOT TypeError "missing required arguments".
raises("image.measure_text(text) arg count", OSError, lambda: image(8, 8).measure_text("hi"))
raises("image.text(...) needs a font", OSError, lambda: image(8, 8).text("hi", vec2(0, 0)))

# ── brush ────────────────────────────────────────────────────────────────────
img.pen = brush.gradient(brush.LINEAR, 0, 0, 1, 0, [(0.0, color.black), (1.0, color.white)])
ok("brush.gradient + image.pen=brush", True)
img.pen = brush.gradient(brush.CONICAL, 16, 16, 16, 0, [(0.0, color.black), (0.75, color.white)])
ok("brush.gradient CONICAL", True)
# geometry() moves a gradient without rebuilding its table
_g = brush.gradient(brush.LINEAR, 0, 0, 32, 0, [(0.0, color.black), (1.0, color.white)])
_g.geometry(32, 0, 0, 0)
ok("brush.geometry on a gradient", True)
raises("brush.geometry on a non-gradient", TypeError, lambda: brush.blur(2).geometry(0, 0, 1, 1))
img.pen = brush.pixelate(2)
img.pen = brush.lighten(40)
ok("brush effects assignable to pen", True)
# pattern index is range-checked (0..37)
raises("brush.pattern index range", (ValueError, TypeError),
       lambda: brush.pattern(color.red, color.black, 99))
ok("brush.pattern(index)", brush.pattern(color.red, color.black, 3) is not None)
ok("brush.pattern(tuple[8])",
   brush.pattern(color.red, color.black, (1, 2, 4, 8, 16, 32, 64, 128)) is not None)

print("DEVICE TESTS:", _p, "passed,", _f, "failed")
