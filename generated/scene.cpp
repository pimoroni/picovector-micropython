// AUTO-GENERATED from api/pico3d/scene.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

extern "C" mp_obj_t scene_reset(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_scene_reset_obj, 1, scene_reset);
extern "C" mp_obj_t scene_add(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args);
static MP_DEFINE_CONST_FUN_OBJ_KW(mpy_scene_add_obj, 5, scene_add);

extern "C" mp_obj_t scene_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t scene_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return scene_make_new_impl(type, n_args, n_kw, args);
}

static void scene_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, scene_obj_t);
  mp_printf(print, "scene(%d meshes, %d verts, %d tris)", (int)self->sc.sub_count, (int)self->sc.vert_count, (int)self->sc.tri_count);
}

void scene_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, scene_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_meshes:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sc.sub_count); return; }
      break;
    }
    case MP_QSTR_vertices:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sc.vert_count); return; }
      break;
    }
    case MP_QSTR_triangles:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sc.tri_count); return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t scene_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&mpy_scene_reset_obj) },
  { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&mpy_scene_add_obj) },
};
static MP_DEFINE_CONST_DICT(scene_locals_dict, scene_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_scene,
  MP_QSTR_scene,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)scene_make_new,
  print, (const void *)scene_print,
  attr, (const void *)scene_attr,
  locals_dict, &scene_locals_dict
);

}
