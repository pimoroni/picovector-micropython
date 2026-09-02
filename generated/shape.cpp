// AUTO-GENERATED from api/shape.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

// shape.custom: Custom shape from one or more contours (extra contours = holes); each is a list of vec2, or an array('f') of flat x, y pairs which boxes no vec2.
mp_obj_t mpy_shape_custom(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_custom);
#endif
  shape_t *paths_shape = new (PV_MALLOC(sizeof(shape_t))) shape_t(n_args);
  for (size_t _p = 0; _p < n_args; _p++) {
    mp_buffer_info_t _bi;
    if (mp_get_buffer(args[_p], &_bi, MP_BUFFER_READ) && _bi.typecode == 'f') {
      if (_bi.len % (2 * sizeof(float))) mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("array('f') needs an even number of values (x, y pairs)"));
      size_t _pc = _bi.len / (2 * sizeof(float));
      const float *_f = (const float *)_bi.buf;
      path_t poly(_pc);
      for (size_t _k = 0; _k < _pc; _k++) poly.add_point(_f[_k * 2], _f[_k * 2 + 1]);
      paths_shape->add_path(poly);
      continue;
    }
    if (!mp_obj_is_type(args[_p], &mp_type_list)) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("expected a list of vec2 points or an array('f')"));
    size_t _pc; mp_obj_t *_pts; mp_obj_list_get(args[_p], &_pc, &_pts);
    path_t poly(_pc);
    for (size_t _k = 0; _k < _pc; _k++) {
      if (!mp_obj_is_type(_pts[_k], &type_vec2)) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("expected a list of vec2 points"));
      poly.add_point(((vec2_obj_t *)MP_OBJ_TO_PTR(_pts[_k]))->v);
    }
    paths_shape->add_path(poly);
  }
  return pv::box_shape(paths_shape);
}

// shape.combine: One compound shape from several, each source's transform baked into the copy it contributes. Draw it with fill_rule = NON_ZERO to fill the union; under EVEN_ODD the overlaps come out hollow. The whole thing rasterises in one pass, which is capped at 1024 points.
mp_obj_t mpy_shape_combine(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_combine);
#endif
  shape_t *combined = new (PV_MALLOC(sizeof(shape_t))) shape_t(n_args);
  for (size_t _s = 0; _s < n_args; _s++) {
    if (mp_obj_is_type(args[_s], &type_shape)) {
      combined->append(*((shape_obj_t *)MP_OBJ_TO_PTR(args[_s]))->shape);
      continue;
    }
    if (!mp_obj_is_type(args[_s], &mp_type_list)) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("expected a shape or a list of shapes"));
    size_t _sc; mp_obj_t *_items; mp_obj_list_get(args[_s], &_sc, &_items);
    for (size_t _k = 0; _k < _sc; _k++) {
      if (!mp_obj_is_type(_items[_k], &type_shape)) mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("expected a shape or a list of shapes"));
      combined->append(*((shape_obj_t *)MP_OBJ_TO_PTR(_items[_k]))->shape);
    }
  }
  return pv::box_shape(combined);
}

// shape.regular_polygon: Regular polygon. Args: centre, radius, number of sides.
mp_obj_t mpy_shape_regular_polygon(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_regular_polygon);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 2);
  float r = mp_obj_get_float(args[_i]); _i++;
  int sides = (int)mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(regular_polygon(c.x, c.y, sides, r));
}

// shape.circle: Circle. Args: centre (vec2 or x, y), radius.
mp_obj_t mpy_shape_circle(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_circle);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 1);
  float r = mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(circle(c.x, c.y, r));
}

// shape.ellipse: Ellipse. Args: centre, x-radius, y-radius.
mp_obj_t mpy_shape_ellipse(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_ellipse);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 2);
  float rx = mp_obj_get_float(args[_i]); _i++;
  float ry = mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(ellipse(c.x, c.y, rx, ry));
}

// shape.rectangle: Rectangle. Args: a rect, or x, y, w, h.
mp_obj_t mpy_shape_rectangle(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_rectangle);
#endif
  size_t _i = 0;
  rect_t r = pv::get_xywh(args, &_i, n_args);
  return pv::box_shape(rectangle(r.x, r.y, r.w, r.h));
}

// shape.rounded_rectangle: Rounded rectangle. Corner radii (TL, TR, BR, BL) default to r1.
mp_obj_t mpy_shape_rounded_rectangle(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_rounded_rectangle);
#endif
  size_t _i = 0;
  rect_t r = pv::get_xywh(args, &_i, n_args);
  pv::need(n_args, _i + 1);
  float r1 = mp_obj_get_float(args[_i]); _i++;
  float r2 = r1;
  if (n_args > _i) { r2 = mp_obj_get_float(args[_i]); _i++; }
  float r3 = r1;
  if (n_args > _i) { r3 = mp_obj_get_float(args[_i]); _i++; }
  float r4 = r1;
  if (n_args > _i) { r4 = mp_obj_get_float(args[_i]); _i++; }
  return pv::box_shape(rounded_rectangle(r.x, r.y, r.w, r.h, r1, r2, r3, r4));
}

// shape.squircle: Squircle (super-ellipse). Args: centre, size; n is the exponent (default 4).
mp_obj_t mpy_shape_squircle(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_squircle);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 1);
  float s = mp_obj_get_float(args[_i]); _i++;
  float n = 4.0;
  if (n_args > _i) { n = mp_obj_get_float(args[_i]); _i++; }
  if (n < 2) n = 2;
  return pv::box_shape(squircle(c.x, c.y, s, n));
}

// shape.arc: Annular sector / arc. Args: centre, inner & outer radius, from/to angle (degrees).
mp_obj_t mpy_shape_arc(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_arc);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 4);
  float inner = mp_obj_get_float(args[_i]); _i++;
  float outer = mp_obj_get_float(args[_i]); _i++;
  float from_a = mp_obj_get_float(args[_i]); _i++;
  float to_a = mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(arc(c.x, c.y, from_a, to_a, inner, outer));
}

// shape.pie: Pie / sector slice. Args: centre, radius, from/to angle (degrees).
mp_obj_t mpy_shape_pie(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_pie);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 3);
  float r = mp_obj_get_float(args[_i]); _i++;
  float from_a = mp_obj_get_float(args[_i]); _i++;
  float to_a = mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(pie(c.x, c.y, from_a, to_a, r));
}

// shape.star: Star polygon. Args: centre, point count, outer & inner radius.
mp_obj_t mpy_shape_star(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_star);
#endif
  size_t _i = 0;
  vec2_t c = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 3);
  int points = (int)mp_obj_get_float(args[_i]); _i++;
  float outer = mp_obj_get_float(args[_i]); _i++;
  float inner = mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(star(c.x, c.y, points, outer, inner));
}

// shape.line: Stroked line shape of width w between two points.
mp_obj_t mpy_shape_line(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_line);
#endif
  size_t _i = 0;
  vec2_t p1 = pv::get_xy(args, &_i, n_args);
  vec2_t p2 = pv::get_xy(args, &_i, n_args);
  pv::need(n_args, _i + 1);
  float w = mp_obj_get_float(args[_i]); _i++;
  return pv::box_shape(line(p1.x, p1.y, p2.x, p2.y, w));
}

// shape.stroke: Replace this shape with its stroked outline. flags: OR of ALIGN_*/PATH_*/JOIN_*/CAP_* (each default is the 0 value). Returns self.
mp_obj_t mpy_shape_stroke(size_t n_args, const mp_obj_t *args) {
  self(args[0], shape_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_stroke);
#endif
  size_t _i = 1;
  float width = mp_obj_get_float(args[_i]); _i++;
  int flags = 0;
  if (n_args > _i) { flags = (int)mp_obj_get_float(args[_i]); _i++; }
  float miter_limit = 4.0;
  if (n_args > _i) { miter_limit = mp_obj_get_float(args[_i]); _i++; }
  self->shape->stroke(width, flags, miter_limit);
  return MP_OBJ_FROM_PTR(self);
}

