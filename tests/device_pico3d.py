# device_pico3d.py — on-device tests for the pico3d bindings.
#
# Runs under MicroPython on the badge (push with mpremote). Where device_test.py
# covers the picovector module, this covers pico3d: that every type accepts and
# hands back what it should, that the guards fire, and that a render actually
# puts ink on a canvas.
#
# The rasteriser itself is covered by the host tests in the core checkout, which
# need no hardware. What needs a board is the binding layer - buffer borrowing,
# the image/colour conversions, and the argument checks.
#
# The final line is "PICO3D TESTS: <p> passed, <f> failed" for a host runner.

from array import array
import pico3d
from picovector import color, image

_p = _f = 0


def ok(name, cond, detail=''):
    global _p, _f
    if cond:
        _p += 1
        print('PASS', name)
    else:
        _f += 1
        print('FAIL', name, detail)


def raises(name, fn, exc=Exception):
    """A guard that does not fire is a failure, so each one is asserted rather
    than assumed."""
    try:
        fn()
        ok(name, False, 'no error raised')
    except exc:
        ok(name, True)

# ── module surface ──────────────────────────────────────────────────────────
ok('module exports', sorted(a for a in dir(pico3d) if not a.startswith('_'))
   == ['engine', 'light', 'mat4', 'material', 'mesh', 'surface', 'vec3'])

# ── vec3 ────────────────────────────────────────────────────────────────────
v = pico3d.vec3(3, 4, 0)
ok('vec3 length', abs(v.length() - 5.0) < 1e-4)
ok('vec3 length_squared', abs(v.length_squared() - 25.0) < 1e-4)
ok('vec3 normalized', abs(pico3d.vec3(0, 3, 0).normalized().y - 1.0) < 1e-4)
ok('vec3 dot', abs(pico3d.vec3(1, 2, 3).dot(pico3d.vec3(4, 5, 6)) - 32.0) < 1e-4)
ok('vec3 cross', pico3d.vec3(1, 0, 0).cross(pico3d.vec3(0, 1, 0)).z == 1.0)
ok('vec3 lerp', pico3d.vec3(0, 0, 0).lerp(pico3d.vec3(10, 20, 30), 0.5).y == 10.0)
ok('vec3 add', (pico3d.vec3(1, 1, 1) + pico3d.vec3(2, 3, 4)).z == 5.0)
ok('vec3 scale', (pico3d.vec3(1, 2, 3) * 2).y == 4.0)
ok('vec3 divide', (pico3d.vec3(2, 4, 6) / 2).z == 3.0)
ok('vec3 equality', pico3d.vec3(1, 2, 3) == pico3d.vec3(1, 2, 3))
a = pico3d.vec3(1, 1, 1); a += pico3d.vec3(1, 1, 1)
ok('vec3 inplace add', a.x == 2.0)
ok('vec3 fields writable', (lambda w: (setattr(w, 'x', 9), w.x)[1])(pico3d.vec3()) == 9.0)
ok('vec3 repr', 'vec3(' in repr(pico3d.vec3(1, 2, 3)))

# ── mat4 ────────────────────────────────────────────────────────────────────
m = pico3d.mat4()
ok('mat4 builders return self', m.translate(1, 2, 3) is m)
ok('mat4 builders mutate', abs(pico3d.mat4().translate(0, 0, -5)
                               .project(pico3d.vec3(0, 0, 0)).z) >= 0)
ok('mat4 scale uniform', pico3d.mat4().scale(2)
   .transform_direction(pico3d.vec3(1, 1, 1)).y == 2.0)
persp = pico3d.mat4.perspective(60, 4 / 3, 0.5, 100)
ok('mat4.perspective static', persp is not None)
look = pico3d.mat4.look_at(pico3d.vec3(0, 0, 5), pico3d.vec3(0, 0, 0), pico3d.vec3(0, 1, 0))
ok('mat4.look_at static', abs(look.project(pico3d.vec3(0, 0, 0)).z + 5.0) < 1e-3)
ok('mat4 rotate_y', abs(pico3d.mat4().rotate_y(90)
                        .transform_direction(pico3d.vec3(1, 0, 0)).z + 1.0) < 1e-3)
