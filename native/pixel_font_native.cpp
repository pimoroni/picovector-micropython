// native/pixel_font_native.cpp — the .ppf (bitmap) font parser.
//
// Invoked by the `font` loader (native/font_native.cpp) once it has sniffed a
// "ppf!" marker; there is no pixel_font.load. See native/font_native.cpp for
// why this file/heap work lives here rather than being generated.

#include "pv_bindings.hpp"

extern "C" {
  #include <inttypes.h>
  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"
  #include "extmod/vfs.h"

  // `file` is an open stream positioned just after the 4-byte "ppf!" marker;
  // `path` is the resolved file path, copied onto the object for its repr.
  mp_obj_t parse_pixel_font(mp_obj_t file, const char *path) {
    pixel_font_obj_t *result =
        mp_obj_malloc(pixel_font_obj_t, &type_pixel_font);

    size_t plen = strlen(path);
    result->path = (char *)m_malloc_no_scan(plen + 1);
    memcpy(result->path, path, plen + 1);

    int error;
    uint16_t flags        = ru16(file);
    uint32_t glyph_count  = ru32(file);
    uint16_t glyph_width  = ru16(file);
    uint16_t glyph_height = ru16(file);
    (void)flags;

    char name[32];
    mp_stream_read_exactly(file, name, sizeof(name), &error);

    uint32_t bpr = floor((glyph_width + 7) / 8);
    uint32_t glyph_data_size = bpr * glyph_height;

    // glyph table (codepoint+width) and raw pixel data are pointer-free, so
    // their buffers need no GC scanning.
    result->glyph_buffer_size = sizeof(pixel_font_glyph_t) * glyph_count;
    result->glyph_buffer = (uint8_t *)m_malloc_no_scan(result->glyph_buffer_size);
    result->glyph_data_buffer_size = glyph_data_size * glyph_count;
    result->glyph_data_buffer = (uint8_t *)m_malloc_no_scan(result->glyph_data_buffer_size);
    if (!result->glyph_buffer || !result->glyph_data_buffer) {
      mp_raise_msg_varg(&mp_type_OSError,
                        MP_ERROR_TEXT("couldn't allocate buffer for font data"));
    }

    pixel_font_glyph_t *glyphs = (pixel_font_glyph_t *)result->glyph_buffer;
    for (uint32_t i = 0; i < glyph_count; i++) {
      glyphs[i].codepoint = ru32(file);
      glyphs[i].width = ru16(file);
    }
    mp_stream_read_exactly(file, result->glyph_data_buffer,
                           result->glyph_data_buffer_size, &error);

    result->font = m_new_class(pixel_font_t);
    result->font->glyph_count     = glyph_count;
    result->font->width           = glyph_width;
    result->font->height          = glyph_height;
    result->font->glyph_data_size = glyph_data_size;
    result->font->glyphs          = glyphs;
    result->font->glyph_data      = result->glyph_data_buffer;
    strcpy(result->font->name, name);

    mp_stream_close(file);
    return MP_OBJ_FROM_PTR(result);
  }

}