// shape.grow: Replace this shape with one offset outward by amount along its edge normals, joining convex corners per join (a JOIN_* value). Winding does not matter: a positive amount always grows outward. Returns self.
mp_obj_t mpy_shape_grow(size_t n_args, const mp_obj_t *args) {
  self(args[0], shape_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_grow);
#endif
  size_t _i = 1;
  float amount = mp_obj_get_float(args[_i]); _i++;
  int join = 0;
  if (n_args > _i) { join = (int)mp_obj_get_float(args[_i]); _i++; }
  float miter_limit = 4.0;
  if (n_args > _i) { miter_limit = mp_obj_get_float(args[_i]); _i++; }
  self->shape->grow(amount, join, miter_limit);
  return MP_OBJ_FROM_PTR(self);
}

// shape.shrink: The same, inset by amount instead of outset. Returns self.
mp_obj_t mpy_shape_shrink(size_t n_args, const mp_obj_t *args) {
  self(args[0], shape_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_shrink);
#endif
  size_t _i = 1;
  float amount = mp_obj_get_float(args[_i]); _i++;
  int join = 0;
  if (n_args > _i) { join = (int)mp_obj_get_float(args[_i]); _i++; }
  float miter_limit = 4.0;
  if (n_args > _i) { miter_limit = mp_obj_get_float(args[_i]); _i++; }
  self->shape->shrink(amount, join, miter_limit);
  return MP_OBJ_FROM_PTR(self);
}

// shape.bounds: Device-space bounding box (local bbox run through the current transform).
mp_obj_t mpy_shape_bounds(size_t n_args, const mp_obj_t *args) {
  self(args[0], shape_obj_t);
#if PV_METRICS
  pv::metric_scope _pvm(PV_M_shape_bounds);
#endif
  return pv::box_rect(self->shape->bounds());
}

static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_custom_obj, 1, mpy_shape_custom);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_custom_static_obj, MP_ROM_PTR(&mpy_shape_custom_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_combine_obj, 1, mpy_shape_combine);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_combine_static_obj, MP_ROM_PTR(&mpy_shape_combine_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_regular_polygon_obj, 3, mpy_shape_regular_polygon);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_regular_polygon_static_obj, MP_ROM_PTR(&mpy_shape_regular_polygon_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_circle_obj, 2, mpy_shape_circle);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_circle_static_obj, MP_ROM_PTR(&mpy_shape_circle_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_ellipse_obj, 3, mpy_shape_ellipse);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_ellipse_static_obj, MP_ROM_PTR(&mpy_shape_ellipse_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_rectangle_obj, 1, mpy_shape_rectangle);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_rectangle_static_obj, MP_ROM_PTR(&mpy_shape_rectangle_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_rounded_rectangle_obj, 2, mpy_shape_rounded_rectangle);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_rounded_rectangle_static_obj, MP_ROM_PTR(&mpy_shape_rounded_rectangle_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_squircle_obj, 2, mpy_shape_squircle);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_squircle_static_obj, MP_ROM_PTR(&mpy_shape_squircle_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_arc_obj, 5, mpy_shape_arc);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_arc_static_obj, MP_ROM_PTR(&mpy_shape_arc_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_pie_obj, 4, mpy_shape_pie);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_pie_static_obj, MP_ROM_PTR(&mpy_shape_pie_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_star_obj, 4, mpy_shape_star);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_star_static_obj, MP_ROM_PTR(&mpy_shape_star_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_line_obj, 3, mpy_shape_line);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(mpy_shape_line_static_obj, MP_ROM_PTR(&mpy_shape_line_obj));
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_stroke_obj, 2, mpy_shape_stroke);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_grow_obj, 2, mpy_shape_grow);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_shrink_obj, 2, mpy_shape_shrink);
static MP_DEFINE_CONST_FUN_OBJ_VAR(mpy_shape_bounds_obj, 1, mpy_shape_bounds);

