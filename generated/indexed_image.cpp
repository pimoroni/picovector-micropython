// AUTO-GENERATED from api/indexed_image.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// indexed_image.get: Read the pixel colour at p, resolved through the palette.
mp_obj_t mpy_indexed_image_get(size_t n_args, const mp_obj_t *args) {
  self(args[0], indexed_image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_indexed_image_get);
#endif
  size_t _i = 1;
  vec2_t p = pv::get_xy(args, &_i, n_args);
  return pv::box_color_packed(self->image->get(p.x, p.y));
}

extern "C" mp_obj_t indexed_image_window(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_indexed_image_window_obj, 2, indexed_image_window);
extern "C" mp_obj_t indexed_image_spritesheet(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_indexed_image_spritesheet_obj, 1, indexed_image_spritesheet);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_indexed_image_get_obj, 2, mpy_indexed_image_get);

static void indexed_image_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, indexed_image_obj_t);
  mp_printf(print, "indexed_image(%d x %d, %d colours)", int(self->image->bounds().w), int(self->image->bounds().h), int(self->image->palette_size()));
}

void indexed_image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, indexed_image_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_width:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->bounds().w); return; }
      break;
    }
    case MP_QSTR_height:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->bounds().h); return; }
      break;
    }
    case MP_QSTR_raw:
    {
      if (action == GET) { dest[0] = mp_obj_new_bytearray_by_ref(self->image->buffer_extent(), self->image->ptr(0, 0)); return; }
      break;
    }
    case MP_QSTR_stride:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->row_stride()); return; }
      break;
    }
    case MP_QSTR_alpha:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->alpha()); return; }
      if (action == SET) { self->image->alpha((int)mp_obj_get_float(dest[1])); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_has_palette:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->image->has_palette()); return; }
      break;
    }
    case MP_QSTR_palette_size:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->palette_size()); return; }
      break;
    }
    case MP_QSTR_palette:
    {
      if (action == GET) { dest[0] = palette_box(self); return; }
      if (action == SET) { palette_assign(self, dest[1]); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static mp_int_t indexed_image_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
  self(self_in, indexed_image_obj_t);
  bufinfo->buf = self->image->ptr(0, 0);
  bufinfo->len = self->image->buffer_extent();
  bufinfo->typecode = 'B';
  return 0;
}

static const mp_rom_map_elem_t indexed_image_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_window), MP_ROM_PTR(&mpy_indexed_image_window_obj) },
  { MP_ROM_QSTR(MP_QSTR_spritesheet), MP_ROM_PTR(&mpy_indexed_image_spritesheet_obj) },
  { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&mpy_indexed_image_get_obj) },
};
static MP_DEFINE_CONST_DICT(indexed_image_locals_dict, indexed_image_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_indexed_image,
  MP_QSTR_indexed_image,
  MP_TYPE_FLAG_NONE,
  print, (const void *)indexed_image_print,
  attr, (const void *)indexed_image_attr,
  buffer, (const void *)indexed_image_get_buffer,
  locals_dict, &indexed_image_locals_dict
);

}
