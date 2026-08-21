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
#include "rasteriser.hpp" // picovector::render (image.shapes / batched shape draw)

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
  extern mp_obj_t image_text(size_t, const mp_obj_t *, mp_map_t *);

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

  static void raise_gif_error(int status, const gif_info_t &info) {
    switch (status) {
      case GIF_UNSUPPORTED:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load GIF: more than 256 colours across its frames"));
      case GIF_TOO_BIG:
        // The figures are the whole point of this one: the limit is a build
        // setting, so a caller can only act on knowing by how much it was missed.
        mp_raise_msg_varg(&mp_type_MemoryError,
          MP_ERROR_TEXT("cannot load GIF: %d frames of %dx%d composite to %dKB, over the %dKB limit. Scale it down, or use fewer frames"),
          info.frame_count, info.width, info.height,
          (int)(((size_t)info.width * info.height * info.frame_count + 1023) / 1024),
          (int)(PV_GIF_MAX_BYTES / 1024));
      case GIF_TOO_MANY_FRAMES:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load GIF: too many frames"));
      case GIF_NO_BUFFER:
        mp_raise_msg(&mp_type_MemoryError,
          MP_ERROR_TEXT("cannot load GIF: out of memory"));
      case GIF_TRUNCATED:
      case GIF_BAD_DATA:
        mp_raise_msg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load GIF: corrupt or truncated data"));
      default:
        mp_raise_msg_varg(&mp_type_ValueError,
          MP_ERROR_TEXT("cannot load GIF (error %d)"), status);
    }
  }

  // Try PNG first; only if the data isn't a PNG at all (PNG_INVALID_FILE) fall
  // back to JPEG, and then to GIF. Track which decoder produced the final status
  // so the raised message names the real problem rather than a bare,
  // enum-ambiguous number (each decoder has its own error enum, and they collide
  // numerically).
  static void image_open_helper(image_obj_t &target, mp_obj_t path_or_bytes_in,
                                int target_width, int target_height) {
    int png_status;
    int jpeg_status = JPEG_SUCCESS;
    int gif_status = GIF_OK;
    bool tried_jpeg = false;
    bool tried_gif = false;
    gif_info_t gif_info = {};

    if (mp_obj_is_str(path_or_bytes_in)) {
      const char *path = mp_obj_str_get_str(path_or_bytes_in);
      png_status = pngdec_open_file(target, path, target_width, target_height);
      if (png_status == PNG_INVALID_FILE) {
        tried_jpeg = true;
        jpeg_status = jpegdec_open_file(target, path, target_width, target_height);
        if (jpeg_status == JPEG_INVALID_FILE) {
          tried_gif = true;
          gif_status = gifdec_open_file(target, path, &gif_info);
        }
      }
    } else {
      mp_buffer_info_t buf;
      mp_get_buffer_raise(path_or_bytes_in, &buf, MP_BUFFER_READ);
      png_status = pngdec_open_ram(target, buf.buf, buf.len, target_width, target_height);
      if (png_status == PNG_INVALID_FILE) {
        tried_jpeg = true;
        jpeg_status = jpegdec_open_ram(target, buf.buf, buf.len, target_width, target_height);
        if (jpeg_status == JPEG_INVALID_FILE) {
          tried_gif = true;
          gif_status = gifdec_open_ram(target, buf.buf, buf.len, &gif_info);
        }
      }
    }

    bool loaded = png_status == PNG_SUCCESS
               || (tried_jpeg && jpeg_status == JPEG_SUCCESS)
               || (tried_gif && gif_status == GIF_OK);

    if (!loaded) {
      if (!tried_jpeg) {
        // PNG signature matched but the decode failed: a genuine PNG problem.
        raise_png_error(png_status);
      } else if (!tried_gif) {
        // JPEG signature matched but the decode failed: a genuine JPEG problem.
        raise_jpeg_error(jpeg_status);
      } else if (gif_status != GIF_BAD_MAGIC) {
        raise_gif_error(gif_status, gif_info);
      } else {
        // No decoder recognised the data (e.g. an AppleDouble ._ file, a
        // renamed non-image, or a truncated download).
        mp_raise_msg(&mp_type_ValueError,
                     MP_ERROR_TEXT("unrecognised image format (not a PNG, JPEG or GIF)"));
      }
    }

    // A GIF has to composite at its own resolution, so a requested size can only
    // be refused - and only once the format is known, which is here.
    if (tried_gif && (target_width != 0 || target_height != 0)) {
      mp_raise_msg(&mp_type_ValueError,
                   MP_ERROR_TEXT("cannot load a GIF at a different size"));
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

  static inline bool pv_is_num(mp_obj_t o) {
    return mp_obj_is_int(o) || mp_obj_is_float(o);
  }

  // Position the caret and draw one word span [ws, we) with the active font.
  // `xf` is the caller's text transform, or nullptr for the untransformed case.
  static inline void image_draw_span(image_obj_t *self, bool vector, float fs,
                                     int scale, const char *ws, const char *we,
                                     float px, float y, const mat3_t *xf) {
    text_cursor_t *c = self->image->text_cursor_state();
    c->x = px; c->y = y; c->origin_x = px; c->valid = true;
    if (vector) self->image->font()->draw(self->image, ws, we, fs, xf);
    else        self->image->pixel_font()->draw(self->image, ws, we, scale, xf);
  }

  static inline float image_measure_span(image_obj_t *self, bool vector, float fs,
                                         int scale, const char *ws, const char *we) {
    return vector ? self->image->font()->measure(self->image, ws, we, fs).w
                  : self->image->pixel_font()->measure(self->image, ws, we, scale).w;
  }

  // Inline glyph-renderer registry: the Python dict text.GLYPH_RENDERERS,
  // name -> callable fn(image, params, measure). It is owned by the frozen text
  // module (a rooted global) and handed to us once via set_glyph_registry, so
  // this plain static reference is safe without a GC root pointer: the dict is
  // kept alive by Python and MicroPython never moves objects.
  static mp_obj_t s_glyph_registry = MP_OBJ_NULL;

  mp_obj_t image__set_glyph_registry(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    s_glyph_registry = args[0];
    return mp_const_none;
  }

  // Resolve a [name] markup code to its renderer, or MP_OBJ_NULL if unregistered.
  // Compares against the registry keys directly: no temporary string is
  // allocated, so this can't trigger a GC mid-layout (we hold interior pointers
  // into the source string). The registry is small, so the linear scan is cheap.
  static mp_obj_t glyph_lookup(const char *name, size_t len) {
    if (s_glyph_registry == MP_OBJ_NULL) return MP_OBJ_NULL;
    mp_map_t *map = mp_obj_dict_get_map(s_glyph_registry);
    for (size_t i = 0; i < map->alloc; i++) {
      if (!mp_map_slot_is_filled(map, i)) continue;
      if (!mp_obj_is_str(map->table[i].key)) continue;
      size_t klen;
      const char *k = mp_obj_str_get_data(map->table[i].key, &klen);
      if (klen == len && memcmp(k, name, len) == 0) return map->table[i].value;
    }
    return MP_OBJ_NULL;
  }

  // Build the params tuple for [name:a,b,...] (comma-split strings). Capped at a
  // small fixed count to keep it off the RP2 stack; extra params are ignored.
  static mp_obj_t glyph_params(const char *p, const char *end) {
    mp_obj_t items[8];
    size_t n = 0;
    const char *s = p;
    while (p <= end && n < MP_ARRAY_SIZE(items)) {
      if (p == end || *p == ',') {
        items[n++] = mp_obj_new_str(s, p - s);
        s = p + 1;
        if (p == end) break;
      }
      p += 1;
    }
    return mp_obj_new_tuple(n, items);
  }

  // Lay out one visual line by greedy word wrap, honouring inline [code] /
  // [code:params] markup (and "[[" as a literal "["). Advances *pp past the line
  // and returns its width. In draw mode words are drawn and glyph renderers
  // invoked at ox + offset, y; otherwise the line is only measured. *hard_break
  // reports a '\n' end. Mirrors the Python text.draw wrap for identical layout.
  static float image_layout_line(image_obj_t *self, bool vector, float fs,
                                  int scale, const char **pp, const char *end,
                                  float max_w, float space_w, bool draw,
                                  float ox, float y, bool *hard_break,
                                  const mat3_t *xf) {
    const char *p = *pp;
    float x = 0.0f;        // placed line width so far
    float pending = 0.0f;  // space width accumulated before the next item
    bool started = false;
    *hard_break = false;

    while (p < end) {
      char ch = *p;
      if (ch == '\n') { p += 1; *hard_break = true; break; }
      if (ch == '\r') { p += 1; continue; }
      if (ch == ' ')  { pending += space_w; p += 1; continue; }

      // Resolve the next item: a word span [ws, we), or a glyph renderer (fn,
      // params). item_start is where this item began, for the wrap rewind.
      const char *item_start = p;
      const char *ws = nullptr, *we = nullptr;
      mp_obj_t fn = MP_OBJ_NULL, params = MP_OBJ_NULL;
      float w = 0.0f;
      bool handled = false;

      if (ch == '[') {
        if (p + 1 < end && p[1] == '[') {        // "[[" -> a literal "["
          ws = p; we = p + 1; p += 2;
          w = image_measure_span(self, vector, fs, scale, ws, we);
          handled = true;
        } else {                                 // [code] / [code:params]
          const char *close = p + 1;
          while (close < end && *close != ']' && *close != '\n') close += 1;
          if (close < end && *close == ']') {
            const char *colon = p + 1;
            while (colon < close && *colon != ':') colon += 1;
            fn = glyph_lookup(p + 1, colon - (p + 1));
            if (fn != MP_OBJ_NULL) {
              params = glyph_params(colon < close ? colon + 1 : close, close);
              mp_obj_t margs[3] = { MP_OBJ_FROM_PTR(self), params, mp_const_true };
              mp_obj_t wobj = mp_call_function_n_kw(fn, 3, 0, margs);
              w = wobj == mp_const_none ? 0.0f : mp_obj_get_float(wobj);
              p = close + 1;
              handled = true;
            }
          }
          // unterminated / unknown markup falls through to the word branch
        }
      }

      if (!handled) {
        // a word: up to the next space / newline / '[' (the first char may be a
        // literal '[' that was not valid markup)
        ws = p; we = nullptr; fn = MP_OBJ_NULL;
        p += 1;
        while (p < end && *p != ' ' && *p != '\n' && *p != '\r' && *p != '[') p += 1;
        we = p;
        w = image_measure_span(self, vector, fs, scale, ws, we);
      }

      // wrap: an item past the first that would overrun starts the next line
      if (started && x + pending + w > max_w) { p = item_start; break; }

      float px = ox + x + (started ? pending : 0.0f);
      if (draw) {
        if (fn != MP_OBJ_NULL) {
          text_cursor_t *c = self->image->text_cursor_state();
          c->x = px; c->y = y; c->origin_x = px; c->valid = true;
          mp_obj_t dargs[3] = { MP_OBJ_FROM_PTR(self), params, mp_const_false };
          mp_call_function_n_kw(fn, 3, 0, dargs);
        } else {
          image_draw_span(self, vector, fs, scale, ws, we, px, y, xf);
        }
      }
      x = px - ox + w;
      pending = 0.0f;
      started = true;
    }
    *pp = p;
    return x;
  }

  // Word-wrapped, aligned text inside `bounds`. Two streaming passes (count,
  // then draw) avoid holding a token list — no large stack arrays. Returns the
  // drawn bounding box. With draw=false it lays out without drawing (or
  // clipping), so measure_text can report the wrapped size.
  static rect_t image_layout_text(image_obj_t *self, const char *text, rect_t bounds,
                                  float size, text_align_t align_h,
                                  text_align_t align_v, text_overflow_t overflow,
                                  float line_height, float word_spacing, bool draw,
                                  const mat3_t *xf) {
    bool vector = (bool)self->font;
    float fs = size > 0.0f ? size : 12.0f;   // vector point size
    int scale = size > 0.0f ? (int)size : 1; // pixel integer scale
    if (scale < 1) scale = 1;

    float font_height = vector ? fs
                               : (float)(self->image->pixel_font()->height * scale);
    float line_advance = font_height * line_height;

    float space_w = vector
      ? self->image->font()->measure(self->image, " ", fs).w
      : self->image->pixel_font()->measure(self->image, " ", scale).w;
    if (space_w == 0.0f) space_w = font_height / 3.0f;
    space_w *= word_spacing;

    const char *end = text + strlen(text);

    // Pass 1: count wrapped lines.
    int n = 0;
    {
      const char *p = text;
      bool hard;
      while (p < end) {
        const char *ls = p;
        image_layout_line(self, vector, fs, scale, &p, end, bounds.w, space_w,
                          false, 0.0f, 0.0f, &hard, nullptr);
        n++;
        if (p == ls && !hard) break;  // safety: no progress
      }
    }
    if (n == 0) return rect_t{bounds.x, bounds.y, 0, 0};

    float total_h = (n - 1) * line_advance + font_height;
    int draw_count = n;
    bool truncate = false;
    if (overflow == ELLIPSES && line_advance > 0.0f && total_h > bounds.h) {
      int fit = (int)((bounds.h - font_height) / line_advance) + 1;
      if (fit < 1) fit = 1;
      if (n > fit) {
        draw_count = fit;
        truncate = true;
        total_h = (draw_count - 1) * line_advance + font_height;
      }
    }

    float y = bounds.y;
    if (align_v == MIDDLE)      y = bounds.y + (bounds.h - total_h) / 2.0f;
    else if (align_v == BOTTOM) y = bounds.y + bounds.h - total_h;

    static const char *ellipsis = "...";
    float ew = 0.0f;
    if (truncate)
      ew = vector ? self->image->font()->measure(self->image, ellipsis, fs).w
                  : self->image->pixel_font()->measure(self->image, ellipsis, scale).w;

    rect_t old_clip{0, 0, 0, 0};
    if (draw) {
      old_clip = self->image->clip();
      self->image->clip(xf ? bounds.transformed(*xf) : bounds);
    }

    // Pass 2: place each line at its aligned x, tracking the bounds (and
    // drawing, unless this is a measure-only run).
    float y0 = y;
    float min_x = bounds.x + bounds.w;
    float max_x = bounds.x;
    const char *p = text;
    for (int i = 0; i < draw_count; i++) {
      const char *line_start = p;
      bool hard;
      float lw = image_layout_line(self, vector, fs, scale, &p, end, bounds.w,
                                   space_w, false, 0.0f, 0.0f, &hard, nullptr);

      bool last_trunc = truncate && i == draw_count - 1;
      float eff_w = last_trunc ? lw + ew : lw;

      float ox = bounds.x;
      if (align_h == CENTER)     ox = bounds.x + (bounds.w - eff_w) / 2.0f;
      else if (align_h == RIGHT) ox = bounds.x + bounds.w - eff_w;

      if (ox < min_x) min_x = ox;
      if (ox + eff_w > max_x) max_x = ox + eff_w;

      if (draw) {
        const char *dp = line_start;
        image_layout_line(self, vector, fs, scale, &dp, end, bounds.w, space_w,
                          true, ox, y, &hard, xf);

        if (last_trunc) {
          text_cursor_t *c = self->image->text_cursor_state();
          float px = ox + lw;
          c->x = px; c->y = y; c->origin_x = px; c->valid = true;
          if (vector) self->image->font()->draw(self->image, ellipsis, fs, xf);
          else        self->image->pixel_font()->draw(self->image, ellipsis, scale, xf);
        }
      }
      y += line_advance;
    }

    if (draw) {
      self->image->clip(old_clip);
      // Leave the caret at the start of the line after the block. The per-word
      // draws each set origin_x to their own x and every font draw() ends with
      // an implicit newline back to it, so without this the caret would sit
      // under the last word of the last line.
      text_cursor_t *c = self->image->text_cursor_state();
      c->x = bounds.x; c->origin_x = bounds.x; c->y = y; c->valid = true;
    }

    float w = max_x - min_x;
    return rect_t{min_x, y0, w > 0.0f ? w : 0.0f, total_h};
  }

  // add_glyph(name, fn): register an inline [name] / [name:params] renderer for
  // text(). fn(image, params, measure) returns the advance width when measure is
  // True, else draws at image.cursor. Re-registering a name overwrites it;
  // fn=None removes it (a no-op if it was not registered). Static: (name, fn).
  mp_obj_t image_add_glyph(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    if (s_glyph_registry == MP_OBJ_NULL) return mp_const_none;
    if (args[1] == mp_const_none) {
      mp_map_lookup(mp_obj_dict_get_map(s_glyph_registry), args[0],
                    MP_MAP_LOOKUP_REMOVE_IF_FOUND);
    } else {
      mp_obj_dict_store(s_glyph_registry, args[0], args[1]);
    }
    return mp_const_none;
  }

  mp_obj_t image_text(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_text);
#endif
    const char *text = mp_obj_str_get_str(args[1]);
    if (!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }

    // Positional grammar (unchanged for back-compat) plus a rect that enables
    // the bounded layout engine:
    //   text(s)                 -> continue at the caret, font default size
    //   text(s, size)           -> continue at the caret, explicit size
    //   text(s, vec2 [, size])  -> single run at vec2 (resets the caret)
    //   text(s, x, y [, size])  -> single run at (x, y) (resets the caret)
    //   text(s, rect [, size])  -> word-wrapped, aligned layout inside rect
    // A lone trailing number is a size: a single number was never a position.
    bool has_at = false, has_rect = false;
    vec2_t at{0, 0};
    rect_t bounds{0, 0, 0, 0};
    float size = 0.0f;
    if (n_args >= 3 && mp_obj_is_rect(args[2])) {
      bounds = mp_obj_get_rect(args[2]);
      has_rect = true;
      if (n_args > 3) size = mp_obj_get_float(args[3]);
    } else if (n_args >= 3 && mp_obj_is_vec2(args[2])) {
      at = mp_obj_get_vec2(args[2]);
      has_at = true;
      if (n_args > 3) size = mp_obj_get_float(args[3]);
    } else if (n_args >= 4 && pv_is_num(args[2]) && pv_is_num(args[3])) {
      at.x = mp_obj_get_float(args[2]);
      at.y = mp_obj_get_float(args[3]);
      has_at = true;
      if (n_args > 4) size = mp_obj_get_float(args[4]);
    } else if (n_args == 3 && pv_is_num(args[2])) {
      size = mp_obj_get_float(args[2]);
    }

    // Keyword settings. font_size overrides a positional size; align, overflow,
    // line_height and word_spacing apply only to the rect layout path.
    text_align_t align_h = LEFT, align_v = TOP;
    text_overflow_t overflow = CLIP;
    float line_height = 1.0f, word_spacing = 1.0f;
    mat3_t xform;
    const mat3_t *xf = nullptr;   // nullptr is the untransformed fast path
    if (kw_args && kw_args->used) {
      mp_map_elem_t *e;
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_transform), MP_MAP_LOOKUP))
          && e->value != mp_const_none) {
        if (!mp_obj_is_type(e->value, &type_mat3)) {
          mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("transform must be a mat3"));
        }
        xform = ((mat3_obj_t *)MP_OBJ_TO_PTR(e->value))->m;
        xf = &xform;
      }
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_font_size), MP_MAP_LOOKUP)))
        size = mp_obj_get_float(e->value);
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_line_height), MP_MAP_LOOKUP)))
        line_height = mp_obj_get_float(e->value);
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_word_spacing), MP_MAP_LOOKUP)))
        word_spacing = mp_obj_get_float(e->value);
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_overflow), MP_MAP_LOOKUP)))
        overflow = (text_overflow_t)mp_obj_get_int(e->value);
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_align), MP_MAP_LOOKUP))) {
        mp_obj_t a = e->value;
        if (mp_obj_is_type(a, &mp_type_tuple) || mp_obj_is_type(a, &mp_type_list)) {
          size_t len; mp_obj_t *items;
          mp_obj_get_array(a, &len, &items);
          if (len >= 1) align_h = (text_align_t)mp_obj_get_int(items[0]);
          if (len >= 2) align_v = (text_align_t)mp_obj_get_int(items[1]);
        } else {
          align_h = (text_align_t)mp_obj_get_int(a);
        }
      }
    }

    // Bounded layout path. Returns the drawn bounding box.
    if (has_rect) {
      rect_t bb = image_layout_text(self, text, bounds, size, align_h, align_v,
                                    overflow, line_height, word_spacing, true, xf);
      return pv::box_rect(xf ? bb.transformed(*xf) : bb);
    }

    // Fast path: a single run from the caret / position (size is a sentinel-0
    // optional: 12pt vector default, 1x pixel default). Clipped to the image.
    text_cursor_t *c = self->image->text_cursor_state();
    if (has_at) {
      c->x = at.x; c->y = at.y; c->origin_x = at.x; c->valid = true;
    } else if (!c->valid) {
      c->x = 0.0f; c->y = 0.0f; c->origin_x = 0.0f; c->valid = true;
    }
    // Capture the start point, since draw() advances (and newlines) the caret.
    float bx = c->x, by = c->y;
    rect_t bb;
    if (self->font) {
      float fs = size > 0.0f ? size : 12.0f;
      self->image->font()->draw(self->image, text, fs, xf);
      bb = self->image->font()->measure(self->image, text, fs);
    } else {
      int scale = size > 0.0f ? (int)size : 1;
      self->image->pixel_font()->draw(self->image, text, scale, xf);
      bb = self->image->pixel_font()->measure(self->image, text, scale);
    }
    bb.x = bx; bb.y = by;
    return pv::box_rect(xf ? bb.transformed(*xf) : bb);
  }

  mp_obj_t image_measure_text(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_measure_text);
