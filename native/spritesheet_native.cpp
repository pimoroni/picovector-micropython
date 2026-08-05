// spritesheet: the members that build objects rather than call one function.
//
// The numbering lives in picovector/spritesheet.hpp, where host tests cover it.
// Here: turning a cell into a sub-image view of the right type, and reporting
// what a GIF said about its frame durations.

#include "pv_bindings.hpp"
#include "types.h"

extern "C" {

  extern mp_obj_t image_load(size_t n_args, const mp_obj_t *args);

  namespace {

    mp_obj_t box_cell(spritesheet_obj_t *self, int x, int y) {
      image_t *source = self->source->image;
      // The sheet's own grid, not the image's: image_t::sprite divides by the
      // grid stored on the buffer, which a sheet does not touch.
      int sw = (int)source->bounds().w / self->sheet.cols();
      int sh = (int)source->bounds().h / self->sheet.rows();

      image_obj_t *result = mp_obj_malloc(image_obj_t, &type_image);
      result->image = new (m_malloc(sizeof(image_t)))
        image_t(source, rect_t(x * sw, y * sh, sw, sh));
      result->parent = (void *)self->source;
      return MP_OBJ_FROM_PTR(result);
    }

  }

  // image.spritesheet(cols=0, rows=0)
  mp_obj_t image_spritesheet(size_t n_args, const mp_obj_t *args) {
    self(args[0], image_obj_t);
#if PV_METRICS
    pv::metric_scope _pvm(PV_M_image_spritesheet);
#endif
    int cols = n_args >= 2 ? pv::to_int(args[1]) : 0;
    int rows = n_args >= 3 ? pv::to_int(args[2]) : 0;

    // No grid asked for: whatever the image carries. A loaded GIF has its frames
    // there; anything else is 1x1, the whole image as one cell.
    if(cols <= 0 && rows <= 0) {
      cols = (int)self->image->cols();
      rows = (int)self->image->rows();
    } else {
      if(cols <= 0) cols = 1;
      if(rows <= 0) rows = 1;
    }

    spritesheet_obj_t *o = mp_obj_malloc(spritesheet_obj_t, &type_spritesheet);
    o->source = self;
    new (&o->sheet) spritesheet_t(cols, rows);
    return MP_OBJ_FROM_PTR(o);
  }

  // spritesheet.load(path_or_bytes, cols=0, rows=0). Forwards to both natives so
  // format detection lives in one place; the image keeps the file's own format.
  mp_obj_t spritesheet_load(size_t n_args, const mp_obj_t *args) {
    mp_obj_t image = image_load(1, args);   // path only; a sheet is not resized
    mp_obj_t forward[3] = {
      image,
      n_args >= 2 ? args[1] : MP_OBJ_NEW_SMALL_INT(0),
      n_args >= 3 ? args[2] : MP_OBJ_NEW_SMALL_INT(0),
    };
    return image_spritesheet(3, forward);
  }

  // sprite(n) | sprite(x, y): a cell by number or by coordinate, told apart by
  // arity. An atlas asks by coordinate, an animation by number.
  mp_obj_t spritesheet_sprite(size_t n_args, const mp_obj_t *args) {
    self(args[0], spritesheet_obj_t);
    int x, y;
    if(n_args >= 3) {
      x = pv::to_int(args[1]);
      y = pv::to_int(args[2]);
      // Clamped, so a caller cannot window outside the buffer. locate() clamps
      // the by-number form itself.
      if(x < 0) x = 0;
      if(x >= self->sheet.cols()) x = self->sheet.cols() - 1;
      if(y < 0) y = 0;
      if(y >= self->sheet.rows()) y = self->sheet.rows() - 1;
    } else {
      self->sheet.locate(pv::to_int(args[1]), &x, &y);
    }
    return box_cell(self, x, y);
  }

  // A GIF's declared frame durations, or None when the file gave none.
  mp_obj_t spritesheet_box_timings(spritesheet_obj_t *self) {
    mp_obj_t delays = self->source->frame_delays;
    return delays == MP_OBJ_NULL ? mp_const_none : delays;
  }

}
