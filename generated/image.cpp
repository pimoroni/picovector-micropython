// AUTO-GENERATED from api/image.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// image.clear: Fill the whole image with the current pen.
mp_obj_t mpy_image_clear(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_clear);
#endif
  self->image->clear();
  return mp_const_none;
}

// image.rectangle: Filled rectangle (rect, or x, y, w, h).
mp_obj_t mpy_image_rectangle(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_rectangle);
#endif
  size_t _i = 1;
  rect_t r = pv::get_xywh(args, &_i, n_args);
  self->image->rectangle(r);
  return mp_const_none;
}

// image.hspan: Horizontal 1px-tall run in the current pen; skips the span buffer (fast path).
mp_obj_t mpy_image_hspan(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_hspan);
#endif
  size_t _i = 1;
  int x = (int)mp_obj_get_float(args[_i]); _i++;
  int y = (int)mp_obj_get_float(args[_i]); _i++;
  int w = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->hspan(x, y, w);
  return mp_const_none;
}

// image.vspan: Vertical 1px-wide run in the current pen; skips the span buffer (fast path).
mp_obj_t mpy_image_vspan(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_vspan);
#endif
  size_t _i = 1;
  int x = (int)mp_obj_get_float(args[_i]); _i++;
  int y = (int)mp_obj_get_float(args[_i]); _i++;
  int h = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->vspan(x, y, h);
  return mp_const_none;
}

// image.line: Line between two points.
mp_obj_t mpy_image_line(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_line);
#endif
  size_t _i = 1;
  vec2_t a = pv::get_xy(args, &_i, n_args);
  vec2_t b = pv::get_xy(args, &_i, n_args);
  self->image->line(a, b);
  return mp_const_none;
}

// image.circle: Filled circle at c with radius r.
mp_obj_t mpy_image_circle(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_circle);
#endif
  size_t _i = 1;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 1);
  float r = mp_obj_get_float(args[_i]); _i++;
  self->image->circle(c, (int)r);
  return mp_const_none;
}

// image.triangle: Filled triangle.
mp_obj_t mpy_image_triangle(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_triangle);
#endif
  size_t _i = 1;
  vec2_t a = pv::get_xy(args, &_i, n_args);
  vec2_t b = pv::get_xy(args, &_i, n_args);
  vec2_t c = pv::get_xy(args, &_i, n_args);
  self->image->triangle(a, b, c);
  return mp_const_none;
}

// image.blur: Box-blur the whole image in place.
mp_obj_t mpy_image_blur(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_blur);
#endif
  size_t _i = 1;
  float radius = mp_obj_get_float(args[_i]); _i++;
  self->image->blur(radius);
  return mp_const_none;
}

// image.bloom: Bloom: soft halo around pixels brighter than `threshold`, added back scaled by `intensity`.
mp_obj_t mpy_image_bloom(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_bloom);
#endif
  size_t _i = 1;
  int threshold = 180;
  if (n_args > _i) { threshold = (int)mp_obj_get_float(args[_i]); _i++; }
  int intensity = 150;
  if (n_args > _i) { intensity = (int)mp_obj_get_float(args[_i]); _i++; }
  float radius = 4.0;
  if (n_args > _i) { radius = mp_obj_get_float(args[_i]); _i++; }
  self->image->bloom(threshold, intensity, radius);
  return mp_const_none;
}

// image.dither: Dither the whole image in place.
mp_obj_t mpy_image_dither(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_dither);
#endif
  self->image->dither();
  return mp_const_none;
}

// image.onebit: Convert to 1-bit black/white in place.
mp_obj_t mpy_image_onebit(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_onebit);
#endif
  self->image->onebit();
  return mp_const_none;
}

// image.monochrome: Convert to monochrome in place.
mp_obj_t mpy_image_monochrome(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_monochrome);
#endif
  self->image->monochrome();
  return mp_const_none;
}

// image.invert: Photonegative the whole image in place.
mp_obj_t mpy_image_invert(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_invert);
#endif
  self->image->invert();
  return mp_const_none;
}

