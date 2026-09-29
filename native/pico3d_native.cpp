// native/pico3d_native.cpp — hand-written bodies for the pico3d types.
//
// The engine works in plain pointer views (pico3d_target_t / pico3d_texture_t /
// pico3d_mesh_t), so this file is the whole boundary between it and MicroPython:
// reading buffers and images into those views, rooting them so the GC cannot
// free what the engine is reading, and owning the depth buffer and per-vertex
// scratch a surface renders through. The generated files provide the type
// definitions, locals dicts and attr handlers.

#include "pv_bindings.hpp"
#include "types.h"
#include "picovector_working_buffer.h"   // the banded depth strip lives here
#include <cstring>

extern "C" {
  #include "py/runtime.h"

  namespace {

    // A borrowed float array, as a pointer + element count. Buffers are borrowed
    // rather than copied, so the caller's array stays writable and a deforming
    // mesh needs no rebuild.
    const float *read_floats(mp_obj_t o, size_t *count) {
      mp_buffer_info_t bi;
      mp_get_buffer_raise(o, &bi, MP_BUFFER_READ);
      *count = bi.len / sizeof(float);
      return (const float *)bi.buf;
    }

    // An optional float array of `per` values a vertex. Refuses one that is too
    // short for the mesh: the engine indexes it by vertex with no bounds check,
    // so a short array reads off the end of the buffer.
    const float *read_vertex_floats(mp_obj_t o, uint32_t vertices, int per,
                                    const char *what) {
      if (o == mp_const_none) return nullptr;
      size_t n;
      const float *p = read_floats(o, &n);
      if (n < (size_t)vertices * per) {
        mp_raise_msg_varg(&mp_type_ValueError,
                          MP_ERROR_TEXT("%s is short for the vertex count"), what);
      }
      return p;
    }

    // Fill a material's texture view from an image argument, rooting the image.
    // False when the argument is None, leaving the view untouched.
    bool read_texture(mp_obj_t o, pico3d_texture_t &tv, mp_obj_t *root) {
      if (o == mp_const_none) return false;
      pv::pico3d_texture_of(o, tv);
      *root = o;
      return true;
    }

  }

  // ── mesh ──────────────────────────────────────────────────────────────────
  mp_obj_t mesh_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                              size_t n_kw, const mp_obj_t *args) {
    enum { ARG_positions, ARG_indices, ARG_normals, ARG_uvs, ARG_colors, ARG_tangents };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_positions, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_indices,   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_normals,   MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_uvs,       MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_colors,    MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_tangents,  MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);

    // mp_obj_malloc zeroes the object, so every optional pointer and root starts
    // null; only what was supplied is filled in.
    mesh_obj_t *self = mp_obj_malloc(mesh_obj_t, type);
    self->normals_ref = self->uvs_ref = mp_const_none;
    self->colors_ref = self->tangents_ref = mp_const_none;
    pico3d_mesh_t &m = self->mesh;

    size_t n;
    m.positions = read_floats(vals[ARG_positions].u_obj, &n);
    m.vertex_count = n / 3;
    self->positions_ref = vals[ARG_positions].u_obj;

    mp_buffer_info_t bi;
    mp_get_buffer_raise(vals[ARG_indices].u_obj, &bi, MP_BUFFER_READ);
    m.indices = (const uint16_t *)bi.buf;
    m.triangle_count = (bi.len / sizeof(uint16_t)) / 3;
    self->indices_ref = vals[ARG_indices].u_obj;

    if (m.vertex_count == 0 || m.triangle_count == 0) {
      mp_raise_msg(&mp_type_ValueError,
                   MP_ERROR_TEXT("a mesh needs at least one triangle"));
    }
    // Every index is dereferenced by the transform pass without a bounds check,
    // so one out-of-range index reads (and lights) whatever follows the vcache.
    for (uint32_t i = 0, e = m.triangle_count * 3; i < e; i++) {
      if (m.indices[i] >= m.vertex_count) {
        mp_raise_msg(&mp_type_ValueError,
                     MP_ERROR_TEXT("an index is past the end of positions"));
      }
    }

    m.normals = read_vertex_floats(vals[ARG_normals].u_obj, m.vertex_count, 3, "normals");
    if (m.normals) self->normals_ref = vals[ARG_normals].u_obj;
    m.uvs = read_vertex_floats(vals[ARG_uvs].u_obj, m.vertex_count, 2, "uvs");
    if (m.uvs) self->uvs_ref = vals[ARG_uvs].u_obj;
    m.tangents = read_vertex_floats(vals[ARG_tangents].u_obj, m.vertex_count, 3, "tangents");
    if (m.tangents) self->tangents_ref = vals[ARG_tangents].u_obj;

    if (vals[ARG_colors].u_obj != mp_const_none) {
      mp_buffer_info_t cbi;
      mp_get_buffer_raise(vals[ARG_colors].u_obj, &cbi, MP_BUFFER_READ);
      if (cbi.len / sizeof(uint32_t) < m.vertex_count) {
        mp_raise_msg(&mp_type_ValueError,
                     MP_ERROR_TEXT("colors is short for the vertex count"));
      }
      m.colors = (const uint32_t *)cbi.buf;
      self->colors_ref = vals[ARG_colors].u_obj;
    }
    // Measure the model-space box once, here, so pico3d_cull_mesh can reject a
    // whole mesh before a vertex is transformed. Without it has_bounds stays
    // clear and nothing is ever culled.
    pico3d_mesh_bounds(&m);
    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t mesh_update_bounds(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    self(args[0], mesh_obj_t);
    pico3d_mesh_bounds(&self->mesh);
    return mp_const_none;
  }

  // ── material ──────────────────────────────────────────────────────────────
  mp_obj_t material_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                                  size_t n_kw, const mp_obj_t *args) {
    enum { ARG_color, ARG_texture, ARG_shading, ARG_filter, ARG_double_sided,
           ARG_alpha_cutoff, ARG_normal_map, ARG_matcap, ARG_specular, ARG_shininess };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_color,        MP_ARG_OBJ,  {.u_obj = mp_const_none} },
      { MP_QSTR_texture,      MP_ARG_OBJ,  {.u_obj = mp_const_none} },
      { MP_QSTR_shading,      MP_ARG_INT,  {.u_int = PICO3D_FLAT} },
      { MP_QSTR_filter,       MP_ARG_INT,  {.u_int = PICO3D_NEAREST} },
      { MP_QSTR_double_sided, MP_ARG_BOOL, {.u_bool = false} },
      { MP_QSTR_alpha_cutoff, MP_ARG_INT,  {.u_int = 128} },
      { MP_QSTR_normal_map,   MP_ARG_OBJ,  {.u_obj = mp_const_none} },
      { MP_QSTR_matcap,       MP_ARG_OBJ,  {.u_obj = mp_const_none} },
      { MP_QSTR_specular,     MP_ARG_OBJ,  {.u_obj = mp_const_none} },
      { MP_QSTR_shininess,    MP_ARG_INT,  {.u_int = 32} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);

    material_obj_t *self = mp_obj_malloc(material_obj_t, type);
    self->texture_ref = self->normal_map_ref = self->matcap_ref = mp_const_none;
    pico3d_material_t &m = self->mat;

    m.color = vals[ARG_color].u_obj == mp_const_none
              ? pico3d_rgb(255, 255, 255) : pv::pico3d_rgb_of(vals[ARG_color].u_obj);
    m.specular = vals[ARG_specular].u_obj == mp_const_none
                 ? 0 : pv::pico3d_rgb_of(vals[ARG_specular].u_obj);
    m.shininess = vals[ARG_shininess].u_int;
    m.filter = (pico3d_filter_t)vals[ARG_filter].u_int;
    m.double_sided = vals[ARG_double_sided].u_bool;
    m.alpha_cutoff = (uint8_t)vals[ARG_alpha_cutoff].u_int;
    self->shading = (pico3d_shading_t)vals[ARG_shading].u_int;

    // The engine takes each texture as a pointer, so the views have to live in
    // the obj rather than on this stack frame.
    if (read_texture(vals[ARG_texture].u_obj, self->tex, &self->texture_ref)) {
      m.texture = &self->tex;
    }
    if (read_texture(vals[ARG_normal_map].u_obj, self->nmap, &self->normal_map_ref)) {
      m.normal_map = &self->nmap;
    }
    if (read_texture(vals[ARG_matcap].u_obj, self->mcap, &self->matcap_ref)) {
      m.matcap = &self->mcap;
    }
    return MP_OBJ_FROM_PTR(self);
  }

  // ── light ─────────────────────────────────────────────────────────────────
  mp_obj_t light_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                               size_t n_kw, const mp_obj_t *args) {
    enum { ARG_direction, ARG_color, ARG_ambient, ARG_position, ARG_atten };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_direction, MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_color,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_ambient,   MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_position,  MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_atten,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);

    auto get_vec3 = [](mp_obj_t o) -> vec3_t {
      if (!mp_obj_is_type(o, &type_vec3)) {
        mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("expected a vec3"));
      }
      return ((vec3_obj_t *)MP_OBJ_TO_PTR(o))->v;
    };

    light_obj_t *self = mp_obj_malloc(light_obj_t, type);
    pico3d_light_t &l = self->light;
    l.direction = vals[ARG_direction].u_obj == mp_const_none
                  ? vec3_t(0, 0, -1) : get_vec3(vals[ARG_direction].u_obj);
    l.color = vals[ARG_color].u_obj == mp_const_none
              ? pico3d_rgb(255, 255, 255) : pv::pico3d_rgb_of(vals[ARG_color].u_obj);
    l.ambient = vals[ARG_ambient].u_obj == mp_const_none
                ? 0 : pv::pico3d_rgb_of(vals[ARG_ambient].u_obj);
    // A position is what makes it a point light; atten only means anything then.
    if (vals[ARG_position].u_obj != mp_const_none) {
      l.point = 1;
      l.position = get_vec3(vals[ARG_position].u_obj);
      l.atten = vals[ARG_atten].u_obj == mp_const_none
                ? 1.0f : mp_obj_get_float(vals[ARG_atten].u_obj);
    }
    return MP_OBJ_FROM_PTR(self);
  }

  // ── surface ───────────────────────────────────────────────────────────────
  // The depth buffer is always picovector's working buffer: on-chip SRAM, and
  // idle while a 3D pass runs. The heap is PSRAM, where a depth buffer read and
  // written for every pixel stepped over is ruinous. So the pool's size decides
  // how the surface bands - as many rows as fit make one band, and the surface
  // takes however many bands that leaves. A surface that fits whole is one band.
  static void surface_size_bands(surface_obj_t *self) {
    int fit = (int)(working_buffer_size / ((size_t)self->w * sizeof(uint16_t)));
    if (fit < 1) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("a %d px wide surface needs %d bytes a depth row, have %d"),
                        self->w, (int)(self->w * sizeof(uint16_t)), (int)working_buffer_size);
    }
    self->band_rows = fit < self->h ? fit : self->h;
    self->bands = (self->h + self->band_rows - 1) / self->band_rows;
    self->depth = (uint16_t *)PicoVector_working_buffer;
  }

  // Fill a render-target view from the wrapped image, reallocating the depth
  // buffer if the image has been resized (a window()ed view can hand back a
  // different size) since the surface was built.
  void surface_view(surface_obj_t *self, pico3d_target_t *t) {
    image_t *im = self->source->image;
    rect_t b = im->bounds(), cl = im->clip();
    int w = (int)b.w, h = (int)b.h;
    if (w != self->w || h != self->h) {
      self->w = w;
      self->h = h;
      surface_size_bands(self);
    }
    t->color = (uint32_t *)im->ptr(0, 0);
    t->color565 = im->pixel_format() == RGB565;
    t->depth = self->depth;
    t->width = w;
    t->height = h;
    t->color_stride = im->row_stride() / im->bytes_per_pixel();
    t->depth_stride = w;
    // The image's clip rect bounds the render, so a 3D viewport is framed the
    // same way anything else is.
    t->clip_x0 = (int)cl.x;
    t->clip_y0 = (int)cl.y;
    t->clip_x1 = (int)(cl.x + cl.w);
    t->clip_y1 = (int)(cl.y + cl.h);
    t->fog = self->fog;
    t->fog_near = self->fog_near;
    t->fog_far = self->fog_far;
    // Whole surface, every row, one core: pico3d_scene_draw walks depth_y0 down
    // the bands itself and splits the rows between cores. Both are read on every
    // path, so leaving them to a stack target's garbage renders nothing.
    t->depth_y0 = 0;
    t->row_step = 1;
    t->row_phase = 0;
  }

  mp_obj_t surface_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *args) {
    enum { ARG_image };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_image, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);

    if (!pv::is_image(vals[ARG_image].u_obj)) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("surface() expects an image"));
    }
    image_obj_t *img = (image_obj_t *)MP_OBJ_TO_PTR(vals[ARG_image].u_obj);
    // has_palette() is separate from the format: a palettised image reports
    // RGBA8888 for its colour table while its pixels are one byte of index, so
    // writing RGBA words into it would run four times past the end. RGB565 (the
    // platform framebuffer) is fine: the rasteriser packs its stores for it.
    if ((img->image->pixel_format() != RGBA8888 && img->image->pixel_format() != RGB565)
        || img->image->has_palette()) {
      mp_raise_msg(&mp_type_ValueError,
                   MP_ERROR_TEXT("pico3d needs an RGBA or RGB565 image"));
    }
    rect_t b = img->image->bounds();

    surface_obj_t *self = mp_obj_malloc(surface_obj_t, type);
    self->source = img;
    self->w = (int)b.w;
    self->h = (int)b.h;
    surface_size_bands(self);
    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t surface_clear_depth(size_t n_args, const mp_obj_t *args) {
    self(args[0], surface_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_surface_clear_depth);
#endif
    uint16_t value = n_args > 1 ? (uint16_t)mp_obj_get_int(args[1]) : 0xFFFF;
    // band_rows rows, not h: on a banded surface that is the whole buffer, and
    // draw() clears each band for itself anyway.
    pico3d_target_t t{};
    t.depth = self->depth;
    t.width = self->w;
    t.height = self->band_rows;
    t.depth_stride = self->w;
    t.clip_x1 = self->w;
    t.clip_y1 = self->band_rows;
    pico3d_depth_clear(&t, value);
    return mp_const_none;
  }

  mp_obj_t surface_render(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], surface_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_surface_render);
