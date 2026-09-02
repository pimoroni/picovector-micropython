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
    return MP_OBJ_FROM_PTR(self);
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
  // Fill a render-target view from the wrapped image, reallocating the depth
  // buffer if the image has been resized (a window()ed view can hand back a
  // different size) since the surface was built.
  void surface_view(surface_obj_t *self, pico3d_target_t *t) {
    image_t *im = self->source->image;
    rect_t b = im->bounds(), cl = im->clip();
    int w = (int)b.w, h = (int)b.h;
    if (w != self->w || h != self->h) {
      self->depth = m_new(uint16_t, (size_t)w * h);
      self->w = w;
      self->h = h;
    }
    t->color = (uint32_t *)im->ptr(0, 0);
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
  }

  mp_obj_t surface_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    if (!pv::is_image(args[0])) {
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("surface() expects an image"));
    }
    image_obj_t *img = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    if (img->image->pixel_format() != RGBA8888) {
      mp_raise_msg(&mp_type_ValueError,
                   MP_ERROR_TEXT("pico3d needs an RGBA image"));
    }
    rect_t b = img->image->bounds();

    surface_obj_t *self = mp_obj_malloc(surface_obj_t, type);
    self->source = img;
    self->w = (int)b.w;
    self->h = (int)b.h;
    self->depth = m_new(uint16_t, (size_t)self->w * self->h);
    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t surface_clear_depth(size_t n_args, const mp_obj_t *args) {
    self(args[0], surface_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_surface_clear_depth);
#endif
    uint16_t value = n_args > 1 ? (uint16_t)mp_obj_get_int(args[1]) : 0xFFFF;
    pico3d_target_t t{};
    t.depth = self->depth;
    t.width = self->w;
    t.height = self->h;
    t.depth_stride = self->w;
    t.clip_x1 = self->w;
    t.clip_y1 = self->h;
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
    // depth=False drops the Z-buffer for this call. Both the read and the write
    // go to PSRAM, so it is a real saving on a mesh that cannot occlude itself.
    if (!vals[ARG_depth].u_bool) t.depth = nullptr;

    int drawn = pico3d_draw_mesh(&t, &mesh->mesh, &model->m, &vp->m, &mat->mat,
                                 mat->shading, light, self->vcache, view);
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