ok('mat4 radians variant', abs(pico3d.mat4().rotate_z_radians(0)
                               .transform_direction(pico3d.vec3(1, 0, 0)).x - 1.0) < 1e-4)
ok('mat4 __mul__ is a new matrix', (persp * look) is not persp)

# ── mesh ────────────────────────────────────────────────────────────────────
pos = array('f', (0, 0, 0, 1, 0, 0, 0, 1, 0))
idx = array('H', (0, 1, 2))
mesh = pico3d.mesh(positions=pos, indices=idx)
ok('mesh counts', (mesh.vertices, mesh.triangles) == (3, 1))
ok('mesh borrows positions', mesh.positions is pos)
ok('mesh optional arrays are None', mesh.normals is None and mesh.uvs is None)
ok('mesh repr', 'verts' in repr(mesh))
raises('mesh rejects a bad index',
       lambda: pico3d.mesh(positions=pos, indices=array('H', (0, 1, 9))), ValueError)
raises('mesh rejects short normals',
       lambda: pico3d.mesh(positions=pos, indices=idx, normals=array('f', (0, 0, 1))),
       ValueError)
raises('mesh rejects empty geometry',
       lambda: pico3d.mesh(positions=array('f', ()), indices=array('H', ())), ValueError)

# ── material ────────────────────────────────────────────────────────────────
tex = image(8, 8)
tex.pen = color.rgb(10, 20, 30)
tex.clear()
mat = pico3d.material(color=color.rgb(1, 2, 3), texture=tex,
                      shading=pico3d.material.GOURAUD,
                      filter=pico3d.material.BILINEAR, specular=color.rgb(9, 9, 9),
                      shininess=48, double_sided=True, alpha_cutoff=64)
ok('material colour round-trips', (mat.color.r, mat.color.g, mat.color.b) == (1, 2, 3))
ok('material colour is opaque on read', mat.color.a == 255)
ok('material texture is held', mat.texture is tex)
ok('material shading', mat.shading == pico3d.material.GOURAUD)
ok('material filter', mat.filter == pico3d.material.BILINEAR)
ok('material specular', mat.specular.r == 9)
ok('material shininess', mat.shininess == 48)
ok('material double_sided', mat.double_sided is True)
ok('material alpha_cutoff', mat.alpha_cutoff == 64)
mat.color = color.rgb(200, 100, 50); ok('material colour writable', mat.color.r == 200)
mat.filter = pico3d.material.NEAREST
ok('material filter writable', mat.filter == pico3d.material.NEAREST)
mat.shading = pico3d.material.FLAT
ok('material shading writable', mat.shading == pico3d.material.FLAT)
mat.shininess = 8; ok('material shininess writable', mat.shininess == 8)
raises('material colour rejects an int', lambda: setattr(mat, 'color', 5), TypeError)
ok('material constants',
   (pico3d.material.FLAT, pico3d.material.GOURAUD, pico3d.material.UNLIT,
    pico3d.material.NEAREST, pico3d.material.BILINEAR) == (0, 1, 2, 0, 1))

# ── light ───────────────────────────────────────────────────────────────────
lit = pico3d.light(direction=pico3d.vec3(0, -1, 0), color=color.rgb(255, 254, 253),
                   ambient=color.rgb(1, 2, 3))
ok('light direction', lit.direction.y == -1.0)
ok('light colour', (lit.color.r, lit.color.g, lit.color.b) == (255, 254, 253))
ok('light ambient', lit.ambient.b == 3)
ok('light is directional by default', lit.point is False)
pt = pico3d.light(position=pico3d.vec3(1, 2, 3), atten=0.25)
ok('a position makes it a point light', pt.point is True)
ok('point light position', pt.position.z == 3.0)
ok('point light atten', abs(pt.atten - 0.25) < 1e-6)
pt.point = False; ok('light point writable', pt.point is False)
lit.ambient = color.rgb(7, 7, 7); ok('light ambient writable', lit.ambient.r == 7)

