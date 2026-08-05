// AUTO-GENERATED from api/spritesheet.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

extern "C" mp_obj_t spritesheet_load(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_load_obj, 1, spritesheet_load);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_spritesheet_load_static_obj, MP_ROM_PTR(&mpy_spritesheet_load_obj));
extern "C" mp_obj_t spritesheet_sprite(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_sprite_obj, 2, spritesheet_sprite);

static void spritesheet_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, spritesheet_obj_t);
  mp_printf(print, "spritesheet(%d x %d, %d cells)", self->sheet.cols(), self->sheet.rows(), self->sheet.frames());
}

void spritesheet_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, spritesheet_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_cols:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.cols()); return; }
      break;
    }
    case MP_QSTR_rows:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.rows()); return; }
      break;
    }
    case MP_QSTR_frames:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.frames()); return; }
      break;
    }
    case MP_QSTR_direction:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.direction()); return; }
      if (action == SET) { self->sheet.direction((sheet_direction_t)(int)mp_obj_get_float(dest[1])); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_timings:
    {
      if (action == GET) { dest[0] = spritesheet_box_timings(self); return; }
      break;
    }
    case MP_QSTR_source:
    {
      if (action == GET) { dest[0] = MP_OBJ_FROM_PTR(self->source); return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t spritesheet_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_ROWS), MP_ROM_INT(SHEET_ROWS) },
  { MP_ROM_QSTR(MP_QSTR_COLUMNS), MP_ROM_INT(SHEET_COLUMNS) },
  { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&mpy_spritesheet_load_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_sprite), MP_ROM_PTR(&mpy_spritesheet_sprite_obj) },
};
static MP_DEFINE_CONST_DICT(spritesheet_locals_dict, spritesheet_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_spritesheet,
  MP_QSTR_spritesheet,
  MP_TYPE_FLAG_NONE,
  print, (const void *)spritesheet_print,
  attr, (const void *)spritesheet_attr,
  locals_dict, &spritesheet_locals_dict
);

}