#endif
    enum { ARG_mesh, ARG_model, ARG_view_proj, ARG_material, ARG_light, ARG_depth, ARG_view };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_mesh,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_model,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_view_proj, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_material,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_light,     MP_ARG_OBJ,  {.u_obj = mp_const_none} },
      { MP_QSTR_depth,     MP_ARG_BOOL, {.u_bool = true} },
      { MP_QSTR_view,      MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args,
                     MP_ARRAY_SIZE(allowed), allowed, vals);

    auto need = [](mp_obj_t o, const mp_obj_type_t *want, mp_rom_error_text_t msg) -> void * {
      if (!mp_obj_is_type(o, want)) mp_raise_msg(&mp_type_TypeError, msg);
      return MP_OBJ_TO_PTR(o);
    };

    mesh_obj_t *mesh = (mesh_obj_t *)need(vals[ARG_mesh].u_obj, &type_mesh,
                                          MP_ERROR_TEXT("mesh must be a pico3d.mesh"));
    mat4_obj_t *model = (mat4_obj_t *)need(vals[ARG_model].u_obj, &type_mat4,
                                           MP_ERROR_TEXT("model must be a pico3d.mat4"));
    mat4_obj_t *vp = (mat4_obj_t *)need(vals[ARG_view_proj].u_obj, &type_mat4,
                                        MP_ERROR_TEXT("view_proj must be a pico3d.mat4"));
    material_obj_t *mat = (material_obj_t *)need(vals[ARG_material].u_obj, &type_material,
                                                 MP_ERROR_TEXT("material must be a pico3d.material"));
    pico3d_light_t *light = nullptr;
    if (vals[ARG_light].u_obj != mp_const_none) {
      light = &((light_obj_t *)need(vals[ARG_light].u_obj, &type_light,
                                    MP_ERROR_TEXT("light must be a pico3d.light")))->light;
    }
    const mat4_t *view = nullptr;
    if (vals[ARG_view].u_obj != mp_const_none) {
      view = &((mat4_obj_t *)need(vals[ARG_view].u_obj, &type_mat4,
                                  MP_ERROR_TEXT("view must be a pico3d.mat4")))->m;
    }

    // The transform cache holds one entry a vertex and is reused every frame, so
    // it only grows - a scene of several meshes settles on the largest.
    if (mesh->mesh.vertex_count > self->vcache_cap) {
      self->vcache = m_new(pico3d_vcache_t, mesh->mesh.vertex_count);
      self->vcache_cap = mesh->mesh.vertex_count;
    }

    pico3d_target_t t;
    surface_view(self, &t);
    // depth=False drops the Z-buffer for this call, and with it any need to band.
    if (!vals[ARG_depth].u_bool) t.depth = nullptr;

    // A surface whose depth fits the working buffer whole draws immediately, and
    // its depth persists from call to call until clear_depth().
    if (self->bands == 1 || !t.depth) {
      int drawn = pico3d_draw_mesh(&t, &mesh->mesh, &model->m, &vp->m, &mat->mat,
                                   mat->shading, light, self->vcache, view);
      return mp_obj_new_int(drawn);
    }

    // Otherwise the depth strip is one band tall, so the mesh goes through a
    // scene of one: transformed once, then rasterised a band at a time with each
    // band clearing its own strip. The mesh depth-tests against itself, but not
    // against earlier render() calls - that needs a scene holding all of them.
    uint32_t nt = mesh->mesh.triangle_count;
    if (nt > self->tri_cap) {
      self->tri_ys = (int16_t *)m_malloc_no_scan(sizeof(int16_t) * 2 * nt);
      self->tri_bin = (uint16_t *)m_malloc_no_scan(sizeof(uint16_t) * nt);
      self->tri_bin1 = (uint16_t *)m_malloc_no_scan(sizeof(uint16_t) * nt);
      self->tri_cap = nt;
    }
    pico3d_sub_t sub;
    pico3d_scene_t sc{};
    sc.subs = &sub;             sc.sub_cap = 1;
    sc.verts = self->vcache;    sc.vert_cap = self->vcache_cap;
    sc.ys = self->tri_ys;       sc.tri_cap = self->tri_cap;
    sc.bin = self->tri_bin;     sc.bin_cap = self->tri_cap;
    sc.bin1 = self->tri_bin1;
    pico3d_scene_reset(&sc);
    pico3d_scene_add(&sc, &t, &mesh->mesh, &model->m, &vp->m, &mat->mat,
                     mat->shading, light, view);
    t.depth_y0 = t.clip_y0;     // as draw(): a clip shorter than a band still lands in the strip
    int drawn = pico3d_scene_draw(&sc, &t, self->band_rows);
    return mp_obj_new_int(drawn);
  }

  mp_obj_t surface_draw(size_t n_args, const mp_obj_t *args) {
    self(args[0], surface_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_surface_draw);
#endif
    if (!mp_obj_is_type(args[1], &type_scene)) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("draw() expects a pico3d.scene"));
    }
    scene_obj_t *sc = (scene_obj_t *)MP_OBJ_TO_PTR(args[1]);
    // add() projected the geometry for this surface's viewport, so replaying it
    // through another one would put every triangle in the wrong place.
    if (sc->target != self) {
      mp_raise_msg(&mp_type_ValueError,
                   MP_ERROR_TEXT("scene belongs to a different surface"));
    }
    uint16_t clear_to = n_args > 2 ? (uint16_t)mp_obj_get_int(args[2]) : 0xFFFF;

    pico3d_target_t t;
    surface_view(self, &t);
    // A one-band buffer is as tall as a band, so its row 0 is the first row
    // drawn. Banding overwrites this per band; it matters only when the clip is
    // shorter than a band, because then nothing bands and the rows still have to
    // land inside the short buffer. A full-height buffer keeps render()'s
    // mapping, so the two can share a surface.
    if (self->bands > 1) t.depth_y0 = t.clip_y0;
    int drawn = pico3d_scene_draw(&sc->scene, &t,
                                  self->bands > 1 ? self->band_rows : 0, clear_to);
    return mp_obj_new_int(drawn);
  }

  // ── the SRAM scene arena ──────────────────────────────────────────────────
  // A board that has spare on-chip RAM (the Tufty freed 150 KB by storing its
  // framebuffer as RGB565) sets PICO3D_SRAM_ARENA_SIZE, and one scene at a
  // time gets its arrays from this pool instead of the PSRAM heap: the
  // transform then writes, and the extents / binning / assembly passes read,
  // single-cycle SRAM instead of paying the QSPI round trip per cache line.
  // Claimed whole by the first scene that fits, released by its finaliser; a
  // scene that does not fit (or arrives second) falls back to the heap and
  // simply runs at PSRAM speed.