void shape_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, shape_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_transform:
    {
      if (action == GET) { dest[0] = pv::box_mat3(self->shape->transform); return; }
      if (action == SET) { self->shape->transform = ((mat3_obj_t *)MP_OBJ_TO_PTR(dest[1]))->m; dest[0] = MP_OBJ_NULL; return; }
      break;
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t shape_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_ALIGN_OUTER), MP_ROM_INT(ALIGN_OUTER) },
  { MP_ROM_QSTR(MP_QSTR_ALIGN_INNER), MP_ROM_INT(ALIGN_INNER) },
  { MP_ROM_QSTR(MP_QSTR_ALIGN_CENTER), MP_ROM_INT(ALIGN_CENTER) },
  { MP_ROM_QSTR(MP_QSTR_PATH_CLOSED), MP_ROM_INT(PATH_CLOSED) },
  { MP_ROM_QSTR(MP_QSTR_PATH_OPEN), MP_ROM_INT(PATH_OPEN) },
  { MP_ROM_QSTR(MP_QSTR_JOIN_MITER), MP_ROM_INT(JOIN_MITER) },
  { MP_ROM_QSTR(MP_QSTR_JOIN_ROUND), MP_ROM_INT(JOIN_ROUND) },
  { MP_ROM_QSTR(MP_QSTR_JOIN_BEVEL), MP_ROM_INT(JOIN_BEVEL) },
  { MP_ROM_QSTR(MP_QSTR_CAP_BUTT), MP_ROM_INT(CAP_BUTT) },
  { MP_ROM_QSTR(MP_QSTR_CAP_ROUND), MP_ROM_INT(CAP_ROUND) },
  { MP_ROM_QSTR(MP_QSTR_CAP_SQUARE), MP_ROM_INT(CAP_SQUARE) },
  { MP_ROM_QSTR(MP_QSTR_custom), MP_ROM_PTR(&mpy_shape_custom_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_combine), MP_ROM_PTR(&mpy_shape_combine_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_regular_polygon), MP_ROM_PTR(&mpy_shape_regular_polygon_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&mpy_shape_circle_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_ellipse), MP_ROM_PTR(&mpy_shape_ellipse_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_rectangle), MP_ROM_PTR(&mpy_shape_rectangle_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_rounded_rectangle), MP_ROM_PTR(&mpy_shape_rounded_rectangle_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_squircle), MP_ROM_PTR(&mpy_shape_squircle_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_arc), MP_ROM_PTR(&mpy_shape_arc_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_pie), MP_ROM_PTR(&mpy_shape_pie_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_star), MP_ROM_PTR(&mpy_shape_star_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&mpy_shape_line_static_obj) },
  { MP_ROM_QSTR(MP_QSTR_stroke), MP_ROM_PTR(&mpy_shape_stroke_obj) },
  { MP_ROM_QSTR(MP_QSTR_grow), MP_ROM_PTR(&mpy_shape_grow_obj) },
  { MP_ROM_QSTR(MP_QSTR_shrink), MP_ROM_PTR(&mpy_shape_shrink_obj) },
  { MP_ROM_QSTR(MP_QSTR_bounds), MP_ROM_PTR(&mpy_shape_bounds_obj) },
};
static MP_DEFINE_CONST_DICT(shape_locals_dict, shape_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
  type_shape,
  MP_QSTR_shape,
  MP_TYPE_FLAG_NONE,
  attr, (const void *)shape_attr,
  locals_dict, &shape_locals_dict
);

}
