// AUTO-GENERATED from api/pico3d/mat4.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// mat4.perspective: A right-handed perspective projection. fov is the vertical field of view in degrees, aspect is width / height, and near / far bound the view frustum - anything outside them is not drawn. Keep near as large as the scene allows: the depth buffer is 16-bit, and its precision is spent near the camera.
mp_obj_t mpy_mat4_perspective(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_perspective);
#endif
  size_t _i = 0;
  float fov = mp_obj_get_float(args[_i]); _i++;
  float aspect = mp_obj_get_float(args[_i]); _i++;
  float near = mp_obj_get_float(args[_i]); _i++;
  float far = mp_obj_get_float(args[_i]); _i++;
  return pv::box_mat4(mat4_t().perspective(fov, aspect, near, far));
}

// mat4.look_at: A view matrix for a camera at eye looking at target, with up naming which way is up (usually vec3(0, 1, 0)).
mp_obj_t mpy_mat4_look_at(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_look_at);
#endif
  size_t _i = 0;
  vec3_t eye = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  vec3_t target = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  vec3_t up = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  return pv::box_mat4(mat4_t().look_at(eye, target, up));
}

// mat4.translate: Translate by (x, y, z). Modifies and returns this transform.
mp_obj_t mpy_mat4_translate(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_translate);
#endif
  size_t _i = 1;
  float x = mp_obj_get_float(args[_i]); _i++;
  float y = mp_obj_get_float(args[_i]); _i++;
  float z = mp_obj_get_float(args[_i]); _i++;
  self->m.translate(x, y, z);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.scale: Scale by (x, y, z). Pass one value to scale uniformly. Modifies and returns this transform.
mp_obj_t mpy_mat4_scale(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_scale);
#endif
  size_t _i = 1;
  float x = mp_obj_get_float(args[_i]); _i++;
  float y = x;
  if (n_args > _i) { y = mp_obj_get_float(args[_i]); _i++; }
  float z = x;
  if (n_args > _i) { z = mp_obj_get_float(args[_i]); _i++; }
  self->m.scale(x, y, z);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.rotate_x: Rotate about the x axis by degrees. Modifies and returns this transform.
mp_obj_t mpy_mat4_rotate_x(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_rotate_x);
#endif
  size_t _i = 1;
  float degrees = mp_obj_get_float(args[_i]); _i++;
  self->m.rotate_x(degrees);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.rotate_y: Rotate about the y axis by degrees. Modifies and returns this transform.
mp_obj_t mpy_mat4_rotate_y(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_rotate_y);
#endif
  size_t _i = 1;
  float degrees = mp_obj_get_float(args[_i]); _i++;
  self->m.rotate_y(degrees);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.rotate_z: Rotate about the z axis by degrees. Modifies and returns this transform.
mp_obj_t mpy_mat4_rotate_z(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_rotate_z);
#endif
  size_t _i = 1;
  float degrees = mp_obj_get_float(args[_i]); _i++;
  self->m.rotate_z(degrees);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.rotate_x_radians: Rotate about the x axis by radians. Modifies and returns this transform.
mp_obj_t mpy_mat4_rotate_x_radians(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_rotate_x_radians);
#endif
  size_t _i = 1;
  float radians = mp_obj_get_float(args[_i]); _i++;
  self->m.rotate_x_radians(radians);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.rotate_y_radians: Rotate about the y axis by radians. Modifies and returns this transform.
mp_obj_t mpy_mat4_rotate_y_radians(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_rotate_y_radians);
#endif
  size_t _i = 1;
  float radians = mp_obj_get_float(args[_i]); _i++;
  self->m.rotate_y_radians(radians);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.rotate_z_radians: Rotate about the z axis by radians. Modifies and returns this transform.
mp_obj_t mpy_mat4_rotate_z_radians(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_rotate_z_radians);
#endif
  size_t _i = 1;
  float radians = mp_obj_get_float(args[_i]); _i++;
  self->m.rotate_z_radians(radians);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.multiply: Multiply this transform by another. Modifies and returns this transform.
mp_obj_t mpy_mat4_multiply(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_multiply);
#endif
  size_t _i = 1;
  mat4_t other = ((mat4_obj_t *)MP_OBJ_TO_PTR(args[_i]))->m; _i++;
  self->m.multiply(other);
  return MP_OBJ_FROM_PTR(self);
}

