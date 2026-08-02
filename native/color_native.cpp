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
}