// image.threshold: Two-level luminance threshold to colours lo/hi.
mp_obj_t mpy_image_threshold(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_threshold);
#endif
  size_t _i = 1;
  int level = (int)mp_obj_get_float(args[_i]); _i++;
  color_t lo = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  color_t hi = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  self->image->threshold(level, lo, hi);
  return mp_const_none;
}

// image.saturation: Adjust saturation (amount>0 boosts, -256 = greyscale).
mp_obj_t mpy_image_saturation(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_saturation);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->saturation(amount);
  return mp_const_none;
}

// image.contrast: Adjust contrast around mid-grey (amount>0 more).
mp_obj_t mpy_image_contrast(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_contrast);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->contrast(amount);
  return mp_const_none;
}

// image.duotone: Map luminance onto a shadow→highlight ramp.
mp_obj_t mpy_image_duotone(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_duotone);
#endif
  size_t _i = 1;
  color_t shadow = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  color_t highlight = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  self->image->duotone(shadow, highlight);
  return mp_const_none;
}

// image.crt: CRT tube: darken every `spacing`-th row by `darkness`, with a rounded corner falloff.
mp_obj_t mpy_image_crt(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_crt);
#endif
  size_t _i = 1;
  int spacing = (int)mp_obj_get_float(args[_i]); _i++;
  int darkness = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->crt(spacing, darkness);
  return mp_const_none;
}

// image.grid: Gentle pixel grid: darken every `spacing`-th row and column by `darkness`.
mp_obj_t mpy_image_grid(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_grid);
#endif
  size_t _i = 1;
  int spacing = (int)mp_obj_get_float(args[_i]); _i++;
  int darkness = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->grid(spacing, darkness);
  return mp_const_none;
}

// image.vignette: Darken by distance from the image centre.
mp_obj_t mpy_image_vignette(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_vignette);
#endif
  size_t _i = 1;
  int strength = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->vignette(strength);
  return mp_const_none;
}

// image.gameboy: Map the whole image to the 4 Game Boy greens.
mp_obj_t mpy_image_gameboy(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_gameboy);
#endif
  self->image->gameboy();
  return mp_const_none;
}

// image.noise: Add per-pixel film grain (+/- amount). interval = refresh period in ms (0 = static).
mp_obj_t mpy_image_noise(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_noise);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  int interval = 0;
  if (n_args > _i) { interval = (int)mp_obj_get_float(args[_i]); _i++; }
  self->image->noise(amount, interval);
  return mp_const_none;
}

// image.glitch: VHS channel-shift glitch bands.
mp_obj_t mpy_image_glitch(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_glitch);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->glitch(amount);
  return mp_const_none;
}

// image.oilpaint: Oil paint: dominant colour in a radius, eased back toward the original by strength (0-255).
mp_obj_t mpy_image_oilpaint(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_oilpaint);
#endif
  size_t _i = 1;
  int radius = (int)mp_obj_get_float(args[_i]); _i++;
  int strength = 255;
  if (n_args > _i) { strength = (int)mp_obj_get_float(args[_i]); _i++; }
  self->image->oilpaint(radius, strength);
  return mp_const_none;
}

// image.cga: CGA 4-colour palette.
mp_obj_t mpy_image_cga(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_cga);
#endif
  self->image->cga();
  return mp_const_none;
}

// image.palette_dither: Map to a restricted palette (list of colours). strength is the dither
mp_obj_t mpy_image_palette_dither(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_palette_dither);
#endif
  size_t _i = 1;
  size_t palette_n; mp_obj_t *palette_items;
  mp_obj_get_array(args[_i], &palette_n, &palette_items); _i++;
  if (palette_n < 1 || palette_n > 64) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("palette must have 1 to 64 colours"));
  uint32_t palette[64];
  for (size_t _s = 0; _s < palette_n; _s++) {
    if (!mp_obj_is_type(palette_items[_s], &type_color)) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("palette must be a list of colours"));
    palette[_s] = ((color_obj_t *)MP_OBJ_TO_PTR(palette_items[_s]))->c._p;
  }
  int strength = 64;
  if (n_args > _i) { strength = (int)mp_obj_get_float(args[_i]); _i++; }
  self->image->palette_dither(palette, palette_n, strength);
  return mp_const_none;
}

