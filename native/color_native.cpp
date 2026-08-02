// native/color.cpp — the colour members that hand back several colours at once.
//
// The DSL has no return type for a list of boxed objects, so a harmony and a
// tonal ladder are written here. Everything they do is core; this is the boxing.

#include "pv_bindings.hpp"

extern "C" {
  #include "py/runtime.h"

  // The most tones anyone builds a palette from. A stack buffer at this size
  // costs under a kilobyte and saves an allocation on a path that then boxes
  // every entry anyway.
  static const int MAX_TONES = 64;

  // A ramp is boxed one colour at a time, so the ceiling is about keeping a
  // typo from asking for a million of them, not about the buffer.
  static const int MAX_RAMP = 1024;

  static mp_obj_t box_colors(const color_t *colors, int n) {
    mp_obj_tuple_t *result = (mp_obj_tuple_t *)MP_OBJ_TO_PTR(mp_obj_new_tuple(n, NULL));
    for(int i = 0; i < n; i++) result->items[i] = pv::box_color(colors[i]);
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t color_harmony(size_t n_args, const mp_obj_t *args) {
    self(args[0], color_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_color_harmony);
#endif
    color_t out[color_t::max_harmony];
    int n = self->c.harmony(mp_obj_get_int(args[1]), out);
    return box_colors(out, n);
  }

  mp_obj_t color_tones(size_t n_args, const mp_obj_t *args) {
    self(args[0], color_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_color_tones);
#endif
    int count = mp_obj_get_int(args[1]);
    if(count < 1 || count > MAX_TONES) {
      mp_raise_msg_varg(&mp_type_ValueError,
        MP_ERROR_TEXT("tones expects 1 to %d colours"), MAX_TONES);
    }

    color_t out[MAX_TONES];
    self->c.tones(out, count);
    return box_colors(out, count);
  }

  // The stop list is parsed here rather than through the DSL's ColorStops
  // marshaller, whose limit and wording belong to brush.gradient.
  mp_obj_t color_ramp(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_color_ramp);
#endif
    size_t stop_count;
    mp_obj_t *items;
    mp_obj_get_array(args[0], &stop_count, &items);
    if(stop_count < 1 || stop_count > (size_t)ramp_max_stops) {
      mp_raise_msg_varg(&mp_type_ValueError,
        MP_ERROR_TEXT("ramp expects 1 to %d colour stops"), ramp_max_stops);
    }

    float positions[ramp_max_stops];
    color_t stops[ramp_max_stops];
    for(size_t i = 0; i < stop_count; i++) {
      size_t pair_len;
      mp_obj_t *pair;
      mp_obj_get_array(items[i], &pair_len, &pair);
      if(pair_len != 2 || !mp_obj_is_type(pair[1], &type_color)) {
        mp_raise_msg(&mp_type_TypeError,
          MP_ERROR_TEXT("each stop must be (position, color)"));
      }
      positions[i] = mp_obj_get_float(pair[0]);
      stops[i] = ((color_obj_t *)MP_OBJ_TO_PTR(pair[1]))->c;
    }

    int count = mp_obj_get_int(args[1]);
    if(count < 1 || count > MAX_RAMP) {
      mp_raise_msg_varg(&mp_type_ValueError,
        MP_ERROR_TEXT("ramp expects 1 to %d colours"), MAX_RAMP);
    }

    color_t *sampled = m_new(color_t, count);
    sample_ramp(sampled, count, positions, stops, (int)stop_count);

    // The list's items start zeroed, which the collector reads as empty, so
    // boxing into it is safe even though each box can collect.
    mp_obj_t result = mp_obj_new_list(count, NULL);
    mp_obj_list_t *list = (mp_obj_list_t *)MP_OBJ_TO_PTR(result);
    for(int i = 0; i < count; i++) list->items[i] = pv::box_color(sampled[i]);

    m_del(color_t, sampled, count);
    return result;
  }
}
