// AUTO-GENERATED from api/pico3d/material.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {


extern "C" mp_obj_t material_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t material_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return material_make_new_impl(type, n_args, n_kw, args);
}

void material_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, material_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_color:
    {
      if (action == GET) { dest[0] = pv::box_pico3d_rgb(self->mat.color); return; }
      if (action == SET) { self->mat.color = pv::pico3d_rgb_of(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_texture:
    {
      if (action == GET) { dest[0] = self->texture_ref; return; }
      break;
    }
    case MP_QSTR_filter:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->mat.filter); return; }
      if (action == SET) { self->mat.filter = (pico3d_filter_t)(int)mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_shading:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->shading); return; }
      if (action == SET) { self->shading = (pico3d_shading_t)(int)mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_double_sided:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->mat.double_sided); return; }
      if (action == SET) { self->mat.double_sided = mp_obj_is_true(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_alpha_cutoff:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->mat.alpha_cutoff); return; }
      if (action == SET) { self->mat.alpha_cutoff = (uint8_t)(int)mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_normal_map:
    {
      if (action == GET) { dest[0] = self->normal_map_ref; return; }
      break;
    }
    case MP_QSTR_matcap:
    {
      if (action == GET) { dest[0] = self->matcap_ref; return; }
      break;
    }
    case MP_QSTR_specular:
    {
      if (action == GET) { dest[0] = pv::box_pico3d_rgb(self->mat.specular); return; }
      if (action == SET) { self->mat.specular = pv::pico3d_rgb_of(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_shininess:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->mat.shininess); return; }
      if (action == SET) { self->mat.shininess = (int)mp_obj_get_float(dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t material_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_FLAT), MP_ROM_INT(PICO3D_FLAT) },
  { MP_ROM_QSTR(MP_QSTR_GOURAUD), MP_ROM_INT(PICO3D_GOURAUD) },
  { MP_ROM_QSTR(MP_QSTR_UNLIT), MP_ROM_INT(PICO3D_UNLIT) },
  { MP_ROM_QSTR(MP_QSTR_NEAREST), MP_ROM_INT(PICO3D_NEAREST) },
  { MP_ROM_QSTR(MP_QSTR_BILINEAR), MP_ROM_INT(PICO3D_BILINEAR) },
};
static MP_DEFINE_CONST_DICT(material_locals_dict, material_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_material,
  MP_QSTR_material,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)material_make_new,
  attr, (const void *)material_attr,
  locals_dict, &material_locals_dict
);

}