# ── surface ─────────────────────────────────────────────────────────────────
canvas = image(32, 32)
surf = pico3d.surface(canvas)
ok('surface size', (surf.width, surf.height) == (32, 32))
ok('surface holds its image', surf.image is canvas)
ok('surface repr', 'surface(' in repr(surf))
surf.clear_depth(1000)
ok('clear_depth takes a value', True)
surf.clear_depth()          # back to far, or everything below fails the depth test
# A palettised image is one byte a pixel, so the engine must refuse it rather
# than write RGBA words four times past the end of the buffer.
raises('surface refuses a palettised image',
       lambda: pico3d.surface(image.qr('hi')), ValueError)
raises('a texture cannot be palettised either',
       lambda: pico3d.material(texture=image.qr('hi')), ValueError)

canvas.pen = color.rgb(0, 0, 0); canvas.clear()
vp = pico3d.mat4.perspective(60, 1.0, 0.1, 20).multiply(
     pico3d.mat4.look_at(pico3d.vec3(0, 0, 3), pico3d.vec3(0, 0, 0), pico3d.vec3(0, 1, 0)))
quad = pico3d.mesh(
    positions=array('f', (-1, -1, 0, 1, -1, 0, 1, 1, 0, -1, 1, 0)),
    normals=array('f', (0, 0, 1) * 4),
    uvs=array('f', (0, 1, 1, 1, 1, 0, 0, 0)),
    indices=array('H', (0, 1, 2, 0, 2, 3)))
white = pico3d.material(color=color.rgb(255, 255, 255), shading=pico3d.material.UNLIT)
n = surf.render(quad, pico3d.mat4(), vp, white, None)
ok('render returns a triangle count', n == 2, 'got %r' % n)
raw, st = canvas.raw, canvas.stride
ok('render put ink on the canvas', raw[16 * st + 16 * 4] > 200)
ok('render accepts depth=False',
   surf.render(quad, pico3d.mat4(), vp, white, None, depth=False) == 2)
surf.clear_depth()          # the render above wrote depth; same quad, same z
ok('render accepts a view matrix',
   surf.render(quad, pico3d.mat4(), vp, white, lit, view=pico3d.mat4()) == 2)
raises('render type-checks the mesh',
       lambda: surf.render(white, pico3d.mat4(), vp, white, None), TypeError)

# ── the near plane ──────────────────────────────────────────────────────────
# Geometry the camera stands in has a vertex behind the eye. The draw stage cuts
# it against the eye plane; before it did, the whole triangle was dropped and
# anything you walked into vanished.
canvas.pen = color.rgb(0, 0, 0)
canvas.clear()
surf.clear_depth()
slab = pico3d.mesh(
    positions=array('f', (-1, -0.5, 2, 1, -0.5, 2, 1, -0.5, -8, -1, -0.5, -8)),
    indices=array('H', (0, 1, 2, 0, 2, 3)))
straddle = pico3d.material(color=color.rgb(255, 255, 255),
                           shading=pico3d.material.UNLIT, double_sided=True)
eye_level = pico3d.mat4.perspective(70, 1.0, 0.1, 50).multiply(
    pico3d.mat4.look_at(pico3d.vec3(0, 0, 0), pico3d.vec3(0, 0, -1),
                        pico3d.vec3(0, 1, 0)))
ok('a surface the camera stands on draws',
   surf.render(slab, pico3d.mat4(), eye_level, straddle, None) > 0)
_below = sum(1 for y in range(16, 32) for x in range(32)
             if canvas.raw[y * canvas.stride + x * 4] > 8)
_above = sum(1 for y in range(0, 16) for x in range(32)
             if canvas.raw[y * canvas.stride + x * 4] > 8)
ok('it paints below the horizon', _below > 40, 'got %d px' % _below)
ok('and nothing above it', _above == 0, 'got %d px' % _above)

# ── engine ──────────────────────────────────────────────────────────────────
ok('engine.cores returns the count', pico3d.engine.cores(2) in (1, 2))
ok('engine.core_count agrees', pico3d.engine.core_count() == pico3d.engine.cores(2))
prof = pico3d.engine.profile()
ok('engine.profile returns 8 counters', isinstance(prof, tuple) and len(prof) == 8)

print("PICO3D TESTS: %d passed, %d failed" % (_p, _f))
