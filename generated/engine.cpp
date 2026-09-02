// AUTO-GENERATED from api/pico3d/engine.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// engine.cores: Rasterise on one core or two, returning the count actually in effect - always 1 on a build without core1. Two cores split the screen into bands and bin each triangle into the bands it touches, so the win is on scenes that are fill-bound rather than triangle-bound. It borrows the same core1 the picovector rasteriser uses, so the two never overlap.
mp_obj_t mpy_engine_cores(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_engine_cores);
#endif
  size_t _i = 0;
  int n = (int)mp_obj_get_float(args[_i]); _i++;
  return mp_obj_new_int((pico3d_set_cores(n), pico3d_get_cores()));
}

// engine.core_count: How many cores the rasteriser is currently using.
mp_obj_t mpy_engine_core_count(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_engine_core_count);
#endif
  return mp_obj_new_int(pico3d_get_cores());
}

static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_engine_cores_obj, 1, mpy_engine_cores);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_engine_cores_static_obj, MP_ROM_PTR(&mpy_engine_cores_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_engine_core_count_obj, 0, mpy_engine_core_count);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_engine_core_count_static_obj, MP_ROM_PTR(&mpy_engine_core_count_obj));
extern "C" mp_obj_t engine_profile(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_engine_profile_obj, 0, engine_profile);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_engine_profile_static_obj, MP_ROM_PTR(&mpy_engine_profile_obj));

static const mp_rom_map_elem_t engine_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_cores), MP_ROM_PTR(&mpy_engine_cores_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_core_count), MP_ROM_PTR(&mpy_engine_core_count_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_profile), MP_ROM_PTR(&mpy_engine_profile_static_obj) },
};
static MP_DEFINE_CONST_DICT(engine_locals_dict, engine_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_engine,
  MP_QSTR_engine,
  MP_TYPE_FLAG_NONE,
  locals_dict, &engine_locals_dict
);

}
