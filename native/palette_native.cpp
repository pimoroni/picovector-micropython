// palette: the colour table as a Python sequence.
//
// A subscript has to tell an index from a slice and a read from a write, and
// build a list for the slice case, so the slot is hand-written; the generator
// wires it in and supplies len() and the buffer protocol.
//
// An attached palette is a handle onto the image's own table, not a copy. Views
// share that table by pointer, so writing one entry recolours every sprite cut
// from the same sheet - and it is also why assigning a palette copies the entries
// in rather than swapping the pointer.

#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

  // Where this palette's entries actually live: the image's table when attached,
  // its own array when free-standing.
  uint32_t *palette_entries(palette_obj_t *self) {
    if(self->source) return self->source->image->palette_data();
    return self->entries;
  }

  namespace {

    int palette_count(palette_obj_t *self) {
      if(self->source) return self->source->image->palette_size();
      return self->count;
    }

    // Read one colour out of a sequence of them. Anything that is not a colour
    // is a TypeError here rather than a silently premultiplied integer.
    uint32_t color_arg(mp_obj_t o) {
      if(!mp_obj_is_type(o, &type_color)) {
        mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("palette entries must be colours"));
      }
      return ((color_obj_t *)MP_OBJ_TO_PTR(o))->c._p;
    }

    void store(palette_obj_t *self, int i, mp_obj_t value) {
      palette_entries(self)[i] = color_arg(value);
    }

  }

  // palette(colors)
  mp_obj_t palette_make_new_impl(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *args) {
    size_t n;
    mp_obj_t *items;
    mp_obj_get_array(args[0], &n, &items);
    if(n < 1 || n > 256) {
      mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("a palette holds 1 to 256 colours"));
    }

    palette_obj_t *self = mp_obj_malloc(palette_obj_t, type);
    self->source = nullptr;
    self->count = (uint16_t)n;
    self->entries = (uint32_t *)m_malloc_no_scan(n * sizeof(uint32_t));
    for(size_t i = 0; i < n; i++) self->entries[i] = color_arg(items[i]);
    return MP_OBJ_FROM_PTR(self);
  }

  // A handle onto an image's table, or none when it has none. Used by the
  // image.palette getter on both image types.
  mp_obj_t palette_box(image_obj_t *image) {
    if(!image->image->has_palette()) return mp_const_none;
    palette_obj_t *self = mp_obj_malloc(palette_obj_t, &type_palette);
    self->source = image;
    self->entries = nullptr;
    self->count = (uint16_t)image->image->palette_size();
    return MP_OBJ_FROM_PTR(self);
  }

  // image.palette = <palette or sequence of colours>
  //
  // Copies the entries into the table that is already there. A pointer swap would
  // be cheaper and wrong: every window()/sprite() of this image holds its own
  // copy of image_t::_palette, so they would keep showing the old colours.
  void palette_assign(image_obj_t *image, mp_obj_t value) {
    if(!image->image->has_palette()) {
      mp_raise_msg(&mp_type_TypeError,
                   MP_ERROR_TEXT("image has no colour table to assign to"));
    }
    int have = image->image->palette_size();

    if(mp_obj_is_type(value, &type_palette)) {
      palette_obj_t *from = (palette_obj_t *)MP_OBJ_TO_PTR(value);
      int n = palette_count(from);
      if(n > have) {
        mp_raise_msg_varg(&mp_type_ValueError,
                          MP_ERROR_TEXT("palette has %u colours, the table holds %u"),
                          (unsigned)n, (unsigned)have);
      }
      const uint32_t *src = palette_entries(from);
      for(int i = 0; i < n; i++) image->image->palette((uint8_t)i, src[i]);
      return;
    }

    size_t n;
    mp_obj_t *items;
    mp_obj_get_array(value, &n, &items);
    if((int)n > have) {
      mp_raise_msg_varg(&mp_type_ValueError,
                        MP_ERROR_TEXT("palette has %u colours, the table holds %u"),
                        (unsigned)n, (unsigned)have);
    }
    for(size_t i = 0; i < n; i++) image->image->palette((uint8_t)i, color_arg(items[i]));
  }

  // palette[i], palette[i] = c, palette[a:b], palette[a:b] = [...]
  mp_obj_t palette_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value) {
    self(self_in, palette_obj_t);
    int count = palette_count(self);

    if(value == MP_OBJ_NULL) {
      // A table is a fixed number of slots; there is no entry to remove, and
      // pretending otherwise would renumber every pixel's index.
      mp_raise_msg(&mp_type_TypeError, MP_ERROR_TEXT("cannot delete a palette entry"));
    }

#if MICROPY_PY_BUILTINS_SLICE
    if(mp_obj_is_type(index_in, &mp_type_slice)) {
      mp_bound_slice_t slice;
      if(!mp_seq_get_fast_slice_indexes(count, index_in, &slice)) {
        mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("only a step of 1 is supported"));
      }
      int n = slice.stop > slice.start ? (int)(slice.stop - slice.start) : 0;

      if(value == MP_OBJ_SENTINEL) {
        mp_obj_list_t *out = (mp_obj_list_t *)MP_OBJ_TO_PTR(mp_obj_new_list(n, NULL));
        for(int i = 0; i < n; i++)
          out->items[i] = pv::box_color_packed(palette_entries(self)[slice.start + i]);
        return MP_OBJ_FROM_PTR(out);
      }

      size_t given;
      mp_obj_t *items;
      mp_obj_get_array(value, &given, &items);
      if((int)given != n) {
        mp_raise_msg_varg(&mp_type_ValueError,
                          MP_ERROR_TEXT("cannot assign %u colours to %u slots"),
                          (unsigned)given, (unsigned)n);
      }
      for(int i = 0; i < n; i++) store(self, (int)slice.start + i, items[i]);
      return mp_const_none;
    }
#endif

    mp_int_t i = pv::to_int(index_in);
    if(i < 0) i += count;                       // palette[-1] is the last entry
    if(i < 0 || i >= count) {
      mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("palette index out of range"));
    }

    if(value == MP_OBJ_SENTINEL) {
      return pv::box_color_packed(palette_entries(self)[i]);
    }
    store(self, (int)i, value);
    return mp_const_none;
  }

  // Also a method, because MicroPython's iteration looks the name up rather than
  // using the slot - which is what makes list(palette) and `for c in palette` go.
  mp_obj_t palette___getitem__(size_t n_args, const mp_obj_t *args) {
    return palette_subscr(args[0], args[1], MP_OBJ_SENTINEL);
  }

}
