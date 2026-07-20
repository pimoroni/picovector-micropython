// AUTO-GENERATED from api/brush.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// brush.pattern: Checkerboard pattern brush. Args: c1, c2 (colors), index (0–37) or 8-tuple.
mp_obj_t mpy_brush_pattern(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_pattern);
#endif
  if (n_args == 3 && mp_obj_is_type(args[0], &type_color) && mp_obj_is_type(args[1], &type_color) && mp_obj_is_int(args[2])) {
    size_t _i = 0;
    color_t c1 = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
    color_t c2 = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
    int index = (int)mp_obj_get_float(args[_i]); _i++;
    if (index < 0 || index > 37) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("index out of range"));
    return pv::box_brush(m_new_class(pattern_brush_t, c1, c2, index));
  }
  if (n_args == 3 && mp_obj_is_type(args[0], &type_color) && mp_obj_is_type(args[1], &type_color) && mp_obj_is_type(args[2], &mp_type_tuple)) {
    size_t _i = 0;
    color_t c1 = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
    color_t c2 = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
    size_t pattern_len; mp_obj_t *pattern_items;
    mp_obj_get_array(args[_i], &pattern_len, &pattern_items); _i++;
    if (pattern_len != 8) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("custom pattern must be a tuple with 8 elements"));
    uint8_t pattern[8];
    for (int _b = 0; _b < 8; _b++) pattern[_b] = mp_obj_get_int(pattern_items[_b]);
    return pv::box_brush(m_new_class(pattern_brush_t, c1, c2, pattern));
  }
  mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("invalid parameter, expected brush.pattern(color, color, index | tuple[8])"));
}

// brush.image: Image brush. Args: img (image). Optional: transform (mat3).
mp_obj_t mpy_brush_image(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_image);
#endif
  if (n_args == 1 && mp_obj_is_type(args[0], &type_image)) {
    size_t _i = 0;
    image_t * img = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
    return pv::box_brush(m_new_class(image_brush_t, img));
  }
  if (n_args == 2 && mp_obj_is_type(args[0], &type_image) && mp_obj_is_type(args[1], &type_mat3)) {
    size_t _i = 0;
    image_t * img = ((image_obj_t *)MP_OBJ_TO_PTR(args[_i]))->image; _i++;
    mat3_t transform = ((mat3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->m; _i++;
    return pv::box_brush(m_new_class(image_brush_t, img, &transform));
  }
  mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("invalid parameter, expected brush.image(image, [mat3])"));
}

// brush.gradient: Gradient brush. type: brush.LINEAR or brush.RADIAL; stops: list of
mp_obj_t mpy_brush_gradient(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_gradient);
#endif
  size_t _i = 0;
  int type = (int)mp_obj_get_float(args[_i]); _i++;
  float x1 = mp_obj_get_float(args[_i]); _i++;
  float y1 = mp_obj_get_float(args[_i]); _i++;
  float x2 = mp_obj_get_float(args[_i]); _i++;
  float y2 = mp_obj_get_float(args[_i]); _i++;
  size_t stops_n; mp_obj_t *stops_items;
  mp_obj_get_array(args[_i], &stops_n, &stops_items); _i++;
  if (stops_n < 1 || stops_n > 16) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("gradient expects 1 to 16 colour stops"));
  float stops_positions[16]; uint32_t stops_colors[16];
  for (size_t _s = 0; _s < stops_n; _s++) {
    size_t _sl; mp_obj_t *_stop; mp_obj_get_array(stops_items[_s], &_sl, &_stop);
    if (_sl != 2 || !mp_obj_is_type(_stop[1], &type_color)) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("each stop must be (position, color)"));
    stops_positions[_s] = mp_obj_get_float(_stop[0]);
    stops_colors[_s] = ((color_obj_t *)MP_OBJ_TO_PTR(_stop[1]))->c._p;
  }
  mat3_t * transform = nullptr;
  if (n_args > _i && mp_obj_is_type(args[_i], &type_mat3)) { transform = &((mat3_obj_t *)MP_OBJ_TO_PTR(args[_i]))->m; _i++; }
  return pv::box_brush(m_new_class(gradient_brush_t, (gradient_type_t)type, x1, y1, x2, y2, stops_positions, stops_colors, stops_n, transform));
}

// brush.erase: Erase/window brush. No args erases (dst-out); pass a color for a translucent
mp_obj_t mpy_brush_erase(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_erase);
#endif
  if (n_args == 0) {
    return pv::box_brush(m_new_class(transparent_brush_t));
  }
  if (n_args == 1 && mp_obj_is_type(args[0], &type_color)) {
    size_t _i = 0;
    color_t c = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
    return pv::box_brush(m_new_class(transparent_brush_t, c));
  }
  mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("invalid parameter, expected brush.erase([color])"));
}

// brush.pixelate: Mosaic the shape's area, block size in pixels.
mp_obj_t mpy_brush_pixelate(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_pixelate);
#endif
  size_t _i = 0;
  int size = (int)mp_obj_get_float(args[_i]); _i++;
  if (size < 1) size = 1;
  return pv::box_brush(m_new_class(pixelate_brush_t, size));
}

// brush.blur: Box-blur the shape's area from the target.
mp_obj_t mpy_brush_blur(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_blur);
#endif
  size_t _i = 0;
  int radius = (int)mp_obj_get_float(args[_i]); _i++;
  if (radius < 1) radius = 1;
  return pv::box_brush(m_new_class(blur_brush_t, radius));
}

// brush.lighten: Add amount (0–255) to each channel of the backdrop.
mp_obj_t mpy_brush_lighten(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_lighten);
#endif
  size_t _i = 0;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(brightness_brush_t, amount));
}

// brush.darken: Subtract amount (0–255) from each channel of the backdrop.
mp_obj_t mpy_brush_darken(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_darken);
#endif
  size_t _i = 0;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(brightness_brush_t, -amount));
}

// brush.monochrome: Greyscale the shape's area (per-pixel green-biased luminance).
mp_obj_t mpy_brush_monochrome(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_monochrome);
#endif
  return pv::box_brush(m_new_class(monochrome_brush_t));
}

// brush.dither: Ordered-dither the shape's area to a 4-level palette (screen-aligned).
mp_obj_t mpy_brush_dither(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_dither);
#endif
  return pv::box_brush(m_new_class(dither_brush_t));
}

// brush.invert: Photonegative the shape's area.
mp_obj_t mpy_brush_invert(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_invert);
#endif
  return pv::box_brush(m_new_class(invert_brush_t));
}

// brush.threshold: Two-level threshold on luminance: <= level -> colour lo, else hi.
mp_obj_t mpy_brush_threshold(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_threshold);
#endif
  size_t _i = 0;
  int level = (int)mp_obj_get_float(args[_i]); _i++;
  color_t lo = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  color_t hi = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  return pv::box_brush(m_new_class(threshold_brush_t, level, lo, hi));
}

// brush.saturation: Saturation: amount>0 boosts, <0 desaturates (-256 = greyscale).
mp_obj_t mpy_brush_saturation(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_saturation);
#endif
  size_t _i = 0;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(saturation_brush_t, amount));
}

// brush.contrast: Contrast around mid-grey: amount>0 more, <0 less.
mp_obj_t mpy_brush_contrast(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_contrast);
#endif
  size_t _i = 0;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(contrast_brush_t, amount));
}

// brush.duotone: Map luminance onto a shadow->highlight two-colour ramp (e.g. sepia).
mp_obj_t mpy_brush_duotone(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_duotone);
#endif
  size_t _i = 0;
  color_t shadow = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  color_t highlight = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  return pv::box_brush(m_new_class(duotone_brush_t, shadow, highlight));
}

// brush.crt: CRT tube: darken every `spacing`-th row by `darkness` (0-255) with a rounded corner falloff.
mp_obj_t mpy_brush_crt(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_crt);
#endif
  size_t _i = 0;
  int spacing = (int)mp_obj_get_float(args[_i]); _i++;
  int darkness = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(crt_brush_t, spacing, darkness));
}

