#ifndef NO_QSTR
#include <algorithm>
#include "mp_allocator.hpp"

#include "picovector.hpp"
#include "primitive.hpp"
#include "shape.hpp"
#include "image.hpp"
#include "brush.hpp"
#include "font.hpp"
#include "color.hpp"
#include "pixel_font.hpp"
#include "blend.hpp"
#include "rasteriser.hpp"
#include "tween/tween.hpp"
#include "PNGdec.h"
#endif

using namespace picovector;

extern "C" {
  #include "types.h"

  typedef struct _brush_obj_t {
    mp_obj_base_t base;
    brush_t *brush;
  } brush_obj_t;

  typedef struct _shape_obj_t {
    mp_obj_base_t base;
    shape_t *shape;
    brush_obj_t *brush;
  } shape_obj_t;

  typedef struct _mat3_obj_t {
    mp_obj_base_t base;
    mat3_t m;
  } mat3_obj_t;

  typedef struct _png_handle_t {
    mp_obj_t fhandle;
  } png_handle_t;

  typedef struct _jpeg_handle_t {
    mp_obj_t fhandle;
  } jpeg_handle_t;

  typedef struct _font_obj_t {
    mp_obj_base_t base;
    font_t font;
    uint8_t *buffer;
    uint32_t buffer_size;
  } font_obj_t;

  typedef struct _color_obj_t {
    mp_obj_base_t base;
    // The colour is stored *by value* (only its premultiplied `_p` is used
    // downstream; the rgb/hsv/oklch subclass is just a construction strategy).
    // Embedding avoids a second heap allocation + pointer indirection, and —
    // crucially — avoids leaking a `new`-allocated color_t that lived off the
    // MicroPython GC heap.
    color_t c;
  } color_obj_t;

  typedef struct _pixel_font_obj_t {
    mp_obj_base_t base;
    pixel_font_t *font;
    uint8_t *glyph_buffer;
    uint32_t glyph_buffer_size;
    uint8_t *glyph_data_buffer;
    uint32_t glyph_data_buffer_size;
  } pixel_font_obj_t;

  typedef struct _image_obj_t {
    mp_obj_base_t base;
    image_t *image;
    brush_obj_t *brush;
    font_obj_t *font;
    pixel_font_obj_t *pixel_font;
    void *parent;
  } image_obj_t;

  typedef struct _rect_obj_t {
    mp_obj_base_t base;
    rect_t r;
  } rect_obj_t;

  typedef struct _vec2_obj_t {
    mp_obj_base_t base;
    vec2_t v;
  } vec2_obj_t;

  // A tween carries one value type, tagged by `kind`. The active union member is
  // placement-new'd by tween_make_new_impl; the members are trivially
  // destructible, so no finaliser is needed. A mat3 tween interpolates through a
  // decomposed xform_t (translate/rotate/scale) — a raw matrix can't be lerped
  // element-wise — so its member is a tween_t<xform_t> recomposed on read.
  enum { TWEEN_FLOAT, TWEEN_VEC2, TWEEN_RECT, TWEEN_MAT3 };
  typedef struct _tween_obj_t {
    mp_obj_base_t base;
    uint8_t kind;
    union {
      tween_t<float>   f;
      tween_t<vec2_t>  v;
      tween_t<rect_t>  r;
      tween_t<xform_t> x;
    };
  } tween_obj_t;

  // tween endpoint/timing accessors (native/tween_native.cpp); the generated
  // attr getters box these by reference to the type-tagged union.
  extern mp_obj_t tween_box_from(tween_obj_t *self);
  extern mp_obj_t tween_box_to(tween_obj_t *self);
  extern mp_obj_t tween_box_duration(tween_obj_t *self);
  extern mp_obj_t tween_box_now(tween_obj_t *self);
  extern mp_obj_t tween_box_elapsed(tween_obj_t *self);
  extern mp_obj_t tween_box_done(tween_obj_t *self);
  extern mp_obj_t tween_box_running(tween_obj_t *self);

  // used by image.pen = N and picovector.pen() (global pen)
  extern brush_obj_t *mp_obj_to_brush(size_t n_args, const mp_obj_t *args);

  // image.cpp uses pngdec_open_file and pngdec_open_ram from image_png
  extern int pngdec_open_file(image_obj_t &target, const char* path, int target_width, int target_height);
  extern int pngdec_open_ram(image_obj_t &target, const void* buffer, const size_t size, int target_width, int target_height);

  // ... and jpegdec_open_file and jpegdec_open_ram from image_jpeg
  extern int jpegdec_open_file(image_obj_t &target, const char* path, int target_width, int target_height);
  extern int jpegdec_open_ram(image_obj_t &target, const void* buffer, const size_t size, int target_width, int target_height);
}

extern rect_t mp_obj_get_rect(mp_obj_t rect_in);
extern rect_t mp_obj_get_rect_from_xywh(const mp_obj_t *args);

extern vec2_t mp_obj_get_vec2(mp_obj_t vec2_in);
extern vec2_t mp_obj_get_vec2_from_xy(const mp_obj_t *args);

extern bool mp_obj_is_rect(mp_obj_t rect_in);
extern bool mp_obj_is_vec2(mp_obj_t vec2_in);