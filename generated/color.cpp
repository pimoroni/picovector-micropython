// AUTO-GENERATED from api/color.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"
#include "blend.hpp"

extern "C" {

static color_obj_t _pv_pal(const color_t &c) {
  color_obj_t o{};
  o.base.type = &type_color;
  o.c = c;
  return o;
}

#ifdef TUFTY
static const color_obj_t color_black_obj = _pv_pal(rgb_color_t(0x14, 0x1e, 0x28, 0xff));
#else
static const color_obj_t color_black_obj = _pv_pal(rgb_color_t(0x00, 0x00, 0x00, 0xff));
#endif
static const color_obj_t color_grape_obj = _pv_pal(rgb_color_t(0x44, 0x24, 0x34, 0xff));
static const color_obj_t color_navy_obj = _pv_pal(rgb_color_t(0x30, 0x34, 0x6d, 0xff));
static const color_obj_t color_grey_obj = _pv_pal(rgb_color_t(0x4e, 0x4a, 0x4e, 0xff));
static const color_obj_t color_brown_obj = _pv_pal(rgb_color_t(0x85, 0x4c, 0x30, 0xff));
static const color_obj_t color_green_obj = _pv_pal(rgb_color_t(0x34, 0x65, 0x24, 0xff));
static const color_obj_t color_red_obj = _pv_pal(rgb_color_t(0xd0, 0x46, 0x48, 0xff));
static const color_obj_t color_taupe_obj = _pv_pal(rgb_color_t(0x75, 0x71, 0x61, 0xff));
static const color_obj_t color_blue_obj = _pv_pal(rgb_color_t(0x59, 0x7d, 0xce, 0xff));
static const color_obj_t color_orange_obj = _pv_pal(rgb_color_t(0xd2, 0x7d, 0x2c, 0xff));
static const color_obj_t color_smoke_obj = _pv_pal(rgb_color_t(0x85, 0x95, 0xa1, 0xff));
static const color_obj_t color_lime_obj = _pv_pal(rgb_color_t(0x6d, 0xaa, 0x2c, 0xff));
static const color_obj_t color_latte_obj = _pv_pal(rgb_color_t(0xd2, 0xaa, 0x99, 0xff));
static const color_obj_t color_cyan_obj = _pv_pal(rgb_color_t(0x6d, 0xc2, 0xca, 0xff));
static const color_obj_t color_yellow_obj = _pv_pal(rgb_color_t(0xda, 0xd4, 0x5e, 0xff));
#ifdef TUFTY
static const color_obj_t color_white_obj = _pv_pal(rgb_color_t(0xde, 0xee, 0xd6, 0xff));
#else
static const color_obj_t color_white_obj = _pv_pal(rgb_color_t(0xff, 0xff, 0xff, 0xff));
#endif
static const color_obj_t color_transparent_obj = _pv_pal(rgb_color_t(0x00, 0x00, 0x00, 0x00));
static const color_obj_t color_light_grey_obj = _pv_pal(rgb_color_t(0xc0, 0xc0, 0xc0, 0xff));
static const color_obj_t color_dark_grey_obj = _pv_pal(rgb_color_t(0x40, 0x40, 0x40, 0xff));

// color.rgb: Create a colour from RGB values (0-255 each). Optional alpha (0-255).
mp_obj_t mpy_color_rgb(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_rgb);
#endif
  size_t _i = 0;
  int r = (int)mp_obj_get_float(args[_i]); _i++;
  int g = (int)mp_obj_get_float(args[_i]); _i++;
  int b = (int)mp_obj_get_float(args[_i]); _i++;
  int a = 255;
  if (n_args > _i) { a = (int)mp_obj_get_float(args[_i]); _i++; }
  return pv::box_color(rgb_color_t(r, g, b, a));
}

