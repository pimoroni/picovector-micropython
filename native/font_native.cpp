// native/font_native.cpp — the `font` namespace singleton + the font loader.
//
// `font` is a namespace object, NOT a font type: `font.load(name)` sniffs a
// file's magic marker and returns a vector_font or a pixel_font, resolving bare
// short names against a set of search paths; `font.<name>` loads a ROM pixel
// font by short name (cached); `dir(font)` lists the ROM fonts. The .af
// container is parsed by the core library (picovector/font_parse.cpp); what's
// here is the stream shim it reads through and the search-path resolution. The
// .ppf parser lives in native/pixel_font_native.cpp.

#include "pv_bindings.hpp"

extern "C" {
  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"
  #include "extmod/vfs.h"

  // ── .af (vector) parser ───────────────────────────────────────────────────
  // Copy a resolved path into a GC block for a font object's repr. The block is
  // pointer-free; the scanned obj holds it alive.
  static char *dup_path(const char *path) {
    size_t n = strlen(path);
    char *p = (char *)m_malloc_no_scan(n + 1);
    memcpy(p, path, n + 1);
    return p;
  }

  // Byte source the core parser reads a font through. A short read is how it
  // detects truncation, so an error is reported as zero bytes.
  static size_t font_stream_read(void *handle, void *dest, size_t len) {
    int error;
    mp_uint_t read = mp_stream_read_exactly((mp_obj_t)handle, dest, len, &error);
    return error ? 0 : (size_t)read;
  }

  // Re-position an open stream (see the same shim in native/image_png.cpp).
  static void font_stream_seek(mp_obj_t file, int32_t pos) {
    struct mp_stream_seek_t seek_s;
    seek_s.offset = pos;
    seek_s.whence = SEEK_SET;

    const mp_stream_p_t *stream_p = mp_get_stream(file);
    int error;
    mp_uint_t res = stream_p->ioctl(file, MP_STREAM_SEEK, (mp_uint_t)(uintptr_t)&seek_s, &error);
    if (res == MP_STREAM_ERROR) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("font: seek failed with %d"), error);
    }
  }

  // mp_raise_* is noreturn, so cases need no break.
  static void raise_font_error(font_status_t status, const char *path) {
    switch (status) {
      case FONT_TRUNCATED:
        mp_raise_msg_varg(&mp_type_OSError,
                          MP_ERROR_TEXT("'%s' is truncated"), path);
      case FONT_UNSUPPORTED_FLAGS:
        mp_raise_msg_varg(&mp_type_OSError,
                          MP_ERROR_TEXT("'%s' needs a newer firmware to read"), path);
      case FONT_BAD_HEADER:
        mp_raise_msg_varg(&mp_type_OSError,
                          MP_ERROR_TEXT("'%s' is corrupt"), path);
      case FONT_MEM_ERROR:
        mp_raise_msg_varg(&mp_type_OSError,
                          MP_ERROR_TEXT("couldn't allocate buffer for font data"));
      default:
        mp_raise_msg_varg(&mp_type_OSError,
                          MP_ERROR_TEXT("'%s' could not be loaded (%d)"), path, (int)status);
    }
  }

  // `file` is an open stream positioned at the start of the .af. The parser
  // builds one self-contained non-scanned block (its internal glyph->paths /
  // path->points pointers stay within it, kept alive by the scanned obj).
  mp_obj_t load_vector_font(mp_obj_t file, const char *path) {
    vector_font_obj_t *result = mp_obj_malloc(vector_font_obj_t, &type_vector_font);
    result->path = dup_path(path);
    // The object is scanned and mp_obj_malloc doesn't zero it, so the buffer
    // pointer must be valid before anything below can raise.
    result->buffer = NULL;
    result->buffer_size = 0;

    font_reader_t reader = { font_stream_read, (void *)file };
    size_t buffer_size = 0;
    font_status_t status = parse_vector_font(reader, &result->font,
                                             &result->buffer, &buffer_size);
    mp_stream_close(file);
    if (status != FONT_OK) raise_font_error(status, path);
    result->buffer_size = buffer_size;
    return MP_OBJ_FROM_PTR(result);
  }

  // ── search-path resolution + magic sniff ──────────────────────────────────
  static const char *const FONT_SEARCH_PATHS[] = {
    "/rom/fonts", "/system/assets/fonts", "/fonts", "/assets", ""
  };
  static const char *const FONT_EXTS[] = { ".af", ".ppf" };

  static mp_obj_t font_open_read(const char *path) {
    mp_obj_t open_args[2] = { mp_obj_new_str(path, strlen(path)),
                              MP_ROM_QSTR(MP_QSTR_r) };
    return mp_vfs_open(MP_ARRAY_SIZE(open_args), open_args,
                       (mp_map_t *)&mp_const_empty_map);
  }

  static bool name_has_ext(const char *name) {
    size_t n = strlen(name);
    if (n >= 3 && strcmp(name + n - 3, ".af") == 0) return true;
    if (n >= 4 && strcmp(name + n - 4, ".ppf") == 0) return true;
    return false;
  }

  // Read the 4-byte marker and dispatch to the matching parser, passing the
  // resolved `path` (stored on the font for its repr). Consumes/closes `file`
  // on error. The .af parser checks the marker itself, so the stream is rewound
  // for it; the .ppf parser expects to start just past its own.
  static mp_obj_t parse_by_marker(mp_obj_t file, const char *path) {
    int error;
    char marker[4];
    mp_stream_read_exactly(file, &marker, sizeof(marker), &error);
    if (memcmp(marker, "af!?", 4) == 0) {
      font_stream_seek(file, 0);
      return load_vector_font(file, path);
    }
    if (memcmp(marker, "ppf!", 4) == 0) return parse_pixel_font(file, path);
    mp_stream_close(file);
    mp_raise_msg_varg(&mp_type_OSError,
                      MP_ERROR_TEXT("'%s' is not a font (bad magic marker)"), path);
    return mp_const_none;  // unreachable
  }

  static mp_obj_t font_load_name(mp_obj_t name_obj) {
    const char *name = mp_obj_str_get_str(name_obj);
    if (strchr(name, '/')) {
      // an explicit path: open directly (raises OSError if missing).
      return parse_by_marker(font_open_read(name), name);
    }
    // a bare name: probe the search paths. If it already carries a .af/.ppf
    // extension, use it as-is; otherwise try both extensions. vstr keeps the
    // candidate path on the heap (no per-iteration stack buffer).
    static const char *const NO_EXT[] = { "" };
    const char *const *exts = name_has_ext(name) ? NO_EXT : FONT_EXTS;
    size_t n_exts = name_has_ext(name) ? 1 : MP_ARRAY_SIZE(FONT_EXTS);

    vstr_t vs;
    vstr_init(&vs, 64);
    for (size_t p = 0; p < MP_ARRAY_SIZE(FONT_SEARCH_PATHS); p++) {
      for (size_t e = 0; e < n_exts; e++) {
        vstr_reset(&vs);
        vstr_printf(&vs, "%s/%s%s", FONT_SEARCH_PATHS[p], name, exts[e]);
        const char *path = vstr_null_terminated_str(&vs);
        if (mp_vfs_import_stat(path) == MP_IMPORT_STAT_FILE) {
          // parse (which copies `path`) before releasing the vstr buffer.
          mp_obj_t f = parse_by_marker(font_open_read(path), path);
          vstr_clear(&vs);
          return f;
        }
      }
    }
    vstr_clear(&vs);
    mp_raise_msg_varg(&mp_type_OSError,
                      MP_ERROR_TEXT("Font \"%s\" not found!"), name);
    return mp_const_none;  // unreachable
  }

  // font.load(name) — the sole loader (a factory returning vector_font/pixel_font).
  mp_obj_t font_load(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return font_load_name(args[0]);
  }
  static MP_DEFINE_CONST_FUN_OBJ_VAR(pv_font_load_obj, 1, font_load);

  // ── ROM short-name lookup (cached), backing font.<name> ────────────────────
  static mp_obj_t font_rom_get(qstr q) {
    if (MP_STATE_VM(pv_font_rom_cache) == MP_OBJ_NULL) {
      MP_STATE_VM(pv_font_rom_cache) = mp_obj_new_dict(0);
    }
    mp_obj_t cache = MP_STATE_VM(pv_font_rom_cache);
    mp_obj_t key = MP_OBJ_NEW_QSTR(q);
    mp_map_elem_t *e = mp_map_lookup(mp_obj_dict_get_map(cache), key, MP_MAP_LOOKUP);
    if (e != NULL) return e->value;

    vstr_t vs;
    vstr_init(&vs, 48);
    vstr_printf(&vs, "/rom/fonts/%s.ppf", qstr_str(q));
    const char *path = vstr_null_terminated_str(&vs);
    if (mp_vfs_import_stat(path) != MP_IMPORT_STAT_FILE) {
      vstr_clear(&vs);
      return MP_OBJ_NULL;  // miss -> AttributeError
    }
    // parse (which copies `path`) before releasing the vstr buffer.
    mp_obj_t f = parse_by_marker(font_open_read(path), path);
    vstr_clear(&vs);
    mp_obj_dict_store(cache, key, f);
    return f;
  }

  // dir(font) — a lazy scan of /rom/fonts, returning the .ppf stems.
  static mp_obj_t font_ns_dir(mp_obj_t self_in) {
    (void)self_in;
    mp_obj_t list = mp_obj_new_list(0, NULL);
    mp_obj_t dir_args[1] = { mp_obj_new_str("/rom/fonts", 10) };
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iter = mp_getiter(mp_vfs_ilistdir(1, dir_args), &iter_buf);
    mp_obj_t item;
    while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
      size_t n; mp_obj_t *fields;
      mp_obj_get_array(item, &n, &fields);
      const char *nm = mp_obj_str_get_str(fields[0]);
      size_t len = strlen(nm);
      if (len > 4 && strcmp(nm + len - 4, ".ppf") == 0) {
        mp_obj_list_append(list, mp_obj_new_str(nm, len - 4));
      }
    }
    return list;
  }
  static MP_DEFINE_CONST_FUN_OBJ_1(pv_font_dir_obj, font_ns_dir);

  // ── the `font` namespace object ────────────────────────────────────────────
  // `load` is reserved (resolved to the loader before any ROM lookup), so a ROM
  // glyph file named load.ppf can never shadow font.load.
  static void font_ns_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (dest[0] != MP_OBJ_NULL) return;  // load-only namespace (no set/delete)
    if (attr == MP_QSTR_load) {
      dest[0] = MP_OBJ_FROM_PTR(&pv_font_load_obj);
      return;
    }
    if (attr == MP_QSTR___dir__) {
      dest[0] = MP_OBJ_FROM_PTR(&pv_font_dir_obj);
      dest[1] = self_in;  // bound method
      return;
    }
    mp_obj_t f = font_rom_get(attr);
    if (f != MP_OBJ_NULL) dest[0] = f;  // else left NULL -> AttributeError
  }

  static MP_DEFINE_CONST_OBJ_TYPE(
    type_font_ns,
    MP_QSTR_font,
    MP_TYPE_FLAG_NONE,
    attr, (const void *)font_ns_attr
  );

  const font_ns_obj_t pv_font_ns_obj = { { &type_font_ns } };
}

// The ROM font cache, rooted so cached pixel_fonts keep their identity across
// GC cycles (so `font.sins is font.sins`).
MP_REGISTER_ROOT_POINTER(mp_obj_t pv_font_rom_cache);
