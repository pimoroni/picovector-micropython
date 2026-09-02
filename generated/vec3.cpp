// AUTO-GENERATED from api/pico3d/vec3.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// vec3.length: Length of the vector.
mp_obj_t mpy_vec3_length(size_t n_args, const mp_obj_t *args) {
  self(args[0], vec3_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_vec3_length);
#endif
  return mp_obj_new_float(self->v.length());
}

// vec3.length_squared: Length squared, which needs no square root - use it to compare two distances or to test one against a radius.
mp_obj_t mpy_vec3_length_squared(size_t n_args, const mp_obj_t *args) {
  self(args[0], vec3_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_vec3_length_squared);
#endif
  return mp_obj_new_float(self->v.length_squared());
}

// vec3.normalized: A unit-length copy, pointing the same way. A zero vector normalises to zero rather than to NaN, so a degenerate direction stays harmless.
mp_obj_t mpy_vec3_normalized(size_t n_args, const mp_obj_t *args) {
  self(args[0], vec3_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_vec3_normalized);
#endif
  return pv::box_vec3(self->v.normalized());
}

// vec3.dot: Dot product. For two unit vectors, the cosine of the angle between them.
mp_obj_t mpy_vec3_dot(size_t n_args, const mp_obj_t *args) {
  self(args[0], vec3_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_vec3_dot);
#endif
  size_t _i = 1;
  vec3_t other = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  return mp_obj_new_float(self->v.dot(other));
}

// vec3.cross: Cross product: a vector perpendicular to both, right-handed.
mp_obj_t mpy_vec3_cross(size_t n_args, const mp_obj_t *args) {
  self(args[0], vec3_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_vec3_cross);
#endif
  size_t _i = 1;
  vec3_t other = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  return pv::box_vec3(self->v.cross(other));
}

// vec3.lerp: Linear interpolation towards other, t from 0 to 1. Not clamped.
mp_obj_t mpy_vec3_lerp(size_t n_args, const mp_obj_t *args) {
  self(args[0], vec3_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_vec3_lerp);
#endif
  size_t _i = 1;
  vec3_t other = ((vec3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->v; _i++;
  float t = mp_obj_get_float(args[_i]); _i++;
  return pv::box_vec3(self->v.lerp(other, t));
}

static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_vec3_length_obj, 1, mpy_vec3_length);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_vec3_length_squared_obj, 1, mpy_vec3_length_squared);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_vec3_normalized_obj, 1, mpy_vec3_normalized);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_vec3_dot_obj, 2, mpy_vec3_dot);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_vec3_cross_obj, 2, mpy_vec3_cross);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_vec3_lerp_obj, 3, mpy_vec3_lerp);

static mp_obj_t vec3_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  vec3_obj_t *self = mp_obj_malloc(vec3_obj_t, type);
  if (n_args == 0) {
  }
  else if (n_args == 3) {
    self->v.x = mp_obj_get_float(args[0]);
    self->v.y = mp_obj_get_float(args[1]);
    self->v.z = mp_obj_get_float(args[2]);
  }
  else mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("invalid parameters"));
  return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t vec3_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
  vec3_obj_t *lhs = (vec3_obj_t *)MP_OBJ_TO_PTR(lhs_in);
  switch (op) {
    case MP_BINARY_OP_ADD: {
      if (mp_obj_is_type(rhs_in, &type_vec3)) {
        vec3_obj_t *rhs = (vec3_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return pv::box_vec3(lhs->v + rhs->v);
      }
    } break;
    case MP_BINARY_OP_SUBTRACT: {
      if (mp_obj_is_type(rhs_in, &type_vec3)) {
        vec3_obj_t *rhs = (vec3_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return pv::box_vec3(lhs->v - rhs->v);
      }
    } break;
    case MP_BINARY_OP_MULTIPLY: {
      if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
        float v = mp_obj_get_float(rhs_in);
        return pv::box_vec3(lhs->v * v);
      }
    } break;
    case MP_BINARY_OP_TRUE_DIVIDE: {
      if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
        float v = mp_obj_get_float(rhs_in);
        return pv::box_vec3(lhs->v / v);
      }
    } break;
    case MP_BINARY_OP_EQUAL: {
      if (mp_obj_is_type(rhs_in, &type_vec3)) {
        vec3_obj_t *rhs = (vec3_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return mp_obj_new_bool(lhs->v == rhs->v);
      }
      return mp_const_false;
    } break;
    case MP_BINARY_OP_NOT_EQUAL: {
      if (mp_obj_is_type(rhs_in, &type_vec3)) {
        vec3_obj_t *rhs = (vec3_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return mp_obj_new_bool(lhs->v != rhs->v);
      }
      return mp_const_true;
    } break;
    case MP_BINARY_OP_INPLACE_ADD: {
      if (mp_obj_is_type(rhs_in, &type_vec3)) {
        vec3_obj_t *rhs = (vec3_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        lhs->v += rhs->v;
        return lhs_in;
      }
    } break;
    case MP_BINARY_OP_INPLACE_SUBTRACT: {
      if (mp_obj_is_type(rhs_in, &type_vec3)) {
        vec3_obj_t *rhs = (vec3_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        lhs->v -= rhs->v;
        return lhs_in;
      }
    } break;
    case MP_BINARY_OP_INPLACE_MULTIPLY: {
      if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
        float v = mp_obj_get_float(rhs_in);
        lhs->v *= v;
        return lhs_in;
      }
    } break;
    default: break;  // unhandled ops fall through to MP_OBJ_NULL
  }
  return MP_OBJ_NULL;
}

static void vec3_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, vec3_obj_t);
  mp_printf(print, "vec3(%f, %f, %f)", self->v.x, self->v.y, self->v.z);
}

void vec3_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, vec3_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_x:
    {
      if (action == GET) { dest[0] = mp_obj_new_float(self->v.x); return; }
      if (action == SET) { self->v.x = mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_y:
    {
      if (action == GET) { dest[0] = mp_obj_new_float(self->v.y); return; }
      if (action == SET) { self->v.y = mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_z:
    {
      if (action == GET) { dest[0] = mp_obj_new_float(self->v.z); return; }
      if (action == SET) { self->v.z = mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t vec3_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_length), MP_ROM_PTR(&mpy_vec3_length_obj) },
  { MP_ROM_QSTR(MP_QSTR_length_squared), MP_ROM_PTR(&mpy_vec3_length_squared_obj) },
  { MP_ROM_QSTR(MP_QSTR_normalized), MP_ROM_PTR(&mpy_vec3_normalized_obj) },
  { MP_ROM_QSTR(MP_QSTR_dot), MP_ROM_PTR(&mpy_vec3_dot_obj) },
  { MP_ROM_QSTR(MP_QSTR_cross), MP_ROM_PTR(&mpy_vec3_cross_obj) },
  { MP_ROM_QSTR(MP_QSTR_lerp), MP_ROM_PTR(&mpy_vec3_lerp_obj) },
};
static MP_DEFINE_CONST_DICT(vec3_locals_dict, vec3_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_vec3,
  MP_QSTR_vec3,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)vec3_make_new,
  print, (const void *)vec3_print,
  binary_op, (const void *)vec3_binary_op,
  attr, (const void *)vec3_attr,
  locals_dict, &vec3_locals_dict
);

}
