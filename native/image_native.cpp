// native/image.cpp — hand-written bodies for the procedural image members.
//
//   load / load_into  decode PNG/JPEG via the bundled decoders
//   window            constructs a sub-view and tracks its parent
//   text / measure_text  branch on the vector vs pixel font
//   shapes            batched shape draw with a reused stack colour brush
//   batch             dispatches (method, *args) command tuples
//
// None of these reduce to "call one C++ function and box the result", so they
// are maintained here. The generated image.cpp provides the per-method drawing
// functions (mpy_image_*) and image_attr, which batch dispatches to.

#include "pv_bindings.hpp"

// JPEG_* error codes, for descriptive load errors. PNG_* arrive via pv_objs.hpp.
#ifndef NO_QSTR
  #include "JPEGDEC.h"
#endif

extern "C" {
  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"
  #include "py/objstr.h"

  // generated drawing functions + attr handler (image.cpp), used by batch
  extern void image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);
  extern mp_obj_t mpy_image_clear(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_rectangle(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_line(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_circle(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_triangle(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_put(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_blur(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_dither(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_shape(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_blit(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_blit_vspan(size_t, const mp_obj_t *);
  extern mp_obj_t mpy_image_blit_hspan(size_t, const mp_obj_t *);
  extern mp_obj_t image_text(size_t, const mp_obj_t *);

  // Raise a descriptive error for a failed decode. Each message is a complete
  // MP_ERROR_TEXT literal so it participates in ROM text compression; composing
  // one with %s would store the substituted phrase uncompressed. The rare/
  // internal codes fall through to the numeric form (a compressed format string
  // with the code as %d). mp_raise_* is noreturn, so cases need no break.
  static void raise_png_error(int status) {
    switch (status) {
      case PNG_UNSUPPORTED_FEATURE:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load PNG: interlaced and 16-bit are not supported"));
      case PNG_TOO_BIG:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load PNG: image line too wide for the decoder"));
      case PNG_DECODE_ERROR:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load PNG: corrupt or truncated data"));
      case PNG_MEM_ERROR:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load PNG: out of memory"));
      default:
        mp_raise_msg_varg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load PNG (error %d)"), status);
    }
  }

  static void raise_jpeg_error(int status) {
    switch (status) {
      case JPEG_UNSUPPORTED_FEATURE:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load JPEG: unsupported feature"));
      case JPEG_DECODE_ERROR:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load JPEG: corrupt or truncated data"));
      default:
        mp_raise_msg_varg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load JPEG (error %d)"), status);
    }
  }

  // Try PNG first; only if the data isn't a PNG at all (PNG_INVALID_FILE) fall
  // back to JPEG. Track which decoder produced the final status so the raised
  // message names the real problem rather than a bare, enum-ambiguous number
  // (PNG and JPEG have separate error enums that collide numerically).
  static void image_open_helper(image_obj_t &target, mp_obj_t path_or_bytes_in,
                                int target_width, int target_height) {
    int png_status;
    int jpeg_status = JPEG_SUCCESS;
    bool tried_jpeg = false;

    if (mp_obj_is_str(path_or_bytes_in)) {
      const char *path = mp_obj_str_get_str(path_or_bytes_in);
      png_status = pngdec_open_file(target, path, target_width, target_height);
      if (png_status == PNG_INVALID_FILE) {
        tried_jpeg = true;
        jpeg_status = jpegdec_open_file(target, path, target_width, target_height);
      }
    } else {
      mp_buffer_info_t buf;
      mp_get_buffer_raise(path_or_bytes_in, &buf, MP_BUFFER_READ);
      png_status = pngdec_open_ram(target, buf.buf, buf.len, target_width, target_height);
      if (png_status == PNG_INVALID_FILE) {
        tried_jpeg = true;
        jpeg_status = jpegdec_open_ram(target, buf.buf, buf.len, target_width, target_height);
      }
    }

    if (png_status != PNG_SUCCESS && !(tried_jpeg && jpeg_status == JPEG_SUCCESS)) {
      if (!tried_jpeg) {
        // PNG signature matched but the decode failed: a genuine PNG problem.
        raise_png_error(png_status);
      } else if (jpeg_status != JPEG_INVALID_FILE) {
        // JPEG signature matched but the decode failed: a genuine JPEG problem.
        raise_jpeg_error(jpeg_status);
      } else {
        // Neither decoder recognised the data (e.g. an AppleDouble ._ file, a
        // renamed non-image, or a truncated download).
        mp_raise_msg(&mp_type_ValueError,
                     MP_ERROR_TEXT("unrecognised image format (not a PNG or JPEG)"));
      }
    }

    // A file with a valid PNG signature but no IHDR chunk parses as 0x0 and
    // "succeeds"; reject that so it surfaces rather than yielding a blank image.
    if (target.image == nullptr ||
        target.image->bounds().w == 0 || target.image->bounds().h == 0) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("image has zero dimensions"));
    }
  }

  mp_obj_t image_load(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_load);
#endif
    image_obj_t *result = mp_obj_malloc(image_obj_t, &type_image);
    result->image = nullptr;
    int target_width  = n_args >= 2 ? (int)mp_obj_get_float(args[1]) : 0;
    int target_height = n_args >= 3 ? (int)mp_obj_get_float(args[2]) : 0;
    image_open_helper(*result, args[0], target_width, target_height);
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t image_load_into(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_load_into);
#endif
    image_open_helper(*self, args[1], 0, 0);
    return mp_const_none;
  }

  mp_obj_t image_window(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_window);
#endif
    int x, y, w, h;
    if (mp_obj_is_rect(args[1])) {
      rect_t r = mp_obj_get_rect(args[1]);
      x = r.x; y = r.y; w = r.w; h = r.h;
    } else {
      x = mp_obj_get_float(args[1]); y = mp_obj_get_float(args[2]);
      w = mp_obj_get_float(args[3]); h = mp_obj_get_float(args[4]);
    }
    image_obj_t *result = mp_obj_malloc(image_obj_t, &type_image);
    result->image = new (m_malloc(sizeof(image_t))) image_t(self->image, rect_t(x, y, w, h));
    result->parent = (void *)self;
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t image_spritesheet(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_spritesheet);
#endif
    self->image->cols(mp_obj_get_int(args[1]));
    self->image->rows(mp_obj_get_int(args[2]));
    return args[0];
  }

  mp_obj_t image_sprite(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_sprite);
#endif
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    image_obj_t *result = mp_obj_malloc(image_obj_t, &type_image);
    result->image = new (m_malloc(sizeof(image_t))) image_t(self->image->sprite(x, y));
    result->parent = (void *)self;
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t image_text(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_text);
#endif
    const char *text = mp_obj_str_get_str(args[1]);
    if (!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }
    size_t _i = 2;
    vec2_t at = pv::get_xy(args, &_i, n_args);
    // size is a sentinel-0 optional: 0 (or omitted) means "font's default" —
    // 12pt for vector fonts, 1x for pixel fonts (where it's the integer scale).
    float size = n_args > _i ? mp_obj_get_float(args[_i]) : 0.0f;
    if (self->font) {
      self->image->font()->draw(self->image, text, at.x, at.y, size > 0.0f ? size : 12.0f);
    }
    if (self->pixel_font) {
      int scale = size > 0.0f ? (int)size : 1;
      self->image->pixel_font()->draw(self->image, text, at.x, at.y, scale);
    }
    return mp_const_none;
  }

  mp_obj_t image_measure_text(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_measure_text);
#endif
    const char *text = mp_obj_str_get_str(args[1]);
    if (!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }
    // size is a sentinel-0 optional: 0 (or omitted) means "font's default" —
    // 12pt for vector fonts, 1x for pixel fonts (where it's the integer scale).
    float size = n_args > 2 ? mp_obj_get_float(args[2]) : 0.0f;
    mp_obj_t result[2];
    if (self->font) {
      rect_t r = self->image->font()->measure(self->image, text, size > 0.0f ? size : 12.0f);
      result[0] = mp_obj_new_float(r.w);
      result[1] = mp_obj_new_float(r.h);
    }
    if (self->pixel_font) {
      int scale = size > 0.0f ? (int)size : 1;
      rect_t r = self->image->pixel_font()->measure(self->image, text, scale);
      result[0] = mp_obj_new_float(r.w);
      result[1] = mp_obj_new_float(r.h);
    }
    return mp_obj_new_tuple(2, result);
  }

  // Batched shape draw: each entry is a shape, or (shape, brush|color). A
  // colour entry is wrapped in a stack-allocated colour brush reused each
  // iteration, so it costs no per-shape heap allocation.
  mp_obj_t image_shapes(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_shapes);
#endif
    size_t len; mp_obj_t *items;
    mp_obj_get_array(args[1], &len, &items);
    brush_t *default_brush = self->image->brush();

    for (size_t i = 0; i < len; i++) {
      mp_obj_t entry = items[i];
      shape_obj_t *shape;
      brush_t *b = default_brush;
      alignas(color_brush_t) char cbuf[sizeof(color_brush_t)];

      if (mp_obj_is_type(entry, &type_shape)) {
        shape = (shape_obj_t *)MP_OBJ_TO_PTR(entry);
      } else {
        size_t tlen; mp_obj_t *t;
        mp_obj_get_array(entry, &tlen, &t);
        if (tlen < 1 || !mp_obj_is_type(t[0], &type_shape)) {
          mp_raise_msg_varg(&mp_type_ValueError,
                            MP_ERROR_TEXT("entry must be a shape or (shape, brush)"));
        }
        shape = (shape_obj_t *)MP_OBJ_TO_PTR(t[0]);
        if (tlen >= 2 && t[1] != mp_const_none) {
          if (mp_obj_is_type(t[1], &type_brush)) {
            b = ((brush_obj_t *)MP_OBJ_TO_PTR(t[1]))->brush;
          } else if (mp_obj_is_type(t[1], &type_color)) {
            color_obj_t *col = (color_obj_t *)MP_OBJ_TO_PTR(t[1]);
            b = new (cbuf) color_brush_t(col->c);
          } else {
            mp_raise_TypeError(MP_ERROR_TEXT("entry brush must be a brush or color"));
          }
        }
      }
      if (!b) mp_raise_msg_varg(&mp_type_ValueError,
                                MP_ERROR_TEXT("no pen set; supply a brush/color in the entry "
                                              "or set screen.pen first"));
      picovector::render(shape->shape, self->image, &shape->shape->transform, b);
    }
    return mp_const_none;
  }

  // Run a list of (command, *args) tuples in one C loop. A single-argument
  // command first tries to set an attribute (pen, clip, …); otherwise the
  // command name selects a generated drawing function.
  mp_obj_t image_batch(size_t n_args, const mp_obj_t *args) {
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_batch);
#endif
    if (!mp_obj_is_type(args[1], &mp_type_list)) {
      mp_raise_TypeError(MP_ERROR_TEXT("invalid parameters, expected list of draw commands"));
    }
    mp_obj_t handler_args[32];
    handler_args[0] = args[0];

    size_t ncommands; mp_obj_t *commands;
    mp_obj_list_get(args[1], &ncommands, &commands);

    for (size_t i = 0; i < ncommands; i++) {
      if (!mp_obj_is_type(commands[i], &mp_type_tuple)) {
        mp_raise_TypeError(MP_ERROR_TEXT("list entries must be tuples in the format "
                                         "(command, parameter1, parameter2, ...)"));
      }
      size_t ncommand; mp_obj_t *command;
      mp_obj_tuple_get(commands[i], &ncommand, &command);
      qstr name = mp_obj_str_get_qstr(command[0]);

      size_t nparameters = ncommand - 1;
      for (size_t j = 1; j < ncommand; j++) handler_args[j] = command[j];

      if (nparameters == 1) {
        mp_obj_t dest[2];
        dest[0] = MP_OBJ_SENTINEL;
        dest[1] = handler_args[1];
        image_attr(handler_args[0], name, dest);
        if (dest[0] == MP_OBJ_NULL) continue;  // attribute set, next command
      }

      size_t nh = nparameters + 1;
      switch (name) {
        case MP_QSTR_clear:      mpy_image_clear(nh, handler_args); break;
        case MP_QSTR_rectangle:  mpy_image_rectangle(nh, handler_args); break;
        case MP_QSTR_line:       mpy_image_line(nh, handler_args); break;
        case MP_QSTR_circle:     mpy_image_circle(nh, handler_args); break;
        case MP_QSTR_triangle:   mpy_image_triangle(nh, handler_args); break;
        case MP_QSTR_put:        mpy_image_put(nh, handler_args); break;
        case MP_QSTR_blur:       mpy_image_blur(nh, handler_args); break;
        case MP_QSTR_dither:     mpy_image_dither(nh, handler_args); break;
        case MP_QSTR_shape:      mpy_image_shape(nh, handler_args); break;
        case MP_QSTR_text:       image_text(nh, handler_args); break;
        case MP_QSTR_blit_vspan: mpy_image_blit_vspan(nh, handler_args); break;
        case MP_QSTR_blit_hspan: mpy_image_blit_hspan(nh, handler_args); break;
        case MP_QSTR_blit:       mpy_image_blit(nh, handler_args); break;
        default: mp_raise_ValueError(MP_ERROR_TEXT("unknown method"));
      }
    }
    return mp_const_none;
  }

}
