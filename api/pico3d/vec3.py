"""vec3 — 3D vector / point."""

from __future__ import annotations

from pv import api


@api("vec3_t", field="v", module="pico3d", box="pv::box_vec3({0})",
     arg_read="((vec3_obj_t *)MP_OBJ_TO_PTR({0}))->v", arg_type="vec3_t",
     print=("vec3(%f, %f, %f)", "self->v.x", "self->v.y", "self->v.z"))
class vec3:
    """3D vector / point. Positions, directions and axes all use this."""

    x: float
    y: float
    z: float

    def __init__(self, x: float = 0, y: float = 0, z: float = 0):
        "vec3() or vec3(x, y, z)."

    def length(self) -> float:
        "Length of the vector."

    def length_squared(self) -> float:
        ("Length squared, which needs no square root - use it to compare two "
         "distances or to test one against a radius.")

    def normalized(self) -> vec3:
        ("A unit-length copy, pointing the same way. A zero vector normalises to "
         "zero rather than to NaN, so a degenerate direction stays harmless.")

    def dot(self, other: vec3) -> float:
        "Dot product. For two unit vectors, the cosine of the angle between them."

    def cross(self, other: vec3) -> vec3:
        "Cross product: a vector perpendicular to both, right-handed."

    def lerp(self, other: vec3, t: float) -> vec3:
        "Linear interpolation towards other, t from 0 to 1. Not clamped."

    # operators (a new vec3; the original is unchanged) ----------------------
    def __add__(self, other: vec3) -> vec3: "Component-wise addition."
    def __sub__(self, other: vec3) -> vec3: "Component-wise subtraction."
    def __mul__(self, other: float) -> vec3: "Scale by a number."
    def __truediv__(self, other: float) -> vec3: "Divide by a number."
    def __eq__(self, other: vec3) -> bool: "Equality."
    def __ne__(self, other: vec3) -> bool: "Inequality."

    # augmented assignment (mutates and returns self; no allocation) ---------
    def __iadd__(self, other: vec3) -> vec3: "Add another vec3 into this one."
    def __isub__(self, other: vec3) -> vec3: "Subtract another vec3 from this one."
    def __imul__(self, other: float) -> vec3: "Scale this one by a number."