// brush.grid: Gentle pixel grid: darken every `spacing`-th row and column by `darkness` (0-255).
mp_obj_t mpy_brush_grid(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_grid);
#endif
  size_t _i = 0;
  int spacing = (int)mp_obj_get_float(args[_i]); _i++;
  int darkness = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(grid_brush_t, spacing, darkness));
}

// brush.vignette: Darken by distance from the centre (strength 0-255).
mp_obj_t mpy_brush_vignette(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_vignette);
#endif
  size_t _i = 0;
  int strength = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(vignette_brush_t, strength));
}

// brush.noise: Per-pixel film grain, +/- up to `amount`. interval is the refresh period
mp_obj_t mpy_brush_noise(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_noise);
#endif
  size_t _i = 0;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  int interval = 0;
  if (n_args > _i) { interval = (int)mp_obj_get_float(args[_i]); _i++; }
  return pv::box_brush(m_new_class(noise_brush_t, amount, interval));
}

// brush.glitch: VHS channel-shift glitch bands; `amount` sets how many bands.
mp_obj_t mpy_brush_glitch(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_glitch);
#endif
  size_t _i = 0;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(glitch_brush_t, amount));
}

// brush.oilpaint: Oil paint: dominant colour in a `radius` neighbourhood, eased back toward
mp_obj_t mpy_brush_oilpaint(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_oilpaint);
#endif
  size_t _i = 0;
  int radius = (int)mp_obj_get_float(args[_i]); _i++;
  int strength = 255;
  if (n_args > _i) { strength = (int)mp_obj_get_float(args[_i]); _i++; }
  return pv::box_brush(m_new_class(oilpaint_brush_t, radius, strength));
}

// brush.phosphor: CRT phosphor glow toward `tint` (e.g. green or amber).
mp_obj_t mpy_brush_phosphor(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_phosphor);
#endif
  size_t _i = 0;
  color_t tint = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  return pv::box_brush(m_new_class(phosphor_brush_t, tint));
}

// brush.nightvision: Night vision: green amplify + grain + edge darkening.
mp_obj_t mpy_brush_nightvision(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_nightvision);
#endif
  return pv::box_brush(m_new_class(nightvision_brush_t));
}

// brush.chromatic: Chromatic aberration: shift R left / B right by `offset` px.
mp_obj_t mpy_brush_chromatic(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_brush_chromatic);
#endif
  size_t _i = 0;
  int offset = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_brush(m_new_class(chromatic_brush_t, offset));
}