#ifndef PICO3D_SRAM_ARENA_SIZE
#define PICO3D_SRAM_ARENA_SIZE 0
#endif
#if PICO3D_SRAM_ARENA_SIZE
  static uint8_t __attribute__((section(".uninitialized_data"), aligned(8)))
      pico3d_sram_arena[PICO3D_SRAM_ARENA_SIZE];
  static bool pico3d_sram_claimed = false;
#endif

  mp_obj_t scene__del__(mp_obj_t self_in) {
#if PICO3D_SRAM_ARENA_SIZE
    self(self_in, scene_obj_t);
    if (self->owns_sram) {
      self->owns_sram = false;
      pico3d_sram_claimed = false;
    }
#else
    (void)self_in;
#endif
    return mp_const_none;
  }


  // ── scene ─────────────────────────────────────────────────────────────────
  mp_obj_t scene_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                               size_t n_kw, const mp_obj_t *args) {
    enum { ARG_surface, ARG_meshes, ARG_vertices, ARG_triangles };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_surface,   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_meshes,    MP_ARG_INT, {.u_int = 16} },
      { MP_QSTR_vertices,  MP_ARG_INT, {.u_int = 512} },
      { MP_QSTR_triangles, MP_ARG_INT, {.u_int = 512} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);

    if (!mp_obj_is_type(vals[ARG_surface].u_obj, &type_surface)) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("scene() expects a pico3d.surface"));
    }
    if (vals[ARG_meshes].u_int < 1 || vals[ARG_vertices].u_int < 1 ||
        vals[ARG_triangles].u_int < 1) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("scene capacities must be positive"));
    }

    scene_obj_t *self = mp_obj_malloc_with_finaliser(scene_obj_t, type);
    self->target = (surface_obj_t *)MP_OBJ_TO_PTR(vals[ARG_surface].u_obj);
    self->owns_sram = false;
    pico3d_scene_t &sc = self->scene;
    sc.sub_cap  = (uint32_t)vals[ARG_meshes].u_int;
    sc.vert_cap = (uint32_t)vals[ARG_vertices].u_int;
    sc.tri_cap  = (uint32_t)vals[ARG_triangles].u_int;
    // A band concatenates every submission's live triangles into one bin without
    // a bounds check, so it has to hold the whole scene's worth.
    sc.bin_cap  = sc.tri_cap;

    // Lay the scene out in the SRAM arena when it fits: the small fixed arrays
    // (row extents, both bins) first, then everything left is the vertex
    // region - per-frame entries grow up it while the engine caches index
    // copies down from its tail. Worth claiming only if the region could hold
    // vert_cap of the smallest entries; otherwise (mona_hi-sized scenes, or a
    // second scene) everything comes from the heap at PSRAM speed as before.
    sc.vert_arena_bytes = 0;
    sc.cache_low = 0;
    sc.cache_count = 0;
    const size_t ys_bytes   = (sizeof(int16_t) * 2 * sc.tri_cap + 7) & ~(size_t)7;
    const size_t bin_bytes  = (sizeof(uint16_t) * sc.bin_cap + 7) & ~(size_t)7;
    bool pooled = false;
