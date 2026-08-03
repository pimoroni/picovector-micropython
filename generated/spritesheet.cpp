// AUTO-GENERATED from api/spritesheet.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// spritesheet.index: Which frame of the sequence covers ms, honouring the per-frame timings. Wraps while loop is set; holds on the last frame when it is not. Use this over at() when you want the number and no allocation.
mp_obj_t mpy_spritesheet_index(size_t n_args, const mp_obj_t *args) {
  self(args[0], spritesheet_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_spritesheet_index);
#endif
  size_t _i = 1;
  int ms = (int)mp_obj_get_float(args[_i]); _i++;
  return mp_obj_new_int(self->sheet.index_at(ms));
}

// spritesheet.stop: Stop self-timing. elapsed holds at 0 and done reads False until the next start().
mp_obj_t mpy_spritesheet_stop(size_t n_args, const mp_obj_t *args) {
  self(args[0], spritesheet_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_spritesheet_stop);
#endif
  self->sheet.stop();
  return mp_const_none;
}

extern "C" mp_obj_t spritesheet_sprite(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_sprite_obj, 3, spritesheet_sprite);
extern "C" mp_obj_t spritesheet_frame(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_frame_obj, 2, spritesheet_frame);
extern "C" mp_obj_t spritesheet_range(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args);
static MP_DEFINE_CONST_FUN_OBJ_KW(mpy_spritesheet_range_obj, 1, spritesheet_range);
extern "C" mp_obj_t spritesheet_interval(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_interval_obj, 1, spritesheet_interval);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_index_obj, 2, mpy_spritesheet_index);
extern "C" mp_obj_t spritesheet_at(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_at_obj, 2, spritesheet_at);
extern "C" mp_obj_t spritesheet_start(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_start_obj, 1, spritesheet_start);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_spritesheet_stop_obj, 1, mpy_spritesheet_stop);

static void spritesheet_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, spritesheet_obj_t);
  mp_printf(print, "spritesheet(%d x %d, %d frames)", self->sheet.cols(), self->sheet.rows(), self->sheet.frames());
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
    case MP_QSTR_duration:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.duration()); return; }
      break;
    }
    case MP_QSTR_loop:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->sheet.loop()); return; }
      if (action == SET) { self->sheet.loop(mp_obj_is_true(dest[1])); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_direction:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.direction()); return; }
      if (action == SET) { self->sheet.direction((sheet_direction_t)(int)mp_obj_get_float(dest[1])); dest[0] = MP_OBJ_NULL; return; }
      break;
    }
    case MP_QSTR_elapsed:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->sheet.elapsed()); return; }
      break;
    }
    case MP_QSTR_now:
    {
      if (action == GET) { dest[0] = spritesheet_box_now(self); return; }
      break;
    }
    case MP_QSTR_done:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->sheet.done()); return; }
      break;
    }
    case MP_QSTR_running:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->sheet.running()); return; }
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
  { MP_ROM_QSTR(MP_QSTR_sprite), MP_ROM_PTR(&mpy_spritesheet_sprite_obj) },
  { MP_ROM_QSTR(MP_QSTR_frame), MP_ROM_PTR(&mpy_spritesheet_frame_obj) },
  { MP_ROM_QSTR(MP_QSTR_range), MP_ROM_PTR(&mpy_spritesheet_range_obj) },
  { MP_ROM_QSTR(MP_QSTR_interval), MP_ROM_PTR(&mpy_spritesheet_interval_obj) },
  { MP_ROM_QSTR(MP_QSTR_index), MP_ROM_PTR(&mpy_spritesheet_index_obj) },
  { MP_ROM_QSTR(MP_QSTR_at), MP_ROM_PTR(&mpy_spritesheet_at_obj) },
  { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&mpy_spritesheet_start_obj) },
  { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mpy_spritesheet_stop_obj) },
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
