// AUTO-GENERATED from api/pico3d/surface.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

extern "C" mp_obj_t surface_clear_depth(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_surface_clear_depth_obj, 1, surface_clear_depth);
extern "C" mp_obj_t surface_render(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args);
static MP_DEFINE_CONST_FUN_OBJ_KW(mpy_surface_render_obj, 5, surface_render);

extern "C" mp_obj_t surface_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t surface_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return surface_make_new_impl(type, n_args, n_kw, args);
}

static void surface_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, surface_obj_t);
  mp_printf(print, "surface(%d x %d)", self->w, self->h);
}

void surface_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, surface_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_image:
    {
      if (action == GET) { dest[0] = MP_OBJ_FROM_PTR(self->source); return; }
      break;
    }
    case MP_QSTR_width:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->w); return; }
      break;
    }
    case MP_QSTR_height:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->h); return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t surface_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_clear_depth), MP_ROM_PTR(&mpy_surface_clear_depth_obj) },
  { MP_ROM_QSTR(MP_QSTR_render), MP_ROM_PTR(&mpy_surface_render_obj) },
};
static MP_DEFINE_CONST_DICT(surface_locals_dict, surface_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_surface,
  MP_QSTR_surface,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)surface_make_new,
  print, (const void *)surface_print,
  attr, (const void *)surface_attr,
  locals_dict, &surface_locals_dict
);

}
