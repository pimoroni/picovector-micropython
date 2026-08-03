// spritesheet: the members that build objects rather than call one function.
//
// The sequence arithmetic is all in picovector/spritesheet.hpp, where host tests
// cover it. What is here is the object plumbing: turning a cell into a sub-image
// view of the right type, reading timings out of whatever Python handed over, and
// carrying a sheet's settings into a narrower one.

#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

  // Which image type a buffer is presented as. Shared with image_native.cpp,
  // which owns it - a palettised buffer cannot be drawn into, so it gets the
  // type that does not offer to.
  extern const mp_obj_type_t *image_type_for(image_t *image);

  namespace {

    // Copy a sequence of milliseconds into an array the sheet owns. Returns the
    // count written, or 0 for "no per-frame timings".
    int read_delays(spritesheet_obj_t *self, mp_obj_t timings) {
      if(timings == mp_const_none) return 0;

      // A single number is a uniform rate, which the core stores directly and
      // which needs no array at all.
      if(mp_obj_is_int(timings) || mp_obj_is_float(timings)) {
        self->sheet.interval((int)mp_obj_get_float(timings));
        return 0;
      }

      size_t n;
      mp_obj_t *items;
      mp_obj_get_array(timings, &n, &items);
      if(n == 0) return 0;

      self->delays = (uint16_t *)m_malloc_no_scan(n * sizeof(uint16_t));
      for(size_t i = 0; i < n; i++) {
        mp_int_t ms = mp_obj_get_int(items[i]);
        // A frame cannot be shown for a negative time, and the array is 16-bit
        // because a GIF's own delay field is. 65535ms is over a minute a frame.
        if(ms < 0) ms = 0;
        if(ms > 0xffff) ms = 0xffff;
        self->delays[i] = (uint16_t)ms;
      }
      self->sheet.delays(self->delays, (int)n);
      return (int)n;
    }

    // A fresh sheet over the same image, with the same settings.
    spritesheet_obj_t *clone_sheet(spritesheet_obj_t *from) {
      spritesheet_obj_t *o = mp_obj_malloc(spritesheet_obj_t, &type_spritesheet);
      o->source = from->source;
      o->delays = nullptr;
      new (&o->sheet) spritesheet_t(from->sheet);
      // The copied core value still points at the parent's array, which is only
      // rooted through the parent's obj. Drop it; the caller re-derives per-frame
      // timings for the child's own range once that range is set.
      o->sheet.delays(nullptr, 0);
      return o;
    }

    // Give `child` the per-frame timings `parent` holds for the cells the child
    // covers. Looked up by cell, not by sequence position: the child's frame 0 is
    // somewhere else in the parent's sequence, so indexing by position would give
    // every frame the timing of a different one.
    void inherit_delays(spritesheet_obj_t *child, spritesheet_obj_t *parent) {
      if(parent->delays == nullptr || parent->sheet.delay_count() == 0) return;

      int n = child->sheet.frames();
      child->delays = (uint16_t *)m_malloc_no_scan((size_t)n * sizeof(uint16_t));
      for(int i = 0; i < n; i++) {
        int x, y;
        child->sheet.cell(i, &x, &y);
        int at = parent->sheet.index_of(x, y);
        // A cell the parent's range never covered has no timing of its own, so
        // it falls back to the interval the child already carries.
        child->delays[i] = at >= 0 ? (uint16_t)parent->sheet.delay_at(at)
                                   : (uint16_t)child->sheet.interval();
      }
      child->sheet.delays(child->delays, n);
    }

    mp_obj_t box_cell(spritesheet_obj_t *self, int x, int y) {
      image_t *source = self->source->image;
      // The sheet's own grid, not the image's: image_t::sprite divides by the
      // grid stored on the buffer, which a sheet deliberately does not touch.
      int sw = (int)source->bounds().w / self->sheet.cols();
      int sh = (int)source->bounds().h / self->sheet.rows();

      image_obj_t *result = mp_obj_malloc(image_obj_t, image_type_for(source));
      result->image = new (m_malloc(sizeof(image_t)))
        image_t(source, rect_t(x * sw, y * sh, sw, sh));
      result->parent = (void *)self->source;
      return MP_OBJ_FROM_PTR(result);
    }

    mp_obj_t box_index(spritesheet_obj_t *self, int index) {
      int x, y;
      self->sheet.cell(index, &x, &y);
      return box_cell(self, x, y);
    }

  }

  // image.spritesheet(cols=0, rows=0, timings=None) — the only way one is built.
  mp_obj_t image_spritesheet(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_spritesheet);
#endif
    int cols = n_args >= 2 ? mp_obj_get_int(args[1]) : 0;
    int rows = n_args >= 3 ? mp_obj_get_int(args[2]) : 0;
    mp_obj_t timings = n_args >= 4 ? args[3] : mp_const_none;

    // No grid asked for: use whatever the image carries. A loaded GIF has its
    // frames there, and anything else is 1x1 - the whole image as one cell.
    if(cols <= 0 && rows <= 0) {
      cols = (int)self->image->cols();
      rows = (int)self->image->rows();
      // ...and a GIF brings its own per-frame delays, which is the whole reason
      // the no-argument form exists.
      if(timings == mp_const_none && self->frame_delays != MP_OBJ_NULL)
        timings = self->frame_delays;
    } else {
      if(cols <= 0) cols = 1;
      if(rows <= 0) rows = 1;
    }

    spritesheet_obj_t *o = mp_obj_malloc(spritesheet_obj_t, &type_spritesheet);
    o->source = self;
    o->delays = nullptr;
    new (&o->sheet) spritesheet_t(cols, rows);
    read_delays(o, timings);
    return MP_OBJ_FROM_PTR(o);
  }

  // indexed_image.spritesheet — the same code; the obj structs are one struct.
  mp_obj_t indexed_image_spritesheet(size_t n_args, const mp_obj_t *args) {
    return image_spritesheet(n_args, args);
  }

  mp_obj_t spritesheet_sprite(size_t n_args, const mp_obj_t *args) {
    self(args[0], spritesheet_obj_t);
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    // Grid-absolute and clamped: a caller indexing an atlas is not walking a
    // sequence, and must not be able to window outside the buffer.
    if(x < 0) x = 0;
    if(x >= self->sheet.cols()) x = self->sheet.cols() - 1;
    if(y < 0) y = 0;
    if(y >= self->sheet.rows()) y = self->sheet.rows() - 1;
    return box_cell(self, x, y);
  }

  mp_obj_t spritesheet_frame(size_t n_args, const mp_obj_t *args) {
    self(args[0], spritesheet_obj_t);
    return box_index(self, mp_obj_get_int(args[1]));
  }

  mp_obj_t spritesheet_at(size_t n_args, const mp_obj_t *args) {
    self(args[0], spritesheet_obj_t);
    return box_index(self, self->sheet.index_at(mp_obj_get_int(args[1])));
  }

  mp_obj_t spritesheet_box_now(spritesheet_obj_t *self) {
    return box_index(self, self->sheet.index_now());
  }

  mp_obj_t spritesheet_start(size_t n_args, const mp_obj_t *args) {
    self(args[0], spritesheet_obj_t);
    if(n_args >= 2 && args[1] != mp_const_none) self->sheet.start(mp_obj_get_int(args[1]));
    else self->sheet.start();
    return args[0];   // chainable, as tween.start() is
  }

  // interval() reads the timings back; interval(x) sets them.
  mp_obj_t spritesheet_interval(size_t n_args, const mp_obj_t *args) {
    self(args[0], spritesheet_obj_t);
    if(n_args >= 2) {
      self->delays = nullptr;
      self->sheet.interval(0);
      read_delays(self, args[1]);
      return mp_const_none;
    }
    // One entry per frame whether or not they differ, so a caller can read the
    // timings of any sheet without checking which kind it is first.
    int n = self->sheet.frames();
    mp_obj_tuple_t *out = (mp_obj_tuple_t *)MP_OBJ_TO_PTR(mp_obj_new_tuple(n, NULL));
    for(int i = 0; i < n; i++) out->items[i] = MP_OBJ_NEW_SMALL_INT(self->sheet.delay_at(i));
    return MP_OBJ_FROM_PTR(out);
  }

  // range(origin, dest, interval=, loop=, direction=)
  mp_obj_t spritesheet_range(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    self(args[0], spritesheet_obj_t);
    spritesheet_obj_t *o = clone_sheet(self);

    size_t i = 1;
    if(i < n_args) {
      vec2_t origin = pv::get_xy(args, &i, n_args);
      vec2_t dest = i < n_args ? pv::get_xy(args, &i, n_args) : origin;
      o->sheet.range((int)origin.x, (int)origin.y, (int)dest.x, (int)dest.y);
    } else {
      o->sheet.range(0, 0, o->sheet.cols() - 1, o->sheet.rows() - 1);
    }

    mp_map_elem_t *e;
    // Direction before the timings: it decides the sequence order, and the
    // per-frame timings are derived per cell in that order.
    if((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_direction), MP_MAP_LOOKUP)))
      o->sheet.direction((sheet_direction_t)mp_obj_get_int(e->value));
    if((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_loop), MP_MAP_LOOKUP)))
      o->sheet.loop(mp_obj_is_true(e->value));

    if((e = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_interval), MP_MAP_LOOKUP))) {
      read_delays(o, e->value);
    } else {
      // No interval asked for, so carry the parent's timings across for whatever
      // cells this range covers.
      inherit_delays(o, self);
    }
    return MP_OBJ_FROM_PTR(o);
  }

}