// image.phosphor: CRT phosphor glow toward tint.
mp_obj_t mpy_image_phosphor(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_phosphor);
#endif
  size_t _i = 1;
  color_t tint = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  self->image->phosphor(tint);
  return mp_const_none;
}

// image.synthwave: Synthwave: neon cyan/magenta/white palette dither with a bloom glow.
mp_obj_t mpy_image_synthwave(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_synthwave);
#endif
  self->image->synthwave();
  return mp_const_none;
}

// image.c64: Commodore 64 16-colour palette.
mp_obj_t mpy_image_c64(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_c64);
#endif
  self->image->c64();
  return mp_const_none;
}

// image.nightvision: Green amplify + grain + edge darkening.
mp_obj_t mpy_image_nightvision(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_nightvision);
#endif
  self->image->nightvision();
  return mp_const_none;
}

// image.chromatic: Chromatic aberration RGB split.
mp_obj_t mpy_image_chromatic(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_chromatic);
#endif
  size_t _i = 1;
  int offset = (int)mp_obj_get_float(args[_i]); _i++;
  self->image->chromatic(offset);
  return mp_const_none;
}

// image.get: Read the pixel colour at p.
mp_obj_t mpy_image_get(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_get);
#endif
  size_t _i = 1;
  vec2_t p = pv::get_xy(args, &_i, n_args);
  return pv::box_color_packed(self->image->get(p.x, p.y));
}

// image.put: Set the pixel at p to the current pen.
mp_obj_t mpy_image_put(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_put);
#endif
  size_t _i = 1;
  vec2_t p = pv::get_xy(args, &_i, n_args);
  self->image->put(p.x, p.y);
  return mp_const_none;
}

// image.shape: Draw a shape, or a list of shapes.
mp_obj_t mpy_image_shape(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_shape);
#endif
  mp_obj_t _arg = args[1];
  if (mp_obj_is_type(_arg, &type_shape)) {
    self->image->shape(((shape_obj_t *)MP_OBJ_TO_PTR(_arg))->shape);
    return mp_const_none;
  }
  if (mp_obj_is_type(_arg, &mp_type_list)) {
    size_t _len; mp_obj_t *_items; mp_obj_list_get(_arg, &_len, &_items);
    for (size_t _k = 0; _k < _len; _k++) self->image->shape(((shape_obj_t *)MP_OBJ_TO_PTR(_items[_k]))->shape);
    return mp_const_none;
  }
  mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("expected a shape or a list of shapes"));
}

// image.blit: Blit another image onto this one (point, rect, or src-rect → dst-rect).
mp_obj_t mpy_image_blit(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_blit);
#endif
  if (n_args == 3 && mp_obj_is_type(args[1], &type_image) && mp_obj_is_type(args[2], &type_vec2)) {
    size_t _i = 1;
    image_t * src = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
    vec2_t at = mp_obj_get_vec2(args[_i]); _i++;
    src->blit(self->image, at);
    return mp_const_none;
  }
  if (n_args == 4 && mp_obj_is_type(args[1], &type_image) && (mp_obj_is_float(args[2]) || mp_obj_is_int(args[2])) && (mp_obj_is_float(args[3]) || mp_obj_is_int(args[3]))) {
    size_t _i = 1;
    image_t * src = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
    float x = mp_obj_get_float(args[_i]); _i++;
    float y = mp_obj_get_float(args[_i]); _i++;
    src->blit(self->image, vec2_t(x,y));
    return mp_const_none;
  }
  if (n_args >= 4 && mp_obj_is_type(args[1], &type_image) && mp_obj_is_type(args[2], &type_rect) && mp_obj_is_type(args[3], &type_rect)) {
    size_t _i = 1;
    image_t * src = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
    rect_t source = mp_obj_get_rect(args[_i]); _i++;
    rect_t dst = mp_obj_get_rect(args[_i]); _i++;
    filter_t filter = NEAREST;
    if (n_args > _i) { filter = (filter_t)mp_obj_get_int(args[_i]); _i++; }
    src->blit(self->image, source, dst, filter);
    return mp_const_none;
  }
  if (n_args >= 3 && mp_obj_is_type(args[1], &type_image) && mp_obj_is_type(args[2], &type_rect)) {
    size_t _i = 1;
    image_t * src = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
    rect_t dst = mp_obj_get_rect(args[_i]); _i++;
    filter_t filter = NEAREST;
    if (n_args > _i) { filter = (filter_t)mp_obj_get_int(args[_i]); _i++; }
    src->blit(self->image, dst, filter);
    return mp_const_none;
  }
  mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("invalid parameter, expected blit(image, point), blit(image, rect) or blit(image, source_rect, dest_rect)"));
}

