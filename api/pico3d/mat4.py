"""mat4 — 3D transform, held as a full 4x4 matrix."""

from __future__ import annotations

from pv import api, cpp, same


@api("mat4_t", field="m", module="pico3d", box="pv::box_mat4({0})",
     arg_read="((mat4_obj_t *)MP_OBJ_TO_PTR({0}))->m", arg_type="mat4_t")
class mat4:
    """3D transform. Builders follow mat3: they modify this matrix and return it,
    so a chain reads in the order it applies, and post-multiply, so
    ``mat4().translate(...).rotate_y(...)`` rotates first and then translates.

    A model matrix places a mesh in the world; a camera is a projection times a
    view, which is what ``surface.render`` takes as its ``view_proj``::

        view_proj = mat4.perspective(60, 320 / 240, 0.1, 100).multiply(
                    mat4.look_at(vec3(0, 2, 6), vec3(0, 0, 0), vec3(0, 1, 0)))
    """

    @cpp(args=["self->m = mat4_t()"])
    def __init__(self):
        "Create an identity transform."

    # ── cameras ─────────────────────────────────────────────────────────────
    @staticmethod
    @cpp(emit="expr", args=["mat4_t().perspective(fov, aspect, near, far)"])
    def perspective(fov: float, aspect: float, near: float, far: float) -> mat4:
        ("A right-handed perspective projection. fov is the vertical field of "
         "view in degrees, aspect is width / height, and near / far bound the "
         "view frustum - anything outside them is not drawn. Keep near as large "
         "as the scene allows: the depth buffer is 16-bit, and its precision is "
         "spent near the camera.")

    @staticmethod
    @cpp(emit="expr", args=["mat4_t().look_at(eye, target, up)"])
    def look_at(eye: vec3, target: vec3, up: vec3) -> mat4:
        ("A view matrix for a camera at eye looking at target, with up naming "
         "which way is up (usually vec3(0, 1, 0)).")

    # ── builders ────────────────────────────────────────────────────────────
    def translate(self, x: float, y: float, z: float) -> Self:
        "Translate by (x, y, z). Modifies and returns this transform."

    def scale(self, x: float, y: float = same("x"), z: float = same("x")) -> Self:
        ("Scale by (x, y, z). Pass one value to scale uniformly. Modifies and "
         "returns this transform.")

    def rotate_x(self, degrees: float) -> Self:
        "Rotate about the x axis by degrees. Modifies and returns this transform."

    def rotate_y(self, degrees: float) -> Self:
        "Rotate about the y axis by degrees. Modifies and returns this transform."

    def rotate_z(self, degrees: float) -> Self:
        "Rotate about the z axis by degrees. Modifies and returns this transform."

    def rotate_x_radians(self, radians: float) -> Self:
        "Rotate about the x axis by radians. Modifies and returns this transform."

    def rotate_y_radians(self, radians: float) -> Self:
        "Rotate about the y axis by radians. Modifies and returns this transform."

    def rotate_z_radians(self, radians: float) -> Self:
        "Rotate about the z axis by radians. Modifies and returns this transform."

    def multiply(self, other: mat4) -> Self:
        "Multiply this transform by another. Modifies and returns this transform."

    # ── transforms ──────────────────────────────────────────────────────────
    @cpp(emit="expr", args=["(self->m * p).project()"])
    def project(self, p: vec3) -> vec3:
        ("Transform a position and divide through by w, giving normalised device "
         "coordinates: x and y in -1..1 across the viewport, z in -1..1 between "
         "the near and far planes. Undefined for a point at or behind the eye.")

    def transform_direction(self, d: vec3) -> vec3:
        ("Transform a direction, ignoring the translation. Correct for normals "
         "under rotation and uniform scale; a non-uniform scale would need the "
         "inverse transpose.")

    @cpp(result="mat4_t(lhs->m).multiply(rhs->m)")
    def __mul__(self, other: mat4) -> mat4:
        "Matrix multiplication, returning a new transform."
