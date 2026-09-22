// native/font_freetype.cpp — real TTF/OTF outlines, via FreeType.
//
// Only built when PV_FREETYPE is set, which is a host-class target thing: the
// embedded builds keep the .af pipeline, where outlines are decomposed and
// quantised ahead of time. This does the same decomposition at load time and
// skips the quantising, which is what large text on a big panel needs -- a
// narrow .af packs the whole em into int8, 128 steps, and that is visible by the
// time a glyph is 80px tall.
//
// The result is an ordinary font_t, built exactly as the .af parser builds one,
// so image.text(), measure_text(), the caret, wrapping and alignment are all
// unchanged: only where the outlines came from is different.
//
// Coordinates: FreeType reports font units, y-up. picovector stores y-down with
// the baseline at 0, so y is negated -- the same flip the .af packer performs.

#if PV_FREETYPE

#include <math.h>
#include <string.h>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_TRUETYPE_TABLES_H

#include "pv_bindings.hpp"

extern "C" {
  #include "py/runtime.h"
  #include "py/stream.h"
}

using namespace picovector;

namespace {

  typedef std::vector<glyph_path_point16_t> contour_t;

  struct decompose_ctx_t {
    std::vector<contour_t> contours;
    double tolerance;       // font units
    double last_x, last_y;  // current point, y already negated
  };

  void emit(decompose_ctx_t *ctx, double x, double y) {
    if (ctx->contours.empty()) return;
    glyph_path_point16_t p;
    p.x = (int16_t)lround(x);
    p.y = (int16_t)lround(y);
    contour_t &c = ctx->contours.back();
    // A repeated point is a zero-length edge: work for the rasteriser, no ink.
    if (!c.empty() && c.back().x == p.x && c.back().y == p.y) return;
    c.push_back(p);
    ctx->last_x = x;
    ctx->last_y = y;
  }

  // Squared distance of the control point from the chord, over the chord length.
  double quad_flatness(double x0, double y0, double cx, double cy,
                       double x1, double y1) {
    double dx = x1 - x0, dy = y1 - y0;
    double d = fabs((cx - x1) * dy - (cy - y1) * dx);
    double len = dx * dx + dy * dy;
    return len > 0.0 ? d * d / len
                     : (cx - x0) * (cx - x0) + (cy - y0) * (cy - y0);
  }

  void flatten_quad(decompose_ctx_t *ctx, double x0, double y0, double cx, double cy,
                    double x1, double y1, int depth) {
    if (depth >= 10 ||
        quad_flatness(x0, y0, cx, cy, x1, y1) <= ctx->tolerance * ctx->tolerance) {
      emit(ctx, x1, y1);
      return;
    }
    double ax = (x0 + cx) * .5, ay = (y0 + cy) * .5;
    double bx = (cx + x1) * .5, by = (cy + y1) * .5;
    double mx = (ax + bx) * .5, my = (ay + by) * .5;
    flatten_quad(ctx, x0, y0, ax, ay, mx, my, depth + 1);
    flatten_quad(ctx, mx, my, bx, by, x1, y1, depth + 1);
  }

  void flatten_cubic(decompose_ctx_t *ctx, double x0, double y0,
                     double c1x, double c1y, double c2x, double c2y,
                     double x1, double y1, int depth) {
    double dx = x1 - x0, dy = y1 - y0;
    double d1 = fabs((c1x - x1) * dy - (c1y - y1) * dx);
    double d2 = fabs((c2x - x1) * dy - (c2y - y1) * dx);
    double dd = (d1 + d2) * (d1 + d2);
    if (depth >= 10 ||
        dd <= ctx->tolerance * ctx->tolerance * (dx * dx + dy * dy)) {
      emit(ctx, x1, y1);
      return;
    }
    double x01 = (x0 + c1x) * .5,  y01 = (y0 + c1y) * .5;
    double x12 = (c1x + c2x) * .5, y12 = (c1y + c2y) * .5;
    double x23 = (c2x + x1) * .5,  y23 = (c2y + y1) * .5;
    double xa = (x01 + x12) * .5,  ya = (y01 + y12) * .5;
    double xb = (x12 + x23) * .5,  yb = (y12 + y23) * .5;
    double xm = (xa + xb) * .5,    ym = (ya + yb) * .5;
    flatten_cubic(ctx, x0, y0, x01, y01, xa, ya, xm, ym, depth + 1);
    flatten_cubic(ctx, xm, ym, xb, yb, x23, y23, x1, y1, depth + 1);
  }

  int cb_move_to(const FT_Vector *to, void *user) {
    decompose_ctx_t *ctx = (decompose_ctx_t *)user;
    ctx->contours.push_back(contour_t());
    ctx->last_x = (double)to->x;
    ctx->last_y = -(double)to->y;
    glyph_path_point16_t p;
    p.x = (int16_t)lround(ctx->last_x);
    p.y = (int16_t)lround(ctx->last_y);
    ctx->contours.back().push_back(p);
    return 0;
  }

  int cb_line_to(const FT_Vector *to, void *user) {
    emit((decompose_ctx_t *)user, (double)to->x, -(double)to->y);
    return 0;
  }

  int cb_conic_to(const FT_Vector *control, const FT_Vector *to, void *user) {
    decompose_ctx_t *ctx = (decompose_ctx_t *)user;
    flatten_quad(ctx, ctx->last_x, ctx->last_y,
                 (double)control->x, -(double)control->y,
                 (double)to->x, -(double)to->y, 0);
    return 0;
  }

  int cb_cubic_to(const FT_Vector *c1, const FT_Vector *c2, const FT_Vector *to,
                  void *user) {
    decompose_ctx_t *ctx = (decompose_ctx_t *)user;
    flatten_cubic(ctx, ctx->last_x, ctx->last_y,
                  (double)c1->x, -(double)c1->y,
                  (double)c2->x, -(double)c2->y,
                  (double)to->x, -(double)to->y, 0);
    return 0;
  }

  struct built_glyph_t {
    uint16_t codepoint;
    int16_t x, y, w, h, advance;
    std::vector<contour_t> contours;
  };

  FT_Library ft_library = NULL;

  // Printable ASCII plus the Latin-1 supplement, which carries the degree sign,
  // the accented letters and the currency marks a UI actually reaches for.
  void default_charset(std::vector<uint32_t> &out) {
    for (uint32_t c = 0x20; c <= 0x7e; c++) out.push_back(c);
    for (uint32_t c = 0xa0; c <= 0xff; c++) out.push_back(c);
  }

  void charset_from_str(const char *s, std::vector<uint32_t> &out) {
    for (const unsigned char *p = (const unsigned char *)s; *p;) {
      uint32_t cp = *p;
      int len = 1;
      if (cp >= 0xf0)      { cp &= 0x07; len = 4; }
      else if (cp >= 0xe0) { cp &= 0x0f; len = 3; }
      else if (cp >= 0xc0) { cp &= 0x1f; len = 2; }
      for (int i = 1; i < len; i++) cp = (cp << 6) | (p[i] & 0x3f);
      p += len;
      out.push_back(cp);
    }
  }