// image.blit_vspan: Blit a vertical texture span (advanced; used by raycasters).
mp_obj_t mpy_image_blit_vspan(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_blit_vspan);
#endif
  size_t _i = 1;
  image_t * src = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
  vec2_t p = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 3);
  float len = mp_obj_get_float(args[_i]); _i++;
  vec2_t uv0 = pv::get_xy(args, &_i, n_args);
  vec2_t uv1 = pv::get_xy(args, &_i, n_args);
  filter_t filter = NEAREST;
  if (n_args > _i) { filter = (filter_t)mp_obj_get_int(args[_i]); _i++; }
  src->blit_vspan(self->image, p, len, uv0, uv1, filter);
  return mp_const_none;
}

// image.blit_hspan: Blit a horizontal texture span (advanced).
mp_obj_t mpy_image_blit_hspan(size_t n_args, const mp_obj_t *args) {
  self(args[0], image_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_image_blit_hspan);
#endif
  size_t _i = 1;
  image_t * src = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
  vec2_t p = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 3);
  float len = mp_obj_get_float(args[_i]); _i++;
  vec2_t uv0 = pv::get_xy(args, &_i, n_args);
  vec2_t uv1 = pv::get_xy(args, &_i, n_args);
  filter_t filter = NEAREST;
  if (n_args > _i) { filter = (filter_t)mp_obj_get_int(args[_i]); _i++; }
  src->blit_hspan(self->image, p, len, uv0, uv1, filter);
  return mp_const_none;
}

extern "C" mp_obj_t image_load(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_load_obj, 1, image_load);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_image_load_static_obj, MP_ROM_PTR(&mpy_image_load_obj));
extern "C" mp_obj_t image_load_into(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_load_into_obj, 2, image_load_into);
extern "C" mp_obj_t image_window(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_window_obj, 2, image_window);
extern "C" mp_obj_t image_spritesheet(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_spritesheet_obj, 3, image_spritesheet);
extern "C" mp_obj_t image_sprite(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_sprite_obj, 3, image_sprite);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_clear_obj, 1, mpy_image_clear);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_rectangle_obj, 2, mpy_image_rectangle);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_hspan_obj, 4, mpy_image_hspan);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_vspan_obj, 4, mpy_image_vspan);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_line_obj, 3, mpy_image_line);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_circle_obj, 3, mpy_image_circle);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_triangle_obj, 4, mpy_image_triangle);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_blur_obj, 2, mpy_image_blur);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_bloom_obj, 1, mpy_image_bloom);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_dither_obj, 1, mpy_image_dither);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_onebit_obj, 1, mpy_image_onebit);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_monochrome_obj, 1, mpy_image_monochrome);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_invert_obj, 1, mpy_image_invert);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_threshold_obj, 4, mpy_image_threshold);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_saturation_obj, 2, mpy_image_saturation);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_contrast_obj, 2, mpy_image_contrast);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_duotone_obj, 3, mpy_image_duotone);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_crt_obj, 3, mpy_image_crt);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_grid_obj, 3, mpy_image_grid);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_vignette_obj, 2, mpy_image_vignette);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_gameboy_obj, 1, mpy_image_gameboy);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_noise_obj, 2, mpy_image_noise);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_glitch_obj, 2, mpy_image_glitch);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_oilpaint_obj, 2, mpy_image_oilpaint);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_cga_obj, 1, mpy_image_cga);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_palette_dither_obj, 2, mpy_image_palette_dither);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_phosphor_obj, 2, mpy_image_phosphor);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_synthwave_obj, 1, mpy_image_synthwave);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_c64_obj, 1, mpy_image_c64);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_nightvision_obj, 1, mpy_image_nightvision);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_chromatic_obj, 2, mpy_image_chromatic);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_get_obj, 2, mpy_image_get);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_put_obj, 2, mpy_image_put);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_shape_obj, 2, mpy_image_shape);
extern "C" mp_obj_t image_shapes(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_shapes_obj, 2, image_shapes);
extern "C" mp_obj_t image_text(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_text_obj, 3, image_text);
extern "C" mp_obj_t image_measure_text(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_measure_text_obj, 2, image_measure_text);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_blit_obj, 1, mpy_image_blit);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_blit_vspan_obj, 6, mpy_image_blit_vspan);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_blit_hspan_obj, 6, mpy_image_blit_hspan);
extern "C" mp_obj_t image_batch(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_image_batch_obj, 2, image_batch);

