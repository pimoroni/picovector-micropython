#pragma once

// pv_bindings.hpp — lean helpers for the *generated* PicoVector bindings.
//
// This replaces the token-pasting MPY_BIND_* macros from mp_helpers.hpp. The
// generator (generate.py) emits the full mp_obj_t function bodies, locals
// dicts and type definitions directly; all this header provides is a handful
// of small `static inline` helpers used by that generated code.
//
// Everything here is `inline` rather than macro-expanded so it stays
// type-checked and debuggable, while still compiling away to nothing in the
// tight draw loops these bindings sit in.

extern "C" {
  #include "py/runtime.h"
  #include "py/stream.h"
}

#include "pv_objs.hpp"   // picovector MicroPython obj structs + type_* externs
#include "pv_metrics.hpp" // optional per-binding metrics (no-op unless PV_METRICS)

using namespace picovector;

// ── instance unwrap ─────────────────────────────────────────────────────────
// `self(self_in, vec2_obj_t)` -> `vec2_obj_t *self = ...`
#define self(self_in, T) T *self = (T *)MP_OBJ_TO_PTR(self_in)

// ── placement new/delete into the MicroPython GC heap ────────────────────────
#define m_new_class(cls, ...) new (m_new(cls, 1)) cls(__VA_ARGS__)
#define m_del_class(cls, ptr) ptr->~cls(); m_del(cls, ptr, 1)

// ── attr GET/SET/DELETE discrimination (shared with picovector.cpp) ──────────
constexpr size_t GET = 0b1 << 31;
constexpr size_t SET = 0b1 << 30;
constexpr size_t DELETE = 0b1 << 29;

typedef size_t action_t;
// Shared helpers defined in runtime/pv_support.cpp (C++ linkage). The vec2 /
// rect / brush helpers are already declared by picovector.hpp.
extern action_t m_attr_action(mp_obj_t *dest);
extern uint32_t ru32(mp_obj_t file);
extern uint16_t ru16(mp_obj_t file);

namespace pv {

  static inline void need(size_t have, size_t want) {
    if (have < want) {
      mp_raise_msg_varg(&mp_type_TypeError,
                        MP_ERROR_TEXT("missing required positional arguments"));
    }
  }

  // Read a vec2 from args[*i], accepting either a single vec2 or an (x, y)
  // float pair, advancing *i past whatever it consumed. This is the inline
  // replacement for the MPY_GET_XY_OR_VEC2 macro — but here the generator
  // already knows the post-expansion arity, so it can emit a correct argument
  // count check (the bug the 0001/0002 patches fixed by hand). The float form
  // is bounds-checked against n so a short call raises instead of reading OOB.
  static inline vec2_t get_xy(const mp_obj_t *args, size_t *i, size_t n) {
    if (mp_obj_is_vec2(args[*i])) {
      vec2_t v = mp_obj_get_vec2(args[*i]);
      *i += 1;
      return v;
    }
    need(n, *i + 2);
    vec2_t v(mp_obj_get_float(args[*i]), mp_obj_get_float(args[*i + 1]));
    *i += 2;
    return v;
  }

  // Read a rect from args[*i], accepting either a single rect or an
  // (x, y, w, h) float quad, advancing *i.
  static inline rect_t get_xywh(const mp_obj_t *args, size_t *i, size_t n) {
    if (mp_obj_is_rect(args[*i])) {
      rect_t r = mp_obj_get_rect(args[*i]);
      *i += 1;
      return r;
    }
    need(n, *i + 4);
    rect_t r(mp_obj_get_float(args[*i]), mp_obj_get_float(args[*i + 1]),
             mp_obj_get_float(args[*i + 2]), mp_obj_get_float(args[*i + 3]));
    *i += 4;
    return r;
  }

  // Range validation. clamp=false raises ValueError; clamp=true silently clamps.
  static inline float check_range_f(float v, float lo, float hi, bool clamp,
                                    const char *name) {
    if (v < lo || v > hi) {
      if (clamp) return v < lo ? lo : hi;
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("%s out of range"), name);
    }
    return v;
  }
  static inline int check_range_i(int v, int lo, int hi, bool clamp,
                                  const char *name) {
    if (v < lo || v > hi) {
      if (clamp) return v < lo ? lo : hi;
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("%s out of range"), name);
    }
    return v;
  }

  // ── boxing: wrap a produced C++ value back into a fresh MicroPython object ──
  static inline mp_obj_t box_vec2(vec2_t v) {
    vec2_obj_t *o = mp_obj_malloc(vec2_obj_t, &type_vec2);
    o->v = v;
    return MP_OBJ_FROM_PTR(o);
  }

  static inline mp_obj_t box_rect(rect_t r) {
    rect_obj_t *o = mp_obj_malloc(rect_obj_t, &type_rect);
    o->r = r;
    return MP_OBJ_FROM_PTR(o);
  }

  static inline mp_obj_t box_mat3(const mat3_t &m) {
    mat3_obj_t *o = mp_obj_malloc(mat3_obj_t, &type_mat3);
    o->m = m;
    return MP_OBJ_FROM_PTR(o);
  }

  // shapes own heap-allocated geometry, so they get a finaliser.
  static inline mp_obj_t box_shape(shape_t *s) {
    shape_obj_t *o = mp_obj_malloc(shape_obj_t, &type_shape);
    o->shape = s;
    o->brush = nullptr;
    return MP_OBJ_FROM_PTR(o);
  }

  // Box a colour by value. The argument is typically a freshly-constructed
  // subclass temporary (rgb_color_t/hsv_color_t/oklch_color_t); the subclasses
  // add no state, so slicing into the obj's embedded base carries everything,
  // including which space the colour was authored in. No separate (leaky) `new`.
  static inline mp_obj_t box_color(const color_t &c) {
    color_obj_t *o = mp_obj_malloc(color_obj_t, &type_color);
    new (&o->c) color_t(c);   // placement-construct the embedded value
    return MP_OBJ_FROM_PTR(o);
  }

  // ── colour components ──────────────────────────────────────────────────────
  // A colour answers `.r/.g/.b/.a` whatever space it was authored in, because
  // those come from the resolved sRGB. The authored components are another
  // matter: `.l` on an RGB colour would hand back a red channel, and `.h` means
  // nothing there at all. Raise rather than answer with a number that looks
  // plausible. Reads and with_*() edits share these guards.
  static inline const color_t &color_require(const color_t &c, color_space_t space) {
    if(c.space() != space) {
      mp_raise_msg(&mp_type_AttributeError,
                   MP_ERROR_TEXT("component belongs to a different colour space"));
    }
    return c;
  }

  static inline const color_t &color_require_hue(const color_t &c) {
    if(c.space() == COLOR_RGB) {
      mp_raise_msg(&mp_type_AttributeError, MP_ERROR_TEXT("an rgb colour has no hue"));
    }
    return c;
  }

  // Hue sits in a different component slot in each space that has one.
  static inline color_t color_with_hue(const color_t &c, uint8_t value) {
    return color_require_hue(c).with_component(c.space() == COLOR_OKLCH ? 2 : 0, value);
  }

  // Doubles as the name of the constructor that made the colour, so a repr reads
  // back as the call that would rebuild it.
  static inline qstr color_space_qstr(const color_t &c) {
    switch(c.space()) {
      case COLOR_HSV:   return MP_QSTR_hsv;
      case COLOR_OKLCH: return MP_QSTR_oklch;
      default:          return MP_QSTR_rgb;
    }
  }

  // box a colour read back from a framebuffer word (image.get). The word is
  // premultiplied, so the components have to be divided back out - passing them
  // straight to rgb_color_t premultiplied them a second time, which read a
  // half-transparent pixel back at a quarter of its brightness.
  static inline mp_obj_t box_color_packed(uint32_t c) {
    return box_color(color_from_premul(c));
  }

  static inline mp_obj_t box_brush(brush_t *b) {
    brush_obj_t *o = mp_obj_malloc(brush_obj_t, &type_brush);
    o->brush = b;
    return MP_OBJ_FROM_PTR(o);
  }

}  // namespace pv