#if PICO3D_SRAM_ARENA_SIZE
    {
      const size_t fixed = ys_bytes + 2 * bin_bytes;
      if (!pico3d_sram_claimed && fixed < (size_t)PICO3D_SRAM_ARENA_SIZE) {
        const size_t vert_region = (size_t)PICO3D_SRAM_ARENA_SIZE - fixed;
        if (vert_region >= (size_t)sc.vert_cap * (size_t)offsetof(pico3d_vcache_t, w)) {
          pico3d_sram_claimed = self->owns_sram = true;
          uint8_t *at = pico3d_sram_arena;
          sc.ys   = (int16_t *)at;   at += ys_bytes;
          sc.bin  = (uint16_t *)at;  at += bin_bytes;
          sc.bin1 = (uint16_t *)at;  at += bin_bytes;
          sc.verts = (pico3d_vcache_t *)at;
          sc.vert_arena_bytes = (uint32_t)vert_region;
          pooled = true;
        }
      }
    }
#endif
    sc.subs = m_new(pico3d_sub_t, sc.sub_cap);
    if (!pooled) {
      // the heap arrays hold no pointers, so keeping them out of the GC's
      // reach saves scanning them every pass
      sc.verts = (pico3d_vcache_t *)m_malloc_no_scan(sizeof(pico3d_vcache_t) * sc.vert_cap);
      sc.ys    = (int16_t *)m_malloc_no_scan(sizeof(int16_t) * 2 * sc.tri_cap);
      sc.bin   = (uint16_t *)m_malloc_no_scan(sizeof(uint16_t) * sc.bin_cap);
      // core1's own bin, so the two cores can take a band's halves apart
      sc.bin1  = (uint16_t *)m_malloc_no_scan(sizeof(uint16_t) * sc.bin_cap);
    }
    self->refs = m_new(mp_obj_t, sc.sub_cap * 3);
    for (uint32_t i = 0; i < sc.sub_cap * 3; i++) self->refs[i] = mp_const_none;
    pico3d_scene_reset(&sc);
    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t scene_reset(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    self(args[0], scene_obj_t);
    pico3d_scene_reset(&self->scene);
    // Drop last frame's roots with it, so a mesh retired from the scene is not
    // held alive until the slot is overwritten.
    for (uint32_t i = 0, e = self->scene.sub_cap * 3; i < e; i++) {
      self->refs[i] = mp_const_none;
    }
    return mp_const_none;
  }

  mp_obj_t scene_add(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], scene_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_scene_add);