// color.hsv: Create a colour from HSV components (0-255 each; hue wraps, the rest clamp). Optional alpha (0-255).
mp_obj_t mpy_color_hsv(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_hsv);
#endif
  size_t _i = 0;
  int h = (int)mp_obj_get_float(args[_i]); _i++;
  int s = (int)mp_obj_get_float(args[_i]); _i++;
  int v = (int)mp_obj_get_float(args[_i]); _i++;
  int a = 255;
  if (n_args > _i) { a = (int)mp_obj_get_float(args[_i]); _i++; }
  return pv::box_color(hsv_color_t(h, s, v, a));
}

// color.oklch: Create a colour from OKLCH components (0-255 each, not CSS units: l spans 0-1 lightness, c spans 0-0.35 chroma, and h is 256 counts to a full turn, so 250 is 352 degrees). Optional alpha (0-255).
mp_obj_t mpy_color_oklch(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_oklch);
#endif
  size_t _i = 0;
  int l = (int)mp_obj_get_float(args[_i]); _i++;
  int c = (int)mp_obj_get_float(args[_i]); _i++;
  int h = (int)mp_obj_get_float(args[_i]); _i++;
  int a = 255;
  if (n_args > _i) { a = (int)mp_obj_get_float(args[_i]); _i++; }
  return pv::box_color(oklch_color_t(l, c, h, a));
}

// color.to_oklch: Return this colour authored in OKLCH, so its l, c and h can be read and its arithmetic acts on the axis you meant. Already-OKLCH colours are returned unchanged; anything else goes via sRGB and lands on the nearest byte per axis. Near-greys have no meaningful hue.
mp_obj_t mpy_color_to_oklch(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_to_oklch);
#endif
  return pv::box_color(self->c.to_oklch());
}

// color.to_rgb: Return this colour authored in RGB, whatever space it came from.
mp_obj_t mpy_color_to_rgb(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_to_rgb);
#endif
  return pv::box_color(self->c.to_rgb());
}

// color.contrast: WCAG 2.1 contrast ratio against another colour, 1.0 (identical) to 21.0 (black on white). The audited thresholds are 3 for large text and interface components, 4.5 for body text at AA, 7 at AAA. Alpha is ignored, so composite with over() first if either is translucent.
mp_obj_t mpy_color_contrast(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_contrast);
#endif
  size_t _i = 1;
  color_t other = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  return mp_obj_new_float(self->c.contrast(other));
}

// color.difference: Perceptual distance to another colour, on a scale where black to white is 100. About 2 is where a difference becomes noticeable and about 5 where it becomes obvious, so it answers 'are these two too close to tell apart'. Alpha is ignored.
mp_obj_t mpy_color_difference(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_difference);
#endif
  size_t _i = 1;
  color_t other = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  return mp_obj_new_float(self->c.difference(other));
}

// color.fit: Return this colour with only as much chroma as the screen can show at its lightness and hue. Colours already in gamut are returned unchanged, as are RGB and HSV ones, which cannot be out of it.
mp_obj_t mpy_color_fit(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_fit);
#endif
  return pv::box_color(self->c.fit());
}

// color.max_chroma: The most chroma an OKLCH colour can carry at that lightness and hue (0-255). The gamut is lopsided: a yellow reaches far more than a blue at the same lightness.
mp_obj_t mpy_color_max_chroma(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_max_chroma);
#endif
  size_t _i = 0;
  int l = (int)mp_obj_get_float(args[_i]); _i++;
  int h = (int)mp_obj_get_float(args[_i]); _i++;
  return mp_obj_new_int(color_t::max_chroma(l, h));
}

// color.rotate: Return this colour with its hue rotated by counts, wrapping (256 is a full turn). Where with_h sets an absolute hue, this moves relative to the one it has.
mp_obj_t mpy_color_rotate(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_rotate);
#endif
  size_t _i = 1;
  int counts = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.rotate(counts));
}

// color.saturate: Return this colour with more chroma (OKLCH) or saturation (HSV). lighten's opposite number for the other axis; a negative amount desaturates. Clamps, and fits.
mp_obj_t mpy_color_saturate(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_saturate);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.saturate(amount));
}

