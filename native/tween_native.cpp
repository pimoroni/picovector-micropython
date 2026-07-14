// native/tween_native.cpp — hand-written bodies for the tween type.
//
// A tween interpolates between two endpoints, shaped by an easing curve. The
// endpoints may be a float, a vec2 or a rect; the type is chosen from the
// constructor arguments and stored in a tagged union (tween_obj_t). None of
// this reduces to the DSL's "call one method and box the result" convention —
// the constructor dispatches on argument type and at()/the accessors switch on
// the tag — so it is maintained here. The generated tween.cpp provides the type
// definition, locals dict (easing constants) and attr handler.

#include "pv_bindings.hpp"

extern "C" {
  #include "py/runtime.h"

  mp_obj_t tween_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                               size_t n_kw, const mp_obj_t *args) {
    enum { ARG_start, ARG_end, ARG_duration, ARG_easing };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_start,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_end,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_duration, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_easing,   MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);

    mp_obj_t a = vals[ARG_start].u_obj, b = vals[ARG_end].u_obj;
    float duration = vals[ARG_duration].u_obj == MP_OBJ_NULL
                     ? 1.0f : mp_obj_get_float(vals[ARG_duration].u_obj);
    // easing() maps an unknown value to linear, so no range check is needed.
    easing_t e = (easing_t)vals[ARG_easing].u_int;

    // mp_obj_malloc zeroes the object; placement-new builds the active member.
    // A mismatched `end` raises from its accessor. A mat3 tween interpolates a
    // decomposed xform_t (translate/rotate/scale, shear discarded), recomposed
    // to a matrix on read; a raw matrix can't be lerped element-wise.
    tween_obj_t *self = mp_obj_malloc(tween_obj_t, type);
    if (mp_obj_is_vec2(a)) {
      self->kind = TWEEN_VEC2;
      new (&self->v) tween_t<vec2_t>(mp_obj_get_vec2(a), mp_obj_get_vec2(b), duration, e);
    } else if (mp_obj_is_rect(a)) {
      self->kind = TWEEN_RECT;
      new (&self->r) tween_t<rect_t>(mp_obj_get_rect(a), mp_obj_get_rect(b), duration, e);
    } else if (mp_obj_is_type(a, &type_mat3) && mp_obj_is_type(b, &type_mat3)) {
      self->kind = TWEEN_MAT3;
      xform_t xa = xform_t::decompose(((mat3_obj_t *)MP_OBJ_TO_PTR(a))->m);
      xform_t xb = xform_t::decompose(((mat3_obj_t *)MP_OBJ_TO_PTR(b))->m);
      new (&self->x) tween_t<xform_t>(xa, xb, duration, e);
    } else {
      self->kind = TWEEN_FLOAT;
      new (&self->f) tween_t<float>(mp_obj_get_float(a), mp_obj_get_float(b), duration, e);
    }
    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t tween_at(size_t n_args, const mp_obj_t *args) {
    self(args[0], tween_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_tween_at);
#endif
    float t = mp_obj_get_float(args[1]);
    switch (self->kind) {
      case TWEEN_VEC2: return pv::box_vec2(self->v.at(t));
      case TWEEN_RECT: return pv::box_rect(self->r.at(t));
      case TWEEN_MAT3: return pv::box_mat3(self->x.at(t).to_mat3());
      default:         return mp_obj_new_float(self->f.at(t));
    }
  }

  mp_obj_t tween_box_start(tween_obj_t *self) {
    switch (self->kind) {
      case TWEEN_VEC2: return pv::box_vec2(self->v.from());
      case TWEEN_RECT: return pv::box_rect(self->r.from());
      case TWEEN_MAT3: return pv::box_mat3(self->x.from().to_mat3());
      default:         return mp_obj_new_float(self->f.from());
    }
  }

  mp_obj_t tween_box_end(tween_obj_t *self) {
    switch (self->kind) {
      case TWEEN_VEC2: return pv::box_vec2(self->v.to());
      case TWEEN_RECT: return pv::box_rect(self->r.to());
      case TWEEN_MAT3: return pv::box_mat3(self->x.to().to_mat3());
      default:         return mp_obj_new_float(self->f.to());
    }
  }

  mp_obj_t tween_box_duration(tween_obj_t *self) {
    switch (self->kind) {
      case TWEEN_VEC2: return mp_obj_new_float(self->v.duration());
      case TWEEN_RECT: return mp_obj_new_float(self->r.duration());
      case TWEEN_MAT3: return mp_obj_new_float(self->x.duration());
      default:         return mp_obj_new_float(self->f.duration());
    }
  }
}
