// AUTO-GENERATED from api/tween.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"
#include "tween/tween.hpp"

extern "C" {

extern "C" mp_obj_t tween_at(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_tween_at_obj, 2, tween_at);
extern "C" mp_obj_t tween_start(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_tween_start_obj, 1, tween_start);
extern "C" mp_obj_t tween_stop(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_tween_stop_obj, 1, tween_stop);

extern "C" mp_obj_t tween_make_new_impl(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_obj_t tween_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  return tween_make_new_impl(type, n_args, n_kw, args);
}

void tween_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, tween_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_from_:
    {
      if (action == GET) { dest[0] = tween_box_from(self); return; }
    }
    case MP_QSTR_to:
    {
      if (action == GET) { dest[0] = tween_box_to(self); return; }
    }
    case MP_QSTR_duration:
    {
      if (action == GET) { dest[0] = tween_box_duration(self); return; }
    }
    case MP_QSTR_elapsed:
    {
      if (action == GET) { dest[0] = tween_box_elapsed(self); return; }
    }
    case MP_QSTR_now:
    {
      if (action == GET) { dest[0] = tween_box_now(self); return; }
    }
    case MP_QSTR_done:
    {
      if (action == GET) { dest[0] = tween_box_done(self); return; }
    }
    case MP_QSTR_running:
    {
      if (action == GET) { dest[0] = tween_box_running(self); return; }
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t tween_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_LINEAR), MP_ROM_INT((uint8_t)easing_t::linear) },
  { MP_ROM_QSTR(MP_QSTR_QUAD_IN), MP_ROM_INT((uint8_t)easing_t::quad_in) },
  { MP_ROM_QSTR(MP_QSTR_QUAD_OUT), MP_ROM_INT((uint8_t)easing_t::quad_out) },
  { MP_ROM_QSTR(MP_QSTR_QUAD_INOUT), MP_ROM_INT((uint8_t)easing_t::quad_inout) },
  { MP_ROM_QSTR(MP_QSTR_CUBIC_IN), MP_ROM_INT((uint8_t)easing_t::cubic_in) },
  { MP_ROM_QSTR(MP_QSTR_CUBIC_OUT), MP_ROM_INT((uint8_t)easing_t::cubic_out) },
  { MP_ROM_QSTR(MP_QSTR_CUBIC_INOUT), MP_ROM_INT((uint8_t)easing_t::cubic_inout) },
  { MP_ROM_QSTR(MP_QSTR_QUART_IN), MP_ROM_INT((uint8_t)easing_t::quart_in) },
  { MP_ROM_QSTR(MP_QSTR_QUART_OUT), MP_ROM_INT((uint8_t)easing_t::quart_out) },
  { MP_ROM_QSTR(MP_QSTR_QUART_INOUT), MP_ROM_INT((uint8_t)easing_t::quart_inout) },
  { MP_ROM_QSTR(MP_QSTR_QUINT_IN), MP_ROM_INT((uint8_t)easing_t::quint_in) },
  { MP_ROM_QSTR(MP_QSTR_QUINT_OUT), MP_ROM_INT((uint8_t)easing_t::quint_out) },
  { MP_ROM_QSTR(MP_QSTR_QUINT_INOUT), MP_ROM_INT((uint8_t)easing_t::quint_inout) },
  { MP_ROM_QSTR(MP_QSTR_SINE_IN), MP_ROM_INT((uint8_t)easing_t::sine_in) },
  { MP_ROM_QSTR(MP_QSTR_SINE_OUT), MP_ROM_INT((uint8_t)easing_t::sine_out) },
  { MP_ROM_QSTR(MP_QSTR_SINE_INOUT), MP_ROM_INT((uint8_t)easing_t::sine_inout) },
  { MP_ROM_QSTR(MP_QSTR_EXPO_IN), MP_ROM_INT((uint8_t)easing_t::expo_in) },
  { MP_ROM_QSTR(MP_QSTR_EXPO_OUT), MP_ROM_INT((uint8_t)easing_t::expo_out) },
  { MP_ROM_QSTR(MP_QSTR_EXPO_INOUT), MP_ROM_INT((uint8_t)easing_t::expo_inout) },
  { MP_ROM_QSTR(MP_QSTR_CIRC_IN), MP_ROM_INT((uint8_t)easing_t::circ_in) },
  { MP_ROM_QSTR(MP_QSTR_CIRC_OUT), MP_ROM_INT((uint8_t)easing_t::circ_out) },
  { MP_ROM_QSTR(MP_QSTR_CIRC_INOUT), MP_ROM_INT((uint8_t)easing_t::circ_inout) },
  { MP_ROM_QSTR(MP_QSTR_BACK_IN), MP_ROM_INT((uint8_t)easing_t::back_in) },
  { MP_ROM_QSTR(MP_QSTR_BACK_OUT), MP_ROM_INT((uint8_t)easing_t::back_out) },
  { MP_ROM_QSTR(MP_QSTR_BACK_INOUT), MP_ROM_INT((uint8_t)easing_t::back_inout) },
  { MP_ROM_QSTR(MP_QSTR_ELASTIC_IN), MP_ROM_INT((uint8_t)easing_t::elastic_in) },
  { MP_ROM_QSTR(MP_QSTR_ELASTIC_OUT), MP_ROM_INT((uint8_t)easing_t::elastic_out) },
  { MP_ROM_QSTR(MP_QSTR_ELASTIC_INOUT), MP_ROM_INT((uint8_t)easing_t::elastic_inout) },
  { MP_ROM_QSTR(MP_QSTR_BOUNCE_IN), MP_ROM_INT((uint8_t)easing_t::bounce_in) },
  { MP_ROM_QSTR(MP_QSTR_BOUNCE_OUT), MP_ROM_INT((uint8_t)easing_t::bounce_out) },
  { MP_ROM_QSTR(MP_QSTR_BOUNCE_INOUT), MP_ROM_INT((uint8_t)easing_t::bounce_inout) },
  { MP_ROM_QSTR(MP_QSTR_at), MP_ROM_PTR(&mpy_tween_at_obj) },
  { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&mpy_tween_start_obj) },
  { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mpy_tween_stop_obj) },
};
static MP_DEFINE_CONST_DICT(tween_locals_dict, tween_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_tween,
  MP_QSTR_tween,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)tween_make_new,
  attr, (const void *)tween_attr,
  locals_dict, &tween_locals_dict
);

}