// mat4.project: Transform a position and divide through by w, giving normalised device coordinates: x and y in -1..1 across the viewport, z in -1..1 between the near and far planes. Undefined for a point at or behind the eye.
mp_obj_t mpy_mat4_project(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_project);
#endif
  size_t _i = 1;
  vec3_t p = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  return pv::box_vec3((self->m * p).project());
}

// mat4.transform_direction: Transform a direction, ignoring the translation. Correct for normals under rotation and uniform scale; a non-uniform scale would need the inverse transpose.
mp_obj_t mpy_mat4_transform_direction(size_t n_args, const mp_obj_t *args) {
  self(args[0], mat4_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_mat4_transform_direction);
#endif
  size_t _i = 1;
  vec3_t d = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  return pv::box_vec3(self->m.transform_direction(d));
}

static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_perspective_obj, 4, mpy_mat4_perspective);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_mat4_perspective_static_obj, MP_ROM_PTR(&mpy_mat4_perspective_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_look_at_obj, 3, mpy_mat4_look_at);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_mat4_look_at_static_obj, MP_ROM_PTR(&mpy_mat4_look_at_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_translate_obj, 4, mpy_mat4_translate);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_scale_obj, 2, mpy_mat4_scale);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_rotate_x_obj, 2, mpy_mat4_rotate_x);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_rotate_y_obj, 2, mpy_mat4_rotate_y);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_rotate_z_obj, 2, mpy_mat4_rotate_z);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_rotate_x_radians_obj, 2, mpy_mat4_rotate_x_radians);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_rotate_y_radians_obj, 2, mpy_mat4_rotate_y_radians);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_rotate_z_radians_obj, 2, mpy_mat4_rotate_z_radians);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_multiply_obj, 2, mpy_mat4_multiply);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_project_obj, 2, mpy_mat4_project);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_mat4_transform_direction_obj, 2, mpy_mat4_transform_direction);

static mp_obj_t mat4_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  mat4_obj_t *self = mp_obj_malloc(mat4_obj_t, type);
  self->m = mat4_t();
  return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mat4_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
  mat4_obj_t *lhs = (mat4_obj_t *)MP_OBJ_TO_PTR(lhs_in);
  switch (op) {
    case MP_BINARY_OP_MULTIPLY: {
      if (mp_obj_is_type(rhs_in, &type_mat4)) {
        mat4_obj_t *rhs = (mat4_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return pv::box_mat4(mat4_t(lhs->m).multiply(rhs->m));
      }
    } break;
    default: break;  // unhandled ops fall through to MP_OBJ_NULL
  }
  return MP_OBJ_NULL;
}

static const mp_rom_map_elem_t mat4_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_perspective), MP_ROM_PTR(&mpy_mat4_perspective_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_look_at), MP_ROM_PTR(&mpy_mat4_look_at_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_translate), MP_ROM_PTR(&mpy_mat4_translate_obj) },
  { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_PTR(&mpy_mat4_scale_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate_x), MP_ROM_PTR(&mpy_mat4_rotate_x_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate_y), MP_ROM_PTR(&mpy_mat4_rotate_y_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate_z), MP_ROM_PTR(&mpy_mat4_rotate_z_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate_x_radians), MP_ROM_PTR(&mpy_mat4_rotate_x_radians_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate_y_radians), MP_ROM_PTR(&mpy_mat4_rotate_y_radians_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate_z_radians), MP_ROM_PTR(&mpy_mat4_rotate_z_radians_obj) },
  { MP_ROM_QSTR(MP_QSTR_multiply), MP_ROM_PTR(&mpy_mat4_multiply_obj) },
  { MP_ROM_QSTR(MP_QSTR_project), MP_ROM_PTR(&mpy_mat4_project_obj) },
  { MP_ROM_QSTR(MP_QSTR_transform_direction), MP_ROM_PTR(&mpy_mat4_transform_direction_obj) },
};
static MP_DEFINE_CONST_DICT(mat4_locals_dict, mat4_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_mat4,
  MP_QSTR_mat4,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)mat4_make_new,
  binary_op, (const void *)mat4_binary_op,
  locals_dict, &mat4_locals_dict
);

}
