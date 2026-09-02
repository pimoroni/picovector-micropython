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
#include "gif.hpp"
#include "spritesheet.hpp"
#include "blend.hpp"
#include "rasteriser.hpp"
#include "pico3d.hpp"
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

  typedef struct _gif_handle_t {
    mp_obj_t fhandle;
  } gif_handle_t;

  typedef struct _vector_font_obj_t {
    mp_obj_base_t base;
    font_t font;
    uint8_t *buffer;
    uint32_t buffer_size;
    char *path;   // resolved file path this font was loaded from (for repr)
  } vector_font_obj_t;

  typedef struct _color_obj_t {
    mp_obj_base_t base;
    // The colour is stored *by value* (the rgb/hsv/oklch subclass is just a
    // construction strategy, and adds no state). Embedding avoids a second heap
    // allocation + pointer indirection, and - crucially - avoids leaking a
    // `new`-allocated color_t that lived off the MicroPython GC heap. 4 + 12
    // bytes, so a boxed colour stays inside one 32-byte GC block.
    color_t c;
  } color_obj_t;

  typedef struct _pixel_font_obj_t {
    mp_obj_base_t base;
    pixel_font_t *font;
    uint8_t *glyph_buffer;
    uint32_t glyph_buffer_size;
    uint8_t *glyph_data_buffer;
    uint32_t glyph_data_buffer_size;
    char *path;   // resolved file path this font was loaded from (for repr)
  } pixel_font_obj_t;

  typedef struct _image_obj_t {
    mp_obj_base_t base;
    image_t *image;
    brush_obj_t *brush;
    vector_font_obj_t *font;
    pixel_font_obj_t *pixel_font;
    void *parent;
    // Per-frame durations in milliseconds, for a sheet composited from a GIF;
    // MP_OBJ_NULL for every other image. Frame timing belongs to the container,
    // not to a pixel buffer, so it stops here and doesn't reach image_t.
    mp_obj_t frame_delays;
  } image_obj_t;

  // A colour table, as a Python sequence.
  //
  // Attached to an image (`source` set), it is a handle onto that image's own
  // table, read and written in place - and sub-views share the table by pointer,
  // so one write recolours every sprite cut from the same sheet. Free-standing
  // (`source` null), it owns `entries` and is something to assign *from*.
  //
  // Which is why assigning a palette copies the entries in rather than swapping
  // the pointer: a view holds its own copy of image_t::_palette, so a swap would
  // leave every existing sprite showing the old colours.
  typedef struct _palette_obj_t {
    mp_obj_base_t base;
    image_obj_t *source;
    uint32_t *entries;
    uint16_t count;
  } palette_obj_t;

  // A grid over someone else's pixels. It owns no buffer, so it holds the source
  // obj to keep it alive - the GC scans this block, so the pointer is a root, the
  // same way image_obj_t::parent keeps a parent view up.
  typedef struct _spritesheet_obj_t {
    mp_obj_base_t base;
    image_obj_t *source;
    spritesheet_t sheet;
  } spritesheet_obj_t;

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

  // spritesheet.timings boxes a sequence, so the generated attr getter calls out
  // to native/spritesheet_native.cpp.
  extern mp_obj_t spritesheet_box_timings(spritesheet_obj_t *self);

  // palette (native/palette_native.cpp): where a palette's entries live, and the
  // getter/setter behind image.palette on both image types.
  extern uint32_t *palette_entries(palette_obj_t *self);
  extern mp_obj_t palette_box(image_obj_t *image);
  extern void palette_assign(image_obj_t *image, mp_obj_t value);

  // used by image.pen = N and picovector.pen() (global pen)
  extern brush_obj_t *mp_obj_to_brush(size_t n_args, const mp_obj_t *args);

  // image.cpp uses pngdec_open_file and pngdec_open_ram from image_png
  extern int pngdec_open_file(image_obj_t &target, const char* path, int target_width, int target_height);
  extern int pngdec_open_ram(image_obj_t &target, const void* buffer, const size_t size, int target_width, int target_height);

  // ... and jpegdec_open_file and jpegdec_open_ram from image_jpeg
  extern int jpegdec_open_file(image_obj_t &target, const char* path, int target_width, int target_height);
  extern int jpegdec_open_ram(image_obj_t &target, const void* buffer, const size_t size, int target_width, int target_height);

  // ... and gifdec_open_file and gifdec_open_ram from image_gif, which build a
  // spritesheet rather than a single frame. `info` comes back filled whenever the
  // file survived its header, so a refusal can be reported in the file's own
  // figures.
  extern int gifdec_open_file(image_obj_t &target, const char* path, gif_info_t *info);
  extern int gifdec_open_ram(image_obj_t &target, const void* buffer, const size_t size,
                             gif_info_t *info);

  // Font loaders. `font` is a namespace singleton (native/font_native.cpp) whose
  // `load` sniffs the file and returns a vector_font or pixel_font. The two
  // parsers take an open file stream with the 4-byte marker already consumed,
  // plus the resolved path the font was loaded from (stored for its repr).
  extern mp_obj_t load_vector_font(mp_obj_t file, const char *path);   // -> vector_font_obj_t
  extern mp_obj_t parse_pixel_font(mp_obj_t file, const char *path);   // -> pixel_font_obj_t

  // The `font` namespace singleton object, registered in the module globals.
  typedef struct _font_ns_obj_t { mp_obj_base_t base; } font_ns_obj_t;
  extern const font_ns_obj_t pv_font_ns_obj;

  // ── pico3d ────────────────────────────────────────────────────────────────
  // The 3D module's objs. The engine works in plain pointer views, so these are
  // where image_t and the GC heap meet it: every borrowed Python buffer is held
  // by the obj that points into it, so the GC can't free it under the engine.

  typedef struct _vec3_obj_t {
    mp_obj_base_t base;
    vec3_t v;
  } vec3_obj_t;

  typedef struct _mat4_obj_t {
    mp_obj_base_t base;
    mat4_t m;
  } mat4_obj_t;

  typedef struct _mesh_obj_t {
    mp_obj_base_t base;
    pico3d_mesh_t mesh;
    // GC roots for the borrowed geometry mesh points into.
    mp_obj_t positions_ref, indices_ref, normals_ref, uvs_ref, colors_ref, tangents_ref;
  } mesh_obj_t;

  typedef struct _material_obj_t {
    mp_obj_base_t base;
    pico3d_material_t mat;
    pico3d_texture_t tex, nmap, mcap;   // views mat's pointers refer to
    pico3d_shading_t shading;
    mp_obj_t texture_ref, normal_map_ref, matcap_ref;   // GC roots
  } material_obj_t;

  typedef struct _light_obj_t {
    mp_obj_base_t base;
    pico3d_light_t light;
  } light_obj_t;

  // Wraps an RGBA8888 image and owns the depth buffer and per-vertex transform
  // scratch that go with it. Both are sized on demand and reused across frames.
  typedef struct _surface_obj_t {
    mp_obj_base_t base;
    image_obj_t *source;
    uint16_t *depth;
    int w, h;
    pico3d_vcache_t *vcache;
    uint32_t vcache_cap;
  } surface_obj_t;

  // pico3d natives (native/pico3d_native.cpp): the depth/vcache-owning render
  // entry point and the two engine-wide accessors behind the `engine` namespace.
  extern void surface_view(surface_obj_t *self, pico3d_target_t *t);
}

extern rect_t mp_obj_get_rect(mp_obj_t rect_in);
extern rect_t mp_obj_get_rect_from_xywh(const mp_obj_t *args);

extern vec2_t mp_obj_get_vec2(mp_obj_t vec2_in);
extern vec2_t mp_obj_get_vec2_from_xy(const mp_obj_t *args);

extern bool mp_obj_is_rect(mp_obj_t rect_in);
extern bool mp_obj_is_vec2(mp_obj_t vec2_in);