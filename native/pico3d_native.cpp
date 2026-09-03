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
    // The bounding box for whole-mesh frustum culling, measured once here.
    // positions stays borrowed and writable, so a mesh that animates a vertex
    // out past this box has to call update_bounds().
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
  // Point a surface's depth buffer at bytes the caller owns. The buffer is
  // borrowed, not copied, and held alive by the surface. Why bother: the depth
  // buffer is the single biggest thing a render touches - two accesses a pixel,
  // every pixel - so on a board whose heap is in external memory, being able to
  // hand the engine a slab of on-chip RAM instead is worth more than any
  // amount of tuning inside the rasteriser.
  static void borrow_depth(surface_obj_t *self, mp_obj_t buf, int w, int h) {
    mp_buffer_info_t bi;
    mp_get_buffer_raise(buf, &bi, MP_BUFFER_RW);
    if (bi.len < (size_t)w * (size_t)h * sizeof(uint16_t)) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("depth buffer needs %d bytes for %dx%d"),
                        (int)((size_t)w * (size_t)h * sizeof(uint16_t)), w, h);
    }
    // The engine indexes it as uint16_t, so an odd address would fault on a
    // core that traps unaligned halfword access.
    if (((uintptr_t)bi.buf & 1) != 0) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("depth buffer must be 2-byte aligned"));
    }
    self->depth = (uint16_t *)bi.buf;
    self->depth_obj = buf;
  }

  // Point a banded surface's depth buffer at picovector's shared working buffer.
  // That buffer is on-chip SRAM and is idle while a 3D pass runs (nothing else
  // is rasterising), which is exactly what a depth buffer wants and exactly
  // what the heap - external PSRAM on this board - is not. One band at a time
  // means the strip is (height / bands) rows, so three bands turn a 150 KB
  // full-screen buffer into a 51 KB strip that fits.
  static void band_depth(surface_obj_t *self) {
    int rows = (self->h + self->bands - 1) / self->bands;
    size_t need = (size_t)rows * (size_t)self->w * sizeof(uint16_t);
    if (need > working_buffer_size) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("%d bands of %dx%d needs %d bytes of working buffer, have %d"),
                        self->bands, self->w, rows, (int)need, (int)working_buffer_size);
    }
    self->depth = (uint16_t *)PicoVector_working_buffer;
    self->band_rows = rows;
    self->depth_obj = MP_OBJ_NULL;      // not borrowed, not heap-allocated
  }

  // Fill a render-target view from the wrapped image, reallocating the depth
  // buffer if the image has been resized (a window()ed view can hand back a
  // different size) since the surface was built.
  void surface_view(surface_obj_t *self, pico3d_target_t *t) {
    image_t *im = self->source->image;
    rect_t b = im->bounds(), cl = im->clip();
    int w = (int)b.w, h = (int)b.h;
    if (w != self->w || h != self->h) {
      // A borrowed buffer cannot be grown, so a view that has outgrown it is an
      // error rather than a silent overrun into whatever follows it.
      self->w = w;
      self->h = h;
      if (self->bands > 0) band_depth(self);        // restripe for the new size
      else if (self->depth_obj == mp_const_false) { /* depth=False: stays none */ }
      else if (self->depth_obj != MP_OBJ_NULL) borrow_depth(self, self->depth_obj, w, h);
      else self->depth = m_new(uint16_t, (size_t)w * h);
    }
    t->color = (uint32_t *)im->ptr(0, 0);
    t->depth = self->depth;
    t->width = w;
    t->height = h;
    t->color_stride = im->row_stride() / im->bytes_per_pixel();
    t->depth_stride = w;
    t->depth_y0 = 0;                    // pico3d_scene_draw walks this per band
    t->row_step = 1;                    // every row; the banded path splits these
    t->row_phase = 0;                   // across cores, nobody else touches them
    // The image's clip rect bounds the render, so a 3D viewport is framed the
    // same way anything else is.
    t->clip_x0 = (int)cl.x;
    t->clip_y0 = (int)cl.y;
    t->clip_x1 = (int)(cl.x + cl.w);
    t->clip_y1 = (int)(cl.y + cl.h);
  }

  mp_obj_t surface_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *args) {
    enum { ARG_image, ARG_depth, ARG_bands };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_image, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_depth, MP_ARG_OBJ,                   {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_bands, MP_ARG_INT,                   {.u_int = 0} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);
    mp_obj_t image_in = vals[ARG_image].u_obj;
    if (!pv::is_image(image_in)) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("surface() expects an image"));
    }
    image_obj_t *img = (image_obj_t *)MP_OBJ_TO_PTR(image_in);
    // has_palette() is separate from the format: a palettised image reports
    // RGBA8888 for its colour table while its pixels are one byte of index, so
    // writing RGBA words into it would run four times past the end.
    if (img->image->pixel_format() != RGBA8888 || img->image->has_palette()) {
      mp_raise_msg(&mp_type_ValueError,
                   MP_ERROR_TEXT("pico3d needs an RGBA image"));
    }
    rect_t b = img->image->bounds();

    surface_obj_t *self = mp_obj_malloc(surface_obj_t, type);
    self->source = img;
    self->w = (int)b.w;
    self->h = (int)b.h;
    mp_obj_t depth_in = vals[ARG_depth].u_obj;
    int bands = vals[ARG_bands].u_int;
    if (bands > 0) {
      if (depth_in != mp_const_none) {
        mp_raise_msg(&mp_type_ValueError,
                     MP_ERROR_TEXT("bands and depth are two ways to place the same buffer"));
      }
      self->bands = bands;
      band_depth(self);
    } else if (depth_in == mp_const_false) {
      // No depth buffer at all. Every render then takes the rasteriser's
      // depth-free path, and the app orders its own draws. Worth having as a
      // first-class option, not just depth=False on every render call: it also
      // means the surface never allocates the buffer, which at 320x240 is
      // 150 KB of heap that a painter's-order renderer has no use for.
      self->depth = nullptr;
      self->depth_obj = mp_const_false;       // marks it deliberate, not unset
    } else if (depth_in != mp_const_none) {
      borrow_depth(self, depth_in, self->w, self->h);
    } else {
      self->depth = m_new(uint16_t, (size_t)self->w * self->h);
    }
    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t surface_clear_depth(size_t n_args, const mp_obj_t *args) {
    self(args[0], surface_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_surface_clear_depth);
#endif
    uint16_t value = n_args > 1 ? (uint16_t)mp_obj_get_int(args[1]) : 0xFFFF;
    // Bounded by the image's clip rect, the same way render() is. It used to
    // clear the whole buffer whatever the clip, which made a clipped 3D
    // viewport pay for rows it never draws - and makes clearing one band of a
    // banded render impossible.
    rect_t cl = self->source->image->clip().intersection(self->source->image->bounds());
    pico3d_target_t t{};
    t.depth = self->depth;
    t.width = self->w;
    t.height = self->h;
    t.depth_stride = self->w;
    t.clip_x0 = (int)cl.x;
    t.clip_y0 = (int)cl.y;
    t.clip_x1 = (int)(cl.x + cl.w);
    t.clip_y1 = (int)(cl.y + cl.h);
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

    pico3d_target_t t{};
    surface_view(self, &t);
    // depth=False drops the Z-buffer for this call. Both the read and the write
    // go to PSRAM, so it is a real saving on a mesh that cannot occlude itself.
    if (!vals[ARG_depth].u_bool) t.depth = nullptr;

    int drawn = pico3d_draw_mesh(&t, &mesh->mesh, &model->m, &vp->m, &mat->mat,
                                 mat->shading, light, self->vcache, view);
    return mp_obj_new_int(drawn);
  }

  // ── scene ─────────────────────────────────────────────────────────────────
  mp_obj_t scene_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                               size_t n_kw, const mp_obj_t *args) {
    enum { ARG_surface, ARG_meshes, ARG_vertices, ARG_triangles };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_surface,   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_meshes,    MP_ARG_INT, {.u_int = 64} },
      { MP_QSTR_vertices,  MP_ARG_INT, {.u_int = 1024} },
      { MP_QSTR_triangles, MP_ARG_INT, {.u_int = 512} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed), allowed, vals);
    // A scene is built FOR a surface: add() projects to screen coordinates, so
    // it has to know the viewport, and geometry gathered for one size would land
    // in the wrong place in another.
    if (!mp_obj_is_type(vals[ARG_surface].u_obj, &type_surface)) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("scene() expects a pico3d.surface"));
    }
    int nm = vals[ARG_meshes].u_int;
    int nv = vals[ARG_vertices].u_int;
    int nt = vals[ARG_triangles].u_int;
    if (nm < 1 || nv < 3 || nt < 1) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("a scene needs room for at least one triangle"));
    }

    scene_obj_t *self = mp_obj_malloc(scene_obj_t, type);
    self->target = (surface_obj_t *)MP_OBJ_TO_PTR(vals[ARG_surface].u_obj);
    self->sc.subs  = m_new(pico3d_sub_t, nm);      self->sc.sub_cap  = (uint32_t)nm;
    self->sc.verts = m_new(pico3d_vcache_t, nv);   self->sc.vert_cap = (uint32_t)nv;
    self->sc.ys    = m_new(int16_t, (size_t)nt * 2); self->sc.tri_cap = (uint32_t)nt;
    // bin holds ONE submission's live triangles, so the whole scene's worth is
    // always enough however the triangles are distributed between meshes.
    self->sc.bin   = m_new(uint16_t, nt);          self->sc.bin_cap  = (uint32_t)nt;
    self->refs     = m_new(mp_obj_t, (size_t)nm * 3);
    pico3d_scene_reset(&self->sc);
    for (int i = 0; i < nm * 3; i++) self->refs[i] = MP_OBJ_NULL;
    return MP_OBJ_FROM_PTR(self);
  }

  // Signature must match what the generator emits for @native, which is
  // MP_DEFINE_CONST_FUN_OBJ_VAR - (n_args, args), not (self). C linkage matches
  // on the name alone, so getting this wrong links cleanly and then hands the
  // argument COUNT over as the object pointer.
  mp_obj_t scene_reset(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    self(args[0], scene_obj_t);
    pico3d_scene_reset(&self->sc);
    // Drop the roots too, or last frame's geometry stays alive for nothing.
    for (uint32_t i = 0, n = self->sc.sub_cap * 3; i < n; i++) self->refs[i] = MP_OBJ_NULL;
    return mp_const_none;
  }

  mp_obj_t scene_add(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], scene_obj_t);
    enum { ARG_mesh, ARG_model, ARG_view_proj, ARG_material, ARG_light, ARG_view };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_mesh,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_model,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_view_proj, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_material,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_light,     MP_ARG_OBJ,  {.u_obj = mp_const_none} },
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

    pico3d_target_t t{};
    surface_view(self->target, &t);        // for the viewport it projects into

    uint32_t slot = self->sc.sub_count;
    bool ok = pico3d_scene_add(&self->sc, &t, &mesh->mesh, &model->m, &vp->m,
                               &mat->mat, mat->shading, light, view);
    if (ok) {
      // Root what the submission now points at, so a collection between add()
      // and draw() cannot free geometry out from under the rasteriser.
      self->refs[slot * 3 + 0] = vals[ARG_mesh].u_obj;
      self->refs[slot * 3 + 1] = vals[ARG_material].u_obj;
      self->refs[slot * 3 + 2] = vals[ARG_light].u_obj;
    }
    return mp_obj_new_bool(ok);
  }

  mp_obj_t surface_draw(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], surface_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_surface_draw);
#endif
    enum { ARG_scene, ARG_clear };
    static const mp_arg_t allowed[] = {
      { MP_QSTR_scene, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_clear, MP_ARG_INT, {.u_int = 0xFFFF} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args,
                     MP_ARRAY_SIZE(allowed), allowed, vals);
    if (!mp_obj_is_type(vals[ARG_scene].u_obj, &type_scene)) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("draw() expects a pico3d.scene"));
    }
    scene_obj_t *sc = (scene_obj_t *)MP_OBJ_TO_PTR(vals[ARG_scene].u_obj);

    pico3d_target_t t{};
    surface_view(self, &t);
    int drawn = pico3d_scene_draw(&sc->sc, &t, self->bands > 0 ? self->band_rows : 0,
                                  (uint16_t)vals[ARG_clear].u_int);
    return mp_obj_new_int(drawn);
  }

  // ── engine ────────────────────────────────────────────────────────────────
  mp_obj_t engine_profile(size_t n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    uint64_t *counters[] = {
      &pico3d_prof_transform_cyc, &pico3d_prof_build_cyc,
      &pico3d_prof_project_cyc, &pico3d_prof_planes_cyc, &pico3d_prof_edges_cyc,
      &pico3d_prof_fill_cyc, &pico3d_prof_bbox_px, &pico3d_prof_px,
    };
    mp_obj_t out[MP_ARRAY_SIZE(counters)];
    for (size_t i = 0; i < MP_ARRAY_SIZE(counters); i++) {
      out[i] = mp_obj_new_int_from_uint((mp_uint_t)*counters[i]);
      *counters[i] = 0;
    }
    return mp_obj_new_tuple(MP_ARRAY_SIZE(out), out);
  }

}