#endif
    const char *text = mp_obj_str_get_str(args[1]);
    if (!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }

    // Grammar mirrors text(): a rect measures word-wrapped inside those bounds;
    // a lone number is the size; otherwise measure the string unwrapped. size is
    // a sentinel-0 optional (12pt vector / 1x pixel default).
    bool has_rect = false;
    rect_t bounds{0, 0, 0, 0};
    float size = 0.0f;
    if (n_args >= 3 && mp_obj_is_rect(args[2])) {
      bounds = mp_obj_get_rect(args[2]);
      has_rect = true;
      if (n_args > 3) size = mp_obj_get_float(args[3]);
    } else if (n_args == 3 && pv_is_num(args[2])) {
      size = mp_obj_get_float(args[2]);
    }

    float line_height = 1.0f, word_spacing = 1.0f;
    if (kw_args && kw_args->used) {
      mp_map_elem_t *e;
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_font_size), MP_MAP_LOOKUP)))
        size = mp_obj_get_float(e->value);
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_line_height), MP_MAP_LOOKUP)))
        line_height = mp_obj_get_float(e->value);
      if ((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_word_spacing), MP_MAP_LOOKUP)))
        word_spacing = mp_obj_get_float(e->value);
    }

    mp_obj_t result[2];
    if (has_rect) {
      // measure-only layout (no draw); align does not affect the size
      rect_t bb = image_layout_text(self, text, bounds, size, LEFT, TOP, CLIP,
                                    line_height, word_spacing, false, nullptr);
      result[0] = mp_obj_new_float(bb.w);
      result[1] = mp_obj_new_float(bb.h);
      return mp_obj_new_tuple(2, result);
    }

    if (self->font) {
      rect_t r = self->image->font()->measure(self->image, text, size > 0.0f ? size : 12.0f);
      result[0] = mp_obj_new_float(r.w);
      result[1] = mp_obj_new_float(r.h);
    } else {
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
      // batch commands are positional (method, *args); text() takes no kwargs here
      mp_map_t no_kw;
      mp_map_init(&no_kw, 0);
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
        case MP_QSTR_text:       image_text(nh, handler_args, &no_kw); break;
        case MP_QSTR_blit_vspan: mpy_image_blit_vspan(nh, handler_args); break;
        case MP_QSTR_blit_hspan: mpy_image_blit_hspan(nh, handler_args); break;
        case MP_QSTR_blit:       mpy_image_blit(nh, handler_args); break;
        default: mp_raise_ValueError(MP_ERROR_TEXT("unknown method"));
      }
    }
    return mp_const_none;
  }

}