#endif
    enum { ARG_mesh, ARG_model, ARG_view_proj, ARG_material, ARG_light, ARG_view };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_mesh,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_model,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_view_proj, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_material,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_light,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_view,      MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args,
                     MP_ARRAY_SIZE(allowed), allowed, vals);

    auto need = [](mp_obj_t o, const mp_obj_type_t *want, mp_rom_error_text_t msg) -> void * {
      if (!mp_obj_is_type(o, want)) mp_raise_msg(&mp_type_TypeError, msg);
      return MP_OBJ_TO_PTR(o);
    };

    mesh_obj_t *mesh = (mesh_obj_t *)need(vals[ARG_mesh].u_obj, &type_mesh,
                                          MP_ERROR_TEXT("mesh must be a pico3d.mesh"));
    mat4_obj_t *model = (mat4_obj_t *)need(vals[ARG_model].u_obj, &type_mat4,
                                           MP_ERROR_TEXT("model must be a pico3d.mat4"));
    mat4_obj_t *vp = (mat4_obj_t *)need(vals[ARG_view_proj].u_obj, &type_mat4,
                                        MP_ERROR_TEXT("view_proj must be a pico3d.mat4"));
    material_obj_t *mat = (material_obj_t *)need(vals[ARG_material].u_obj, &type_material,
                                                 MP_ERROR_TEXT("material must be a pico3d.material"));
    pico3d_light_t *light = nullptr;
    if (vals[ARG_light].u_obj != mp_const_none) {
      light = &((light_obj_t *)need(vals[ARG_light].u_obj, &type_light,
                                    MP_ERROR_TEXT("light must be a pico3d.light")))->light;
    }
    const mat4_t *view = nullptr;
    if (vals[ARG_view].u_obj != mp_const_none) {
      view = &((mat4_obj_t *)need(vals[ARG_view].u_obj, &type_mat4,
                                  MP_ERROR_TEXT("view must be a pico3d.mat4")))->m;
    }

    pico3d_target_t t;
    surface_view(self->target, &t);
    const uint32_t before = self->scene.sub_count;
    const char *why = nullptr;
    if (!pico3d_scene_add(&self->scene, &t, &mesh->mesh, &model->m, &vp->m,
                          &mat->mat, mat->shading, light, view, &why)) {
      // the declared capacities keep the documented contract (return False);
      // running out of the platform's vertex arena is new and raises instead
      if (why && strcmp(why, "vertex arena") == 0) {
        mp_raise_msg(&mp_type_RuntimeError,
                     MP_ERROR_TEXT("scene vertex arena exhausted: fewer/smaller meshes, or a smaller scene"));
      }
      return mp_const_false;
    }
    // A culled mesh is a success that added nothing, so root against the slot
    // that was actually taken rather than assuming one was.
    if (self->scene.sub_count != before) {
      mp_obj_t *slot = self->refs + (size_t)before * 3;
      slot[0] = vals[ARG_mesh].u_obj;
      slot[1] = vals[ARG_material].u_obj;
      slot[2] = vals[ARG_light].u_obj;
    }
    return mp_const_true;
  }

  // ── engine ────────────────────────────────────────────────────────────────
  mp_obj_t engine_profile(size_t n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    uint64_t *counters[] = {
      pico3d_prof_transform_cyc, pico3d_prof_build_cyc,
      pico3d_prof_project_cyc, pico3d_prof_planes_cyc, pico3d_prof_edges_cyc,
      pico3d_prof_fill_cyc, pico3d_prof_bbox_px, pico3d_prof_px,
    };
    mp_obj_t out[MP_ARRAY_SIZE(counters)];
    for (size_t i = 0; i < MP_ARRAY_SIZE(counters); i++) {
      // both cores' counts, summed
      out[i] = mp_obj_new_int_from_uint((mp_uint_t)(counters[i][0] + counters[i][1]));
      counters[i][0] = counters[i][1] = 0;
    }
    return mp_obj_new_tuple(MP_ARRAY_SIZE(out), out);
  }

  mp_obj_t engine_profile_detail(size_t n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    mp_obj_t d = mp_obj_new_dict(24);
    auto store = [&](qstr name, uint64_t c0, uint64_t c1) {
      mp_obj_t pair[2] = { mp_obj_new_int_from_ull(c0), mp_obj_new_int_from_ull(c1) };
      mp_obj_dict_store(d, MP_OBJ_NEW_QSTR(name), mp_obj_new_tuple(2, pair));
    };
    struct { qstr name; uint64_t *c; } wide[] = {
      { MP_QSTR_transform_wall, pico3d_prof_transform_cyc },
      { MP_QSTR_project,        pico3d_prof_project_cyc },
      { MP_QSTR_planes,         pico3d_prof_planes_cyc },
      { MP_QSTR_edges,          pico3d_prof_edges_cyc },
      { MP_QSTR_fill,           pico3d_prof_fill_cyc },
      { MP_QSTR_bbox_px,        pico3d_prof_bbox_px },
      { MP_QSTR_px,             pico3d_prof_px },
    };
    for (auto &r : wide) { store(r.name, r.c[0], r.c[1]); r.c[0] = r.c[1] = 0; }
    struct { qstr name; int id; } detail[] = {
      { MP_QSTR_add_wall,     PICO3D_PD_ADD_WALL },  { MP_QSTR_xform,      PICO3D_PD_XFORM },
      { MP_QSTR_extents,      PICO3D_PD_EXTENTS },   { MP_QSTR_draw_wall,  PICO3D_PD_DRAW_WALL },
      { MP_QSTR_clear,        PICO3D_PD_CLEAR },     { MP_QSTR_bin,        PICO3D_PD_BIN },
      { MP_QSTR_pass2,        PICO3D_PD_PASS2 },     { MP_QSTR_raster,     PICO3D_PD_RASTER },
      { MP_QSTR_wait,         PICO3D_PD_WAIT },      { MP_QSTR_verts,      PICO3D_PD_VERTS },
      { MP_QSTR_tris_in,      PICO3D_PD_TRIS_IN },   { MP_QSTR_tris_drawn, PICO3D_PD_TRIS_DRAWN },
      { MP_QSTR_tris_clipped, PICO3D_PD_TRIS_CLIPPED },
      { MP_QSTR_rows, PICO3D_PD_ROWS }, { MP_QSTR_rows_empty, PICO3D_PD_ROWS_EMPTY }, { MP_QSTR_fills, PICO3D_PD_FILLS },
      { MP_QSTR_span_px, PICO3D_PD_SPAN_PX },
    };
    for (auto &r : detail) {
      store(r.name, pico3d_prof_detail[0][r.id], pico3d_prof_detail[1][r.id]);
      pico3d_prof_detail[0][r.id] = pico3d_prof_detail[1][r.id] = 0;
    }
    return d;
  }

}