// color.readable_on: Return this colour moved along its lightness until it reaches `ratio` contrast against background, holding hue and chroma. Returns it unchanged if it already clears the ratio. A saturated mid-tone background can be unreachable from either end, in which case this returns the most readable colour there is rather than raising.
mp_obj_t mpy_color_readable_on(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_readable_on);
#endif
  size_t _i = 1;
  color_t background = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  float ratio = 4.5;
  if (n_args > _i) { ratio = mp_obj_get_float(args[_i]); _i++; }
  return pv::box_color(self->c.readable_on(background, ratio));
}

// color.lighten: Return this colour lightened by amount (0-255). Moves v on an HSV colour and l on an OKLCH one; on an RGB colour it moves all three channels. Clamps.
mp_obj_t mpy_color_lighten(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_lighten);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.lighten(amount));
}

// color.darken: Return this colour darkened by amount (0-255). The inverse of lighten.
mp_obj_t mpy_color_darken(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_darken);
#endif
  size_t _i = 1;
  int amount = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.darken(amount));
}

// color.scale: Return this colour with its lightness scaled by percent (100 is unchanged). Acts on the same component lighten does. Alpha is untouched.
mp_obj_t mpy_color_scale(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_scale);
#endif
  size_t _i = 1;
  int percent = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.scale(percent));
}

// color.with_alpha: Return this colour at a different alpha (0-255).
mp_obj_t mpy_color_with_alpha(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_alpha);
#endif
  size_t _i = 1;
  int a = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.with_alpha(a));
}

// color.mix: Return this colour blended toward other; t 0 is this colour, 255 is other. Interpolates the authored components when both share a space, taking a hue the short way round; otherwise interpolates sRGB.
mp_obj_t mpy_color_mix(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_mix);
#endif
  size_t _i = 1;
  color_t other = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  int t = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(self->c.mix(other, t));
}

// color.over: Return this colour as it lands over background, weighted by its own alpha. The same composite the renderer performs.
mp_obj_t mpy_color_over(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_over);
#endif
  size_t _i = 1;
  color_t background = (((color_obj_t *)MP_OBJ_TO_PTR(args[_i]))->c); _i++;
  return pv::box_color(self->c.over(background));
}

// color.with_r: Return this colour with a different red channel. RGB colours only.
mp_obj_t mpy_color_with_r(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_r);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_RGB).with_component(0, value));
}

// color.with_g: Return this colour with a different green channel. RGB colours only.
mp_obj_t mpy_color_with_g(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_g);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_RGB).with_component(1, value));
}

// color.with_b: Return this colour with a different blue channel. RGB colours only.
mp_obj_t mpy_color_with_b(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_b);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_RGB).with_component(2, value));
}

// color.with_h: Return this colour at a different hue. HSV and OKLCH colours only.
mp_obj_t mpy_color_with_h(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_h);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_with_hue(self->c, value));
}

// color.with_s: Return this colour at a different saturation. HSV colours only.
mp_obj_t mpy_color_with_s(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_s);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_HSV).with_component(1, value));
}

// color.with_v: Return this colour at a different value. HSV colours only.
mp_obj_t mpy_color_with_v(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_v);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_HSV).with_component(2, value));
}

// color.with_l: Return this colour at a different lightness. OKLCH colours only.
mp_obj_t mpy_color_with_l(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_l);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_OKLCH).with_component(0, value));
}

// color.with_c: Return this colour at a different chroma. OKLCH colours only.
mp_obj_t mpy_color_with_c(size_t n_args, const mp_obj_t *args) {
  self(args[0], color_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_color_with_c);
#endif
  size_t _i = 1;
  int value = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_color(pv::color_require(self->c, COLOR_OKLCH).with_component(1, value));
}