  // Apply named variable-font axes, e.g. {"wght": 700, "wdth": 87.5}.
  void apply_variations(FT_Face face, mp_obj_t variations, const char *path) {
    if (variations == mp_const_none) return;
    if ((face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS) == 0) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("'%s' is not a variable font"), path);
    }
    FT_MM_Var *mm = NULL;
    if (FT_Get_MM_Var(face, &mm) || mm == NULL) return;

    std::vector<FT_Fixed> coords(mm->num_axis);
    for (FT_UInt i = 0; i < mm->num_axis; i++) coords[i] = mm->axis[i].def;

    // Walk the mapping's keys, so a dict or anything dict-like works.
    mp_obj_t iterable = mp_getiter(variations, NULL);
    mp_obj_t key;
    while ((key = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
      const char *tag_str = mp_obj_str_get_str(key);
      mp_obj_t value = mp_obj_subscr(variations, key, MP_OBJ_SENTINEL);
      double want = mp_obj_get_float(value);
      bool matched = false;
      for (FT_UInt i = 0; i < mm->num_axis; i++) {
        char tag[5] = {
          (char)((mm->axis[i].tag >> 24) & 0xff), (char)((mm->axis[i].tag >> 16) & 0xff),
          (char)((mm->axis[i].tag >> 8) & 0xff),  (char)(mm->axis[i].tag & 0xff), 0
        };
        if (strcmp(tag, tag_str) == 0) {
          FT_Fixed v = (FT_Fixed)lround(want * 65536.0);
          if (v < mm->axis[i].minimum) v = mm->axis[i].minimum;
          if (v > mm->axis[i].maximum) v = mm->axis[i].maximum;
          coords[i] = v;
          matched = true;
          break;
        }
      }
      if (!matched) {
        FT_Done_MM_Var(ft_library, mm);
        mp_raise_msg_varg(&mp_type_ValueError,
                          MP_ERROR_TEXT("'%s' has no axis '%s'"), path, tag_str);
      }
    }
    FT_Set_Var_Design_Coordinates(face, (FT_UInt)coords.size(), &coords[0]);
    FT_Done_MM_Var(ft_library, mm);
  }

} // namespace

