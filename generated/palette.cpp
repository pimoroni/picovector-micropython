// AUTO-GENERATED from api/palette.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

extern "C" mp_obj_t palette___getitem__(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_palette___getitem___obj, 2, palette___getitem__);

extern "C" mp_obj_t palette_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t palette_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return palette_make_new_impl(type, n_args, n_kw, args);
}

static mp_obj_t palette_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
  self(self_in, palette_obj_t);
  switch (op) {
    case MP_UNARY_OP_LEN: return MP_OBJ_NEW_SMALL_INT(self->count);
    default: break;
  }
  return MP_OBJ_NULL;
}

static void palette_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, palette_obj_t);
  mp_printf(print, "palette(%d colours)", self->count);
}

void palette_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, palette_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_raw:
    {
      if (action == GET) { dest[0] = mp_obj_new_bytearray_by_ref(self->count * sizeof(uint32_t), palette_entries(self)); return; }
      break;
    }
    case MP_QSTR_image:
    {
      if (action == GET) { dest[0] = self->source ? MP_OBJ_FROM_PTR(self->source) : mp_const_none; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static mp_int_t palette_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
  self(self_in, palette_obj_t);
  bufinfo->buf = palette_entries(self);
  bufinfo->len = self->count * sizeof(uint32_t);
  bufinfo->typecode = 'B';
  return 0;
}

static const mp_rom_map_elem_t palette_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR___getitem__), MP_ROM_PTR(&mpy_palette___getitem___obj) },
};
static MP_DEFINE_CONST_DICT(palette_locals_dict, palette_locals_dict_table);

extern "C" mp_obj_t palette_subscr(mp_obj_t, mp_obj_t, mp_obj_t);
MP_DEFINE_CONST_OBJ_TYPE(
  type_palette,
  MP_QSTR_palette,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)palette_make_new,
  print, (const void *)palette_print,
  unary_op, (const void *)palette_unary_op,
  attr, (const void *)palette_attr,
  subscr, (const void *)palette_subscr,
  buffer, (const void *)palette_get_buffer,
  locals_dict, &palette_locals_dict
);

}
