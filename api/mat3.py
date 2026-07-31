"""mat3 — 2D affine transform, held as the top two rows of a 3×3 matrix."""

from __future__ import annotations

from pv import api, cpp, same, XY


@api("mat3_t", field="m", arg_read="((mat3_obj_t *)MP_OBJ_TO_PTR({0}))->m",
     arg_type="mat3_t")
class mat3:
    """2D affine transform. The third row of the matrix is always 0, 0, 1."""

    @cpp(args=["self->m = mat3_t()"])
    def __init__(self):
        "Create an identity transform."

    @staticmethod
    @cpp(call="mat3_t::trs", emit="free", args="t.x t.y degrees scale scale")
    def trs(t: XY, degrees: float, scale: float = 1.0) -> mat3:
        "Translate, rotate and scale in one step. Same result as "
        "mat3().translate(t).rotate(degrees).scale(scale), in one allocation "
        "instead of four."

    def rotate(self, degrees: float) -> Self:
        "Rotate by degrees. Modifies and returns this transform."

    def rotate_radians(self, radians: float) -> Self:
        "Rotate by radians. Modifies and returns this transform."

    @cpp(args="p.x p.y")
    def translate(self, p: XY) -> Self:
        "Translate by (x, y). Also accepts a single vec2. Modifies and returns "
        "this transform."

    def scale(self, x: float, y: float = same("x")) -> Self:
        "Scale by (x, y). Pass one value to scale uniformly. Modifies and returns "
        "this transform."

    def multiply(self, other: mat3) -> Self:
        "Multiply this transform by another. Modifies and returns this transform."

    @cpp(emit="expr", args="mat3_t(self->m).inverse()")
    def inverse(self) -> mat3:
        "Return the inverse as a new transform, leaving this one unchanged."

    @cpp(result="mat3_t(lhs->m).multiply(rhs->m)")
    def __mul__(self, other: mat3) -> mat3:
        "Matrix multiplication, returning a new transform."
