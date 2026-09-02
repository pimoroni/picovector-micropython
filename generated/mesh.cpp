// AUTO-GENERATED from api/pico3d/mesh.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {


extern "C" mp_obj_t mesh_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t mesh_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return mesh_make_new_impl(type, n_args, n_kw, args);
}

static void mesh_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, mesh_obj_t);
  mp_printf(print, "mesh(%d verts, %d tris)", (int)self->mesh.vertex_count, (int)self->mesh.triangle_count);
}

void mesh_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, mesh_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_vertices:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->mesh.vertex_count); return; }
      break;
    }
    case MP_QSTR_triangles:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->mesh.triangle_count); return; }
      break;
    }
    case MP_QSTR_positions:
    {
      if (action == GET) { dest[0] = self->positions_ref; return; }
      break;
    }
    case MP_QSTR_indices:
    {
      if (action == GET) { dest[0] = self->indices_ref; return; }
      break;
    }
    case MP_QSTR_normals:
    {
      if (action == GET) { dest[0] = self->normals_ref; return; }
      break;
    }
    case MP_QSTR_uvs:
    {
      if (action == GET) { dest[0] = self->uvs_ref; return; }
      break;
    }
    case MP_QSTR_colors:
    {
      if (action == GET) { dest[0] = self->colors_ref; return; }
      break;
    }
    case MP_QSTR_tangents:
    {
      if (action == GET) { dest[0] = self->tangents_ref; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

MP_DEFINE_CONST_OBJ_TYPE(
  type_mesh,
  MP_QSTR_mesh,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)mesh_make_new,
  print, (const void *)mesh_print,
  attr, (const void *)mesh_attr
);

}