extern "C" {

  // TrueType (0x00010000 / "true"), collections ("ttcf") and CFF/OTF ("OTTO").
  bool pv_ft_is_font_marker(const char marker[4]) {
    static const char sfnt[4] = { 0x00, 0x01, 0x00, 0x00 };
    return memcmp(marker, sfnt, 4) == 0 || memcmp(marker, "true", 4) == 0 ||
           memcmp(marker, "ttcf", 4) == 0 || memcmp(marker, "OTTO", 4) == 0;
  }

  // `file` is an open stream positioned at 0; consumed here.
  mp_obj_t pv_ft_load(mp_obj_t file, const char *path, mp_obj_t chars_obj,
                      mp_obj_t tolerance_obj, mp_obj_t variations) {
    // FreeType needs the whole face in memory, and the stream may be a VFS file
    // with no OS path, so slurp it rather than handing FreeType a filename.
    vstr_t data;
    vstr_init(&data, 64 * 1024);
    for (;;) {
      vstr_hint_size(&data, 64 * 1024);
      int error;
      mp_uint_t got = mp_stream_rw(file, data.buf + data.len,
                                   data.alloc - data.len, &error, 0);
      if (error) {
        vstr_clear(&data);
        mp_stream_close(file);
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("'%s' read failed"), path);
      }
      if (got == 0) break;
      data.len += got;
    }
    mp_stream_close(file);

    if (ft_library == NULL && FT_Init_FreeType(&ft_library)) {
      vstr_clear(&data);
      mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("FreeType init failed"));
    }

    FT_Face face;
    if (FT_New_Memory_Face(ft_library, (const FT_Byte *)data.buf,
                           (FT_Long)data.len, 0, &face)) {
      vstr_clear(&data);
      mp_raise_msg_varg(&mp_type_OSError,
                        MP_ERROR_TEXT("'%s' could not be read as a font"), path);
    }

    apply_variations(face, variations, path);

    float upem = (float)face->units_per_EM;
    if (upem <= 0.0f) upem = 1000.0f;

    // Under ~0.3px of error when drawn at 300px. Finer costs points per glyph,
    // which the rasteriser pays for on every frame the text is drawn.
    double tolerance = (tolerance_obj == mp_const_none)
                           ? (double)upem / 1000.0
                           : mp_obj_get_float(tolerance_obj);
    if (tolerance <= 0.0) tolerance = 1.0;

    std::vector<uint32_t> charset;
    if (chars_obj == mp_const_none) default_charset(charset);
    else charset_from_str(mp_obj_str_get_str(chars_obj), charset);

    std::vector<built_glyph_t> built;
    size_t total_paths = 0, total_points = 0;

    for (size_t ci = 0; ci < charset.size(); ci++) {
      uint32_t cp = charset[ci];
      if (cp > 0xffff) continue;  // glyph_t.codepoint is uint16_t

      FT_UInt index = FT_Get_Char_Index(face, cp);
      if (index == 0) continue;
      if (FT_Load_Glyph(face, index,
                        FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING)) {
        continue;
      }

      decompose_ctx_t ctx;
      ctx.tolerance = tolerance;
      ctx.last_x = ctx.last_y = 0.0;

      FT_Outline_Funcs funcs;
      funcs.move_to = cb_move_to;
      funcs.line_to = cb_line_to;
      funcs.conic_to = cb_conic_to;
      funcs.cubic_to = cb_cubic_to;
      funcs.shift = 0;
      funcs.delta = 0;
      if (FT_Outline_Decompose(&face->glyph->outline, &funcs, &ctx)) continue;

      built_glyph_t g;
      g.codepoint = (uint16_t)cp;
      g.advance = (int16_t)face->glyph->metrics.horiAdvance;

      FT_BBox box;
      FT_Outline_Get_CBox(&face->glyph->outline, &box);
      g.x = (int16_t)box.xMin;
      g.y = (int16_t)box.yMin;   // stored y-up; glyph_t::bounds reads it back as -y
      g.w = (int16_t)(box.xMax - box.xMin);
      g.h = (int16_t)(box.yMax - box.yMin);

      for (size_t i = 0; i < ctx.contours.size(); i++) {
        if (ctx.contours[i].size() >= 3) g.contours.push_back(ctx.contours[i]);
      }
      if (g.contours.size() > 255) g.contours.resize(255);  // path_count is uint8_t

      total_paths += g.contours.size();
      for (size_t i = 0; i < g.contours.size(); i++) {
        total_points += g.contours[i].size();
      }
      built.push_back(g);
    }

    FT_Done_Face(face);
    vstr_clear(&data);

    if (built.empty()) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("no glyphs decoded from '%s'"), path);
    }

    // One block, laid out as the .af parser lays its own out: the glyph table,
    // then the contour table, then the points. Every pointer stays inside the
    // block, so it needs no scanning.
    size_t glyph_bytes = built.size() * sizeof(glyph_t);
    size_t path_bytes  = total_paths * sizeof(glyph_path_t);
    size_t point_bytes = total_points * sizeof(glyph_path_point16_t);
    size_t total = glyph_bytes + path_bytes + point_bytes;

    uint8_t *block = (uint8_t *)m_malloc_no_scan(total);
    glyph_t *glyphs = (glyph_t *)block;
    glyph_path_t *paths = (glyph_path_t *)(block + glyph_bytes);
    glyph_path_point16_t *points =
        (glyph_path_point16_t *)(block + glyph_bytes + path_bytes);

    glyph_path_t *next_path = paths;
    glyph_path_point16_t *next_point = points;
    for (size_t i = 0; i < built.size(); i++) {
      built_glyph_t &b = built[i];
      glyph_t *g = &glyphs[i];
      g->codepoint = b.codepoint;
      g->x = b.x; g->y = b.y; g->w = b.w; g->h = b.h;
      g->advance = b.advance;
      g->path_count = (uint8_t)b.contours.size();
      g->paths = next_path;
      for (size_t j = 0; j < b.contours.size(); j++) {
        contour_t &c = b.contours[j];
        next_path->point_count = (uint16_t)c.size();
        next_path->points = next_point;
        memcpy(next_point, &c[0], c.size() * sizeof(glyph_path_point16_t));
        next_point += c.size();
        next_path++;
      }
    }

    vector_font_obj_t *o = mp_obj_malloc(vector_font_obj_t, &type_vector_font);
    o->font.glyph_count = (int)built.size();
    o->font.glyphs = glyphs;
    o->font.units_per_em = upem;
    o->font.wide_points = true;
    o->font.nonzero_fill = true;   // TTF/OTF outlines are authored non-zero
    o->buffer = block;
    o->buffer_size = (uint32_t)total;
    size_t path_len = strlen(path) + 1;
    o->path = (char *)m_malloc_no_scan(path_len);
    memcpy(o->path, path, path_len);
    return MP_OBJ_FROM_PTR(o);
  }

} // extern "C"

#endif // PV_FREETYPE