static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_pattern_obj, 0, mpy_brush_pattern);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_pattern_static_obj, MP_ROM_PTR(&mpy_brush_pattern_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_image_obj, 0, mpy_brush_image);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_image_static_obj, MP_ROM_PTR(&mpy_brush_image_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_gradient_obj, 6, mpy_brush_gradient);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_gradient_static_obj, MP_ROM_PTR(&mpy_brush_gradient_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_erase_obj, 0, mpy_brush_erase);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_erase_static_obj, MP_ROM_PTR(&mpy_brush_erase_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_pixelate_obj, 1, mpy_brush_pixelate);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_pixelate_static_obj, MP_ROM_PTR(&mpy_brush_pixelate_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_blur_obj, 1, mpy_brush_blur);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_blur_static_obj, MP_ROM_PTR(&mpy_brush_blur_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_lighten_obj, 1, mpy_brush_lighten);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_lighten_static_obj, MP_ROM_PTR(&mpy_brush_lighten_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_darken_obj, 1, mpy_brush_darken);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_darken_static_obj, MP_ROM_PTR(&mpy_brush_darken_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_monochrome_obj, 0, mpy_brush_monochrome);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_monochrome_static_obj, MP_ROM_PTR(&mpy_brush_monochrome_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_dither_obj, 0, mpy_brush_dither);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_dither_static_obj, MP_ROM_PTR(&mpy_brush_dither_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_invert_obj, 0, mpy_brush_invert);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_invert_static_obj, MP_ROM_PTR(&mpy_brush_invert_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_threshold_obj, 3, mpy_brush_threshold);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_threshold_static_obj, MP_ROM_PTR(&mpy_brush_threshold_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_saturation_obj, 1, mpy_brush_saturation);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_saturation_static_obj, MP_ROM_PTR(&mpy_brush_saturation_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_contrast_obj, 1, mpy_brush_contrast);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_contrast_static_obj, MP_ROM_PTR(&mpy_brush_contrast_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_duotone_obj, 2, mpy_brush_duotone);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_duotone_static_obj, MP_ROM_PTR(&mpy_brush_duotone_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_crt_obj, 2, mpy_brush_crt);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_crt_static_obj, MP_ROM_PTR(&mpy_brush_crt_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_grid_obj, 2, mpy_brush_grid);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_grid_static_obj, MP_ROM_PTR(&mpy_brush_grid_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_vignette_obj, 1, mpy_brush_vignette);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_vignette_static_obj, MP_ROM_PTR(&mpy_brush_vignette_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_noise_obj, 1, mpy_brush_noise);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_noise_static_obj, MP_ROM_PTR(&mpy_brush_noise_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_glitch_obj, 1, mpy_brush_glitch);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_glitch_static_obj, MP_ROM_PTR(&mpy_brush_glitch_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_oilpaint_obj, 1, mpy_brush_oilpaint);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_oilpaint_static_obj, MP_ROM_PTR(&mpy_brush_oilpaint_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_phosphor_obj, 1, mpy_brush_phosphor);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_phosphor_static_obj, MP_ROM_PTR(&mpy_brush_phosphor_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_nightvision_obj, 0, mpy_brush_nightvision);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_nightvision_static_obj, MP_ROM_PTR(&mpy_brush_nightvision_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_brush_chromatic_obj, 1, mpy_brush_chromatic);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_brush_chromatic_static_obj, MP_ROM_PTR(&mpy_brush_chromatic_obj));

static const mp_rom_map_elem_t brush_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_LINEAR), MP_ROM_INT(GRADIENT_LINEAR) },
  { MP_ROM_QSTR(MP_QSTR_RADIAL), MP_ROM_INT(GRADIENT_RADIAL) },
  { MP_ROM_QSTR(MP_QSTR_pattern), MP_ROM_PTR(&mpy_brush_pattern_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_image), MP_ROM_PTR(&mpy_brush_image_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_gradient), MP_ROM_PTR(&mpy_brush_gradient_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_erase), MP_ROM_PTR(&mpy_brush_erase_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_pixelate), MP_ROM_PTR(&mpy_brush_pixelate_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_blur), MP_ROM_PTR(&mpy_brush_blur_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_lighten), MP_ROM_PTR(&mpy_brush_lighten_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_darken), MP_ROM_PTR(&mpy_brush_darken_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_monochrome), MP_ROM_PTR(&mpy_brush_monochrome_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_dither), MP_ROM_PTR(&mpy_brush_dither_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_invert), MP_ROM_PTR(&mpy_brush_invert_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_threshold), MP_ROM_PTR(&mpy_brush_threshold_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_saturation), MP_ROM_PTR(&mpy_brush_saturation_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_contrast), MP_ROM_PTR(&mpy_brush_contrast_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_duotone), MP_ROM_PTR(&mpy_brush_duotone_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_crt), MP_ROM_PTR(&mpy_brush_crt_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_grid), MP_ROM_PTR(&mpy_brush_grid_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_vignette), MP_ROM_PTR(&mpy_brush_vignette_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_noise), MP_ROM_PTR(&mpy_brush_noise_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_glitch), MP_ROM_PTR(&mpy_brush_glitch_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_oilpaint), MP_ROM_PTR(&mpy_brush_oilpaint_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_phosphor), MP_ROM_PTR(&mpy_brush_phosphor_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_nightvision), MP_ROM_PTR(&mpy_brush_nightvision_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_chromatic), MP_ROM_PTR(&mpy_brush_chromatic_static_obj) },
};
static MP_DEFINE_CONST_DICT(brush_locals_dict, brush_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_brush,
  MP_QSTR_brush,
  MP_TYPE_FLAG_NONE,
  locals_dict, &brush_locals_dict
);

}
