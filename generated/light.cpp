// AUTO-GENERATED from api/pico3d/light.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {


extern "C" mp_obj_t light_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t light_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return light_make_new_impl(type, n_args, n_kw, args);
}

void light_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, light_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_direction:
    {
      if (action == GET) { dest[0] = pv::box_vec3(self->light.direction); return; }
      if (action == SET) { self->light.direction = ((vec3_obj_t *)MP_OBJ_TO_PTR(dest[1]))->v; dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_color:
    {
      if (action == GET) { dest[0] = pv::box_pico3d_rgb(self->light.color); return; }
      if (action == SET) { self->light.color = pv::pico3d_rgb_of(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_ambient:
    {
      if (action == GET) { dest[0] = pv::box_pico3d_rgb(self->light.ambient); return; }
      if (action == SET) { self->light.ambient = pv::pico3d_rgb_of(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_position:
    {
      if (action == GET) { dest[0] = pv::box_vec3(self->light.position); return; }
      if (action == SET) { self->light.position = ((vec3_obj_t *)MP_OBJ_TO_PTR(dest[1]))->v; dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_point:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool((bool)self->light.point); return; }
      if (action == SET) { self->light.point = mp_obj_is_true(dest[1]) ? 1 : 0; dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_atten:
    {
      if (action == GET) { dest[0] = mp_obj_new_float(self->light.atten); return; }
      if (action == SET) { self->light.atten = mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

MP_DEFINE_CONST_OBJ_TYPE(
  type_light,
  MP_QSTR_light,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)light_make_new,
  attr, (const void *)light_attr
);

}
