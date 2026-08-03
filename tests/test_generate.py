"""test_generate.py — spot-check that the generator emits the expected C++ for a
few representative members (scalar return, arg reordering, overload dispatch,
optional argument + boxing). Complements the surface-parity and compile checks.

Run:  python tests/test_generate.py
"""

from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, ROOT)

import model            # noqa: E402
import generate         # noqa: E402

CHECKS = {
    "vec2": [
        # scalar method: direct call, float-boxed return
        "return mp_obj_new_float(self->v.dot(other));",
        # binary op switch has a default (the -Werror=switch fix)
        "default: break;",
        # inplace op is a statement on lhs, returning the receiver unboxed
        "case MP_BINARY_OP_INPLACE_ADD: {",
        "lhs->v += rhs->v;",
        "return lhs_in;",
    ],
    "shape": [
        # arc reorders Python (inner, outer, from, to) -> C++ (from, to, inner, outer)
        "arc(c.x, c.y, from_a, to_a, inner, outer)",
        # custom builds the shape from a path list and boxes it
        "pv::box_shape(paths_shape)",
    ],
    "color": [
        # optional alpha defaults to 255; colour boxed by value (no leaky `new`)
        "int a = 255;",
        "pv::box_color(rgb_color_t(r, g, b, a))",
    ],
    "image": [
        # blit overloads: n_args checked first, then arg types; receiver is `src`.
        # A source is AnyImage, so the check is the helper that takes either
        # image type, not a single mp_obj_is_type.
        "if (n_args == 3 && pv::is_image(args[1]) "
        "&& mp_obj_is_type(args[2], &type_vec2)) {",
        "src->blit(self->image, source, dst, filter)",
        # the span blits read their source through the same helper, which is
        # where their (previously absent) type check comes from
        "image_t * src = pv::get_image(args[_i]); _i++;",
        # a read-only property breaks rather than falling into the next setter
        "if (action == GET) { dest[0] = mp_obj_new_int(self->image->bounds().w); return; }\n      break;",
        # optional metrics guard injected (compiled away unless PV_METRICS)
        "#if PV_METRICS",
        "pv::metric_scope _pvm(PV_M_image_circle);",
    ],
    "spritesheet": [
        # the arithmetic is the core value's; the binding only forwards
        "return mp_obj_new_int(self->sheet.index_at(ms));",
        "self->sheet.stop();",
        # range() takes keyword arguments, so its body parses its own kw map
        "static MP_DEFINE_CONST_FUN_OBJ_KW(mpy_spritesheet_range_obj, 1, spritesheet_range);",
        # now boxes a view, so the attr getter calls out to the native side
        "dest[0] = spritesheet_box_now(self); return;",
    ],
    "indexed_image": [
        # the read-only half: no setter for anything but alpha, and none of the
        # drawing surface. The fall-through is what reports the absence.
        "dest[1] = MP_OBJ_SENTINEL;",
        "return pv::box_color_packed(self->image->get(p.x, p.y));",
    ],
    "brush": [
        # pixelate clamps size at 1 (Range(1, None, clamp=True))
        "if (size < 1) size = 1;",
    ],
}


def main():
    types = {t.name: t for t in model.load()}
    ok = True
    for name, needles in CHECKS.items():
        src = generate.emit_type_file(types[name])
        for needle in needles:
            if needle in src:
                print(f"✓ {name}: {needle[:60]}")
            else:
                print(f"✗ {name}: MISSING {needle!r}")
                ok = False
    print()
    print("GENERATOR OK" if ok else "GENERATOR CHECK FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