static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_rgb_obj, 3, mpy_color_rgb);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_color_rgb_static_obj, MP_ROM_PTR(&mpy_color_rgb_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_hsv_obj, 3, mpy_color_hsv);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_color_hsv_static_obj, MP_ROM_PTR(&mpy_color_hsv_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_oklch_obj, 3, mpy_color_oklch);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_color_oklch_static_obj, MP_ROM_PTR(&mpy_color_oklch_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_to_oklch_obj, 1, mpy_color_to_oklch);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_to_rgb_obj, 1, mpy_color_to_rgb);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_contrast_obj, 2, mpy_color_contrast);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_difference_obj, 2, mpy_color_difference);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_fit_obj, 1, mpy_color_fit);
extern "C" mp_obj_t color_ramp(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_ramp_obj, 2, color_ramp);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_color_ramp_static_obj, MP_ROM_PTR(&mpy_color_ramp_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_max_chroma_obj, 2, mpy_color_max_chroma);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_color_max_chroma_static_obj, MP_ROM_PTR(&mpy_color_max_chroma_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_rotate_obj, 2, mpy_color_rotate);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_saturate_obj, 2, mpy_color_saturate);
extern "C" mp_obj_t color_harmony(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_harmony_obj, 2, color_harmony);
extern "C" mp_obj_t color_tones(size_t n_args, const mp_obj_t *args);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_tones_obj, 2, color_tones);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_readable_on_obj, 2, mpy_color_readable_on);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_lighten_obj, 2, mpy_color_lighten);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_darken_obj, 2, mpy_color_darken);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_scale_obj, 2, mpy_color_scale);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_alpha_obj, 2, mpy_color_with_alpha);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_mix_obj, 3, mpy_color_mix);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_over_obj, 2, mpy_color_over);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_r_obj, 2, mpy_color_with_r);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_g_obj, 2, mpy_color_with_g);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_b_obj, 2, mpy_color_with_b);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_h_obj, 2, mpy_color_with_h);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_s_obj, 2, mpy_color_with_s);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_v_obj, 2, mpy_color_with_v);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_l_obj, 2, mpy_color_with_l);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_color_with_c_obj, 2, mpy_color_with_c);

static mp_obj_t color_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
  color_obj_t *lhs = (color_obj_t *)MP_OBJ_TO_PTR(lhs_in);
  switch (op) {
    case MP_BINARY_OP_ADD: {
      if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
        float v = mp_obj_get_float(rhs_in);
        return pv::box_color(lhs->c.lighten((int)v));
      }
    } break;
    case MP_BINARY_OP_SUBTRACT: {
      if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
        float v = mp_obj_get_float(rhs_in);
        return pv::box_color(lhs->c.darken((int)v));
      }
    } break;
    case MP_BINARY_OP_MULTIPLY: {
      if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
        float v = mp_obj_get_float(rhs_in);
        return pv::box_color(lhs->c.scale((int)(v * 100.0f + 0.5f)));
      }
    } break;
    case MP_BINARY_OP_EQUAL: {
      if (mp_obj_is_type(rhs_in, &type_color)) {
        color_obj_t *rhs = (color_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return mp_obj_new_bool(lhs->c == rhs->c);
      }
      return mp_const_false;
    } break;
    case MP_BINARY_OP_NOT_EQUAL: {
      if (mp_obj_is_type(rhs_in, &type_color)) {
        color_obj_t *rhs = (color_obj_t *)MP_OBJ_TO_PTR(rhs_in);
        return mp_obj_new_bool(lhs->c != rhs->c);
      }
      return mp_const_true;
    } break;
    default: break;  // unhandled ops fall through to MP_OBJ_NULL
  }
  return MP_OBJ_NULL;
}

static mp_obj_t color_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
  self(self_in, color_obj_t);
  switch (op) {
    case MP_UNARY_OP_HASH: return MP_OBJ_NEW_SMALL_INT((self->c._p ^ (self->c._p >> 30)) & 0x3fffffff);
    default: break;
  }
  return MP_OBJ_NULL;
}