static mp_obj_t image_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  image_obj_t *self = mp_obj_malloc_with_finaliser(image_obj_t, type);
  int w = mp_obj_get_int(args[0]);
  int h = mp_obj_get_int(args[1]);
  if (n_args > 2) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
    self->image = new (m_malloc(sizeof(image_t))) image_t(bufinfo.buf, w, h);
  } else {
    self->image = new (m_malloc(sizeof(image_t))) image_t(w, h);
  }
  return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t image__del__(mp_obj_t self_in) {
  self(self_in, image_obj_t);
  if(self->image) { m_del_class(image_t, self->image); };
  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(image__del___obj, image__del__);

static void image_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, image_obj_t);
  mp_printf(print, "image(%d x %d)", int(self->image->bounds().w), int(self->image->bounds().h));
}

void image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, image_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_width:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->bounds().w); return; }
    }
    case MP_QSTR_height:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->bounds().h); return; }
    }
    case MP_QSTR_raw:
    {
      if (action == GET) { dest[0] = mp_obj_new_bytearray_by_ref(self->image->buffer_size(), self->image->ptr(0, 0)); return; }
    }
    case MP_QSTR_clip:
    {
      if (action == GET) { dest[0] = pv::box_rect(self->image->clip()); return; }
      if (action == SET) { self->image->clip(mp_obj_get_rect(dest[1])); dest[0] = MP_OBJ_NULL; return; }
    }
    case MP_QSTR_has_palette:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->image->has_palette()); return; }
    }
    case MP_QSTR_antialias:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->antialias()); return; }
      if (action == SET) { self->image->antialias((antialias_t)(int)mp_obj_get_float(dest[1])); dest[0] = MP_OBJ_NULL; return; }
    }
    case MP_QSTR_fill_rule:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->fill_rule()); return; }
      if (action == SET) { self->image->fill_rule((fill_rule_t)(int)mp_obj_get_float(dest[1])); dest[0] = MP_OBJ_NULL; return; }
    }
    case MP_QSTR_alpha:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->image->alpha()); return; }
      if (action == SET) { self->image->alpha((int)mp_obj_get_float(dest[1])); dest[0] = MP_OBJ_NULL; return; }
    }
    case MP_QSTR_pen:
    {
      if (action == GET) { dest[0] = self->brush ? MP_OBJ_FROM_PTR(self->brush) : mp_const_none; return; }
      if (action == SET) {
        brush_obj_t *brush = mp_obj_to_brush(1, &dest[1]);
        if (!brush) mp_raise_TypeError(MP_ERROR_TEXT("value must be of type brush or color"));
        self->brush = brush; self->image->brush(brush->brush);
        dest[0] = MP_OBJ_NULL; return;
      }
    }
    case MP_QSTR_font:
    {
      if (action == GET) {
        if (self->font) dest[0] = MP_OBJ_FROM_PTR(self->font);
        else if (self->pixel_font) dest[0] = MP_OBJ_FROM_PTR(self->pixel_font);
        else dest[0] = mp_const_none;
        return;
      }
      if (action == SET) {
        if (mp_obj_is_type(dest[1], &type_font)) {
          self->font = (font_obj_t *)dest[1]; self->pixel_font = nullptr;
          self->image->font(&self->font->font);
        } else if (mp_obj_is_type(dest[1], &type_pixel_font)) {
          self->pixel_font = (pixel_font_obj_t *)dest[1]; self->font = nullptr;
          self->image->pixel_font(self->pixel_font->font);
        } else {
          mp_raise_TypeError(MP_ERROR_TEXT("value must be of type Font or PixelFont"));
        }
        dest[0] = MP_OBJ_NULL; return;
      }
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static mp_int_t image_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
  self(self_in, image_obj_t);
  bufinfo->buf = self->image->ptr(0, 0);
  bufinfo->len = self->image->buffer_size();
  bufinfo->typecode = 'B';
  return 0;
}

