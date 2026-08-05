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

  // A numeric argument, narrowed. Generated bindings read `int` params this way
  // so a computed value needs no cast at the call site - sheet.sprite(t.now) and
  // color.rgb(0, 0, 0, 100 * pulse) both work. Hand-written natives use this to
  // match; mp_obj_get_int would reject the float outright.
  static inline int to_int(mp_obj_t o) { return (int)mp_obj_get_float(o); }

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

  // ── constructing an image ──────────────────────────────────────────────────
  // How many bytes an RGBA image of this size needs, refusing a size that cannot
  // be one. The multiply is done in 64 bits and checked, because on the badge
  // size_t is 32 bits: image(65536, 65536) wants 16GB, which wraps to something
  // small and plausible, and the image then believes it owns the larger area.
  static inline size_t check_image_size(int w, int h) {
    if(w <= 0 || h <= 0) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("image width and height must be positive"));
    }
    uint64_t bytes = (uint64_t)(uint32_t)w * (uint32_t)h * 4u;
    if(bytes > (uint64_t)SIZE_MAX) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("image is too large to address"));
    }
    return (size_t)bytes;
  }

  // A caller-supplied buffer has to hold the size it is being wrapped at. It was
  // taken on trust, so image(64, 64, bytearray(16)) built a 16KB image over 16
  // bytes and every draw wrote past the end of it.
  static inline void check_image_buffer(size_t have, size_t want) {
    if(have < want) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("buffer is %u bytes, need %u for an image this size"),
                        (unsigned)have, (unsigned)want);
    }
  }

  // ── a source image, of either type ─────────────────────────────────────────
  // `image` and `indexed_image` are two MicroPython types over one image_t, and
  // an indexed buffer is a perfectly good thing to blit *from*. Both obj structs
  // have the same layout, so one cast serves either.
  static inline bool is_image(mp_obj_t o) {
    return mp_obj_is_type(o, &type_image) || mp_obj_is_type(o, &type_indexed_image);
  }

  static inline image_t *get_image(mp_obj_t o) {
    if(!is_image(o)) mp_raise_TypeError(MP_ERROR_TEXT("expected an image"));
    return ((image_obj_t *)MP_OBJ_TO_PTR(o))->image;
  }

  // ── spritesheet timing ─────────────────────────────────────────────────────
  // A sheet composited from a GIF carries the file's per-frame delays. These
  // turn that into the two answers a caller actually wants - how long a loop
  // lasts, and which cell covers a given point in it - without image growing a
  // clock or any playback state of its own.

  static inline mp_int_t image_total_delay(image_obj_t *self) {
    if(self->frame_delays == MP_OBJ_NULL) return 0;
    size_t n;
    mp_obj_t *items;
    mp_obj_get_array(self->frame_delays, &n, &items);

    mp_int_t total = 0;
    for(size_t i = 0; i < n; i++) total += mp_obj_get_int(items[i]);
    return total;
  }

  static inline mp_int_t image_frame_at(image_obj_t *self, mp_int_t ms) {
    if(self->frame_delays == MP_OBJ_NULL) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("image has no frame timings"));
    }
    size_t n;
    mp_obj_t *items;
    mp_obj_get_array(self->frame_delays, &n, &items);
    if(n == 0) return 0;

    mp_int_t total = 0;
    for(size_t i = 0; i < n; i++) total += mp_obj_get_int(items[i]);
    // Every frame timed at zero is a GIF asking to run as fast as it can, which
    // is a question about the caller's frame rate rather than about the file.
    if(total <= 0) return 0;

    mp_int_t at = ms % total;
    if(at < 0) at += total;   // a clock that has run backwards still lands in the loop

    mp_int_t elapsed = 0;
    for(size_t i = 0; i < n; i++) {
      elapsed += mp_obj_get_int(items[i]);
      if(at < elapsed) return (mp_int_t)i;
    }
    return (mp_int_t)(n - 1);
  }

  // ── gradient geometry ──────────────────────────────────────────────────────
  // Reached through a plain brush, since that is the only type the bindings
  // expose. Only a gradient has geometry, so anything else is a TypeError rather
  // than a silent no-op. Moving a gradient this way skips the lookup table
  // rebuild, which is most of what constructing one costs.
  static inline void brush_geometry(brush_t *b, float x1, float y1, float x2, float y2,
                                    mat3_t *transform) {
    gradient_brush_t *g = b ? b->as_gradient() : nullptr;
    if(!g) mp_raise_TypeError(MP_ERROR_TEXT("only a gradient brush has geometry"));
    g->geometry(x1, y1, x2, y2, transform);
  }

  // ── fractal noise ──────────────────────────────────────────────────────────
  // Reached through a plain brush, for the same reason gradient geometry is.
  static inline fractal_brush_t *as_fractal_or_raise(brush_t *b) {
    fractal_brush_t *f = b ? b->as_fractal() : nullptr;
    if(!f) mp_raise_TypeError(MP_ERROR_TEXT("only a fractal brush has that"));
    return f;
  }

  static inline void brush_ramp(brush_t *b, const float *positions,
                                const color_t *stops, int stop_count) {
    as_fractal_or_raise(b)->ramp(positions, stops, stop_count);
  }

  static inline mat3_t brush_placement(brush_t *b) {
    return as_fractal_or_raise(b)->placement;
  }

  static inline void brush_place(brush_t *b, mat3_t m) {
    as_fractal_or_raise(b)->geometry(&m);
  }

  static inline int brush_seed(brush_t *b) {
    return (int)as_fractal_or_raise(b)->seed;
  }

  static inline int brush_repeat(brush_t *b) {
    return as_fractal_or_raise(b)->repeat;
  }

  static inline float brush_cell(brush_t *b) {
    return as_fractal_or_raise(b)->cell;
  }

  static inline void brush_resize(brush_t *b, float scale) {
    as_fractal_or_raise(b)->resize(scale);
  }

  static inline mp_obj_t box_brush(brush_t *b) {
    brush_obj_t *o = mp_obj_malloc(brush_obj_t, &type_brush);
    o->brush = b;
    return MP_OBJ_FROM_PTR(o);
  }

}  // namespace pv