static void color_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, color_obj_t);
  mp_printf(print, "color.%q(%d, %d, %d, %d)", pv::color_space_qstr(self->c), self->c.component(0), self->c.component(1), self->c.component(2), self->c.a());
}

void color_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, color_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_r:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->c.r()); return; }
      break;
    }
    case MP_QSTR_g:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->c.g()); return; }
      break;
    }
    case MP_QSTR_b:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->c.b()); return; }
      break;
    }
    case MP_QSTR_a:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->c.a()); return; }
      break;
    }
    case MP_QSTR_h:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(pv::color_require_hue(self->c).h()); return; }
      break;
    }
    case MP_QSTR_s:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(pv::color_require(self->c, COLOR_HSV).s()); return; }
      break;
    }
    case MP_QSTR_v:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(pv::color_require(self->c, COLOR_HSV).v()); return; }
      break;
    }
    case MP_QSTR_l:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(pv::color_require(self->c, COLOR_OKLCH).l()); return; }
      break;
    }
    case MP_QSTR_c:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(pv::color_require(self->c, COLOR_OKLCH).c()); return; }
      break;
    }
    case MP_QSTR_space:
    {
      if (action == GET) { dest[0] = MP_OBJ_NEW_QSTR(pv::color_space_qstr(self->c)); return; }
      break;
    }
    case MP_QSTR_p:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->c._p); return; }
      break;
    }
    case MP_QSTR_luminance:
    {
      if (action == GET) { dest[0] = mp_obj_new_float(self->c.luminance()); return; }
      break;
    }
    case MP_QSTR_in_gamut:
    {
      if (action == GET) { dest[0] = mp_obj_new_bool(self->c.in_gamut()); return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t color_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_COMPLEMENT), MP_ROM_INT(SCHEME_COMPLEMENT) },
  { MP_ROM_QSTR(MP_QSTR_SPLIT), MP_ROM_INT(SCHEME_SPLIT) },
  { MP_ROM_QSTR(MP_QSTR_TRIAD), MP_ROM_INT(SCHEME_TRIAD) },
  { MP_ROM_QSTR(MP_QSTR_TETRAD), MP_ROM_INT(SCHEME_TETRAD) },
  { MP_ROM_QSTR(MP_QSTR_SQUARE), MP_ROM_INT(SCHEME_SQUARE) },
  { MP_ROM_QSTR(MP_QSTR_ANALOGOUS), MP_ROM_INT(SCHEME_ANALOGOUS) },
  { MP_ROM_QSTR(MP_QSTR_black), MP_ROM_PTR(&color_black_obj) },
  { MP_ROM_QSTR(MP_QSTR_grape), MP_ROM_PTR(&color_grape_obj) },
  { MP_ROM_QSTR(MP_QSTR_navy), MP_ROM_PTR(&color_navy_obj) },
  { MP_ROM_QSTR(MP_QSTR_grey), MP_ROM_PTR(&color_grey_obj) },
  { MP_ROM_QSTR(MP_QSTR_brown), MP_ROM_PTR(&color_brown_obj) },
  { MP_ROM_QSTR(MP_QSTR_green), MP_ROM_PTR(&color_green_obj) },
  { MP_ROM_QSTR(MP_QSTR_red), MP_ROM_PTR(&color_red_obj) },
  { MP_ROM_QSTR(MP_QSTR_taupe), MP_ROM_PTR(&color_taupe_obj) },
  { MP_ROM_QSTR(MP_QSTR_blue), MP_ROM_PTR(&color_blue_obj) },
  { MP_ROM_QSTR(MP_QSTR_orange), MP_ROM_PTR(&color_orange_obj) },
  { MP_ROM_QSTR(MP_QSTR_smoke), MP_ROM_PTR(&color_smoke_obj) },
  { MP_ROM_QSTR(MP_QSTR_lime), MP_ROM_PTR(&color_lime_obj) },
  { MP_ROM_QSTR(MP_QSTR_latte), MP_ROM_PTR(&color_latte_obj) },
  { MP_ROM_QSTR(MP_QSTR_cyan), MP_ROM_PTR(&color_cyan_obj) },
  { MP_ROM_QSTR(MP_QSTR_yellow), MP_ROM_PTR(&color_yellow_obj) },
  { MP_ROM_QSTR(MP_QSTR_white), MP_ROM_PTR(&color_white_obj) },
  { MP_ROM_QSTR(MP_QSTR_transparent), MP_ROM_PTR(&color_transparent_obj) },
  { MP_ROM_QSTR(MP_QSTR_light_grey), MP_ROM_PTR(&color_light_grey_obj) },
  { MP_ROM_QSTR(MP_QSTR_dark_grey), MP_ROM_PTR(&color_dark_grey_obj) },
  { MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&mpy_color_rgb_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_hsv), MP_ROM_PTR(&mpy_color_hsv_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_oklch), MP_ROM_PTR(&mpy_color_oklch_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_to_oklch), MP_ROM_PTR(&mpy_color_to_oklch_obj) },
  { MP_ROM_QSTR(MP_QSTR_to_rgb), MP_ROM_PTR(&mpy_color_to_rgb_obj) },
  { MP_ROM_QSTR(MP_QSTR_contrast), MP_ROM_PTR(&mpy_color_contrast_obj) },
  { MP_ROM_QSTR(MP_QSTR_difference), MP_ROM_PTR(&mpy_color_difference_obj) },
  { MP_ROM_QSTR(MP_QSTR_fit), MP_ROM_PTR(&mpy_color_fit_obj) },
  { MP_ROM_QSTR(MP_QSTR_ramp), MP_ROM_PTR(&mpy_color_ramp_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_max_chroma), MP_ROM_PTR(&mpy_color_max_chroma_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_rotate), MP_ROM_PTR(&mpy_color_rotate_obj) },
  { MP_ROM_QSTR(MP_QSTR_saturate), MP_ROM_PTR(&mpy_color_saturate_obj) },
  { MP_ROM_QSTR(MP_QSTR_harmony), MP_ROM_PTR(&mpy_color_harmony_obj) },
  { MP_ROM_QSTR(MP_QSTR_tones), MP_ROM_PTR(&mpy_color_tones_obj) },
  { MP_ROM_QSTR(MP_QSTR_readable_on), MP_ROM_PTR(&mpy_color_readable_on_obj) },
  { MP_ROM_QSTR(MP_QSTR_lighten), MP_ROM_PTR(&mpy_color_lighten_obj) },
  { MP_ROM_QSTR(MP_QSTR_darken), MP_ROM_PTR(&mpy_color_darken_obj) },
  { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_PTR(&mpy_color_scale_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_alpha), MP_ROM_PTR(&mpy_color_with_alpha_obj) },
  { MP_ROM_QSTR(MP_QSTR_mix), MP_ROM_PTR(&mpy_color_mix_obj) },
  { MP_ROM_QSTR(MP_QSTR_over), MP_ROM_PTR(&mpy_color_over_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_r), MP_ROM_PTR(&mpy_color_with_r_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_g), MP_ROM_PTR(&mpy_color_with_g_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_b), MP_ROM_PTR(&mpy_color_with_b_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_h), MP_ROM_PTR(&mpy_color_with_h_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_s), MP_ROM_PTR(&mpy_color_with_s_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_v), MP_ROM_PTR(&mpy_color_with_v_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_l), MP_ROM_PTR(&mpy_color_with_l_obj) },
  { MP_ROM_QSTR(MP_QSTR_with_c), MP_ROM_PTR(&mpy_color_with_c_obj) },
};
static MP_DEFINE_CONST_DICT(color_locals_dict, color_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_color,
  MP_QSTR_color,
  MP_TYPE_FLAG_NONE,
  print, (const void *)color_print,
  unary_op, (const void *)color_unary_op,
  binary_op, (const void *)color_binary_op,
  attr, (const void *)color_attr,
  locals_dict, &color_locals_dict
);

}