static const mp_rom_map_elem_t image_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&image__del___obj) },
  { MP_ROM_QSTR(MP_QSTR_X4), MP_ROM_INT(antialias_t::X4) },
  { MP_ROM_QSTR(MP_QSTR_X2), MP_ROM_INT(antialias_t::X2) },
  { MP_ROM_QSTR(MP_QSTR_OFF), MP_ROM_INT(antialias_t::OFF) },
  { MP_ROM_QSTR(MP_QSTR_EVEN_ODD), MP_ROM_INT(fill_rule_t::EVEN_ODD) },
  { MP_ROM_QSTR(MP_QSTR_NON_ZERO), MP_ROM_INT(fill_rule_t::NON_ZERO) },
  { MP_ROM_QSTR(MP_QSTR_NEAREST), MP_ROM_INT(filter_t::NEAREST) },
  { MP_ROM_QSTR(MP_QSTR_BILINEAR), MP_ROM_INT(filter_t::BILINEAR) },
  { MP_ROM_QSTR(MP_QSTR_BICUBIC), MP_ROM_INT(filter_t::BICUBIC) },
  { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&mpy_image_load_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_load_into), MP_ROM_PTR(&mpy_image_load_into_obj) },
  { MP_ROM_QSTR(MP_QSTR_window), MP_ROM_PTR(&mpy_image_window_obj) },
  { MP_ROM_QSTR(MP_QSTR_spritesheet), MP_ROM_PTR(&mpy_image_spritesheet_obj) },
  { MP_ROM_QSTR(MP_QSTR_sprite), MP_ROM_PTR(&mpy_image_sprite_obj) },
  { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&mpy_image_clear_obj) },
  { MP_ROM_QSTR(MP_QSTR_rectangle), MP_ROM_PTR(&mpy_image_rectangle_obj) },
  { MP_ROM_QSTR(MP_QSTR_hspan), MP_ROM_PTR(&mpy_image_hspan_obj) },
  { MP_ROM_QSTR(MP_QSTR_vspan), MP_ROM_PTR(&mpy_image_vspan_obj) },
  { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&mpy_image_line_obj) },
  { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&mpy_image_circle_obj) },
  { MP_ROM_QSTR(MP_QSTR_triangle), MP_ROM_PTR(&mpy_image_triangle_obj) },
  { MP_ROM_QSTR(MP_QSTR_blur), MP_ROM_PTR(&mpy_image_blur_obj) },
  { MP_ROM_QSTR(MP_QSTR_bloom), MP_ROM_PTR(&mpy_image_bloom_obj) },
  { MP_ROM_QSTR(MP_QSTR_dither), MP_ROM_PTR(&mpy_image_dither_obj) },
  { MP_ROM_QSTR(MP_QSTR_onebit), MP_ROM_PTR(&mpy_image_onebit_obj) },
  { MP_ROM_QSTR(MP_QSTR_monochrome), MP_ROM_PTR(&mpy_image_monochrome_obj) },
  { MP_ROM_QSTR(MP_QSTR_invert), MP_ROM_PTR(&mpy_image_invert_obj) },
  { MP_ROM_QSTR(MP_QSTR_threshold), MP_ROM_PTR(&mpy_image_threshold_obj) },
  { MP_ROM_QSTR(MP_QSTR_saturation), MP_ROM_PTR(&mpy_image_saturation_obj) },
  { MP_ROM_QSTR(MP_QSTR_contrast), MP_ROM_PTR(&mpy_image_contrast_obj) },
  { MP_ROM_QSTR(MP_QSTR_duotone), MP_ROM_PTR(&mpy_image_duotone_obj) },
  { MP_ROM_QSTR(MP_QSTR_crt), MP_ROM_PTR(&mpy_image_crt_obj) },
  { MP_ROM_QSTR(MP_QSTR_grid), MP_ROM_PTR(&mpy_image_grid_obj) },
  { MP_ROM_QSTR(MP_QSTR_vignette), MP_ROM_PTR(&mpy_image_vignette_obj) },
  { MP_ROM_QSTR(MP_QSTR_gameboy), MP_ROM_PTR(&mpy_image_gameboy_obj) },
  { MP_ROM_QSTR(MP_QSTR_noise), MP_ROM_PTR(&mpy_image_noise_obj) },
  { MP_ROM_QSTR(MP_QSTR_glitch), MP_ROM_PTR(&mpy_image_glitch_obj) },
  { MP_ROM_QSTR(MP_QSTR_oilpaint), MP_ROM_PTR(&mpy_image_oilpaint_obj) },
  { MP_ROM_QSTR(MP_QSTR_cga), MP_ROM_PTR(&mpy_image_cga_obj) },
  { MP_ROM_QSTR(MP_QSTR_palette_dither), MP_ROM_PTR(&mpy_image_palette_dither_obj) },
  { MP_ROM_QSTR(MP_QSTR_phosphor), MP_ROM_PTR(&mpy_image_phosphor_obj) },
  { MP_ROM_QSTR(MP_QSTR_synthwave), MP_ROM_PTR(&mpy_image_synthwave_obj) },
  { MP_ROM_QSTR(MP_QSTR_c64), MP_ROM_PTR(&mpy_image_c64_obj) },
  { MP_ROM_QSTR(MP_QSTR_nightvision), MP_ROM_PTR(&mpy_image_nightvision_obj) },
  { MP_ROM_QSTR(MP_QSTR_chromatic), MP_ROM_PTR(&mpy_image_chromatic_obj) },
  { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&mpy_image_get_obj) },
  { MP_ROM_QSTR(MP_QSTR_put), MP_ROM_PTR(&mpy_image_put_obj) },
  { MP_ROM_QSTR(MP_QSTR_shape), MP_ROM_PTR(&mpy_image_shape_obj) },
  { MP_ROM_QSTR(MP_QSTR_shapes), MP_ROM_PTR(&mpy_image_shapes_obj) },
  { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&mpy_image_text_obj) },
  { MP_ROM_QSTR(MP_QSTR_measure_text), MP_ROM_PTR(&mpy_image_measure_text_obj) },
  { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&mpy_image_blit_obj) },
  { MP_ROM_QSTR(MP_QSTR_blit_vspan), MP_ROM_PTR(&mpy_image_blit_vspan_obj) },
  { MP_ROM_QSTR(MP_QSTR_blit_hspan), MP_ROM_PTR(&mpy_image_blit_hspan_obj) },
  { MP_ROM_QSTR(MP_QSTR_batch), MP_ROM_PTR(&mpy_image_batch_obj) },
};
static MP_DEFINE_CONST_DICT(image_locals_dict, image_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_image,
  MP_QSTR_image,
  MP_TYPE_FLAG_NONE,
  make_new, (const void *)image_make_new,
  print, (const void *)image_print,
  attr, (const void *)image_attr,
  buffer, (const void *)image_get_buffer,
  locals_dict, &image_locals_dict
);

}
