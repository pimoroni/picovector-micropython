// AUTO-GENERATED from api/pixel_font.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {


static void pixel_font_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  self(self_in, pixel_font_obj_t);
  mp_printf(print, "pixel_font(\"%s\")", self->path);
}

void pixel_font_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
  self(self_in, pixel_font_obj_t);
  action_t action = m_attr_action(dest);
  switch (attr) {
    case MP_QSTR_height:
    {
      if (action == GET) { dest[0] = mp_obj_new_int(self->font->height); return; }
    }
    case MP_QSTR_name:
    {
      if (action == GET) { dest[0] = mp_obj_new_str(self->font->name, strlen(self->font->name)); return; }
    }
  }
  dest[1] = MP_OBJ_SENTINEL;
}

MP_DEFINE_CONST_OBJ_TYPE(
  type_pixel_font,
  MP_QSTR_pixel_font,
  MP_TYPE_FLAG_NONE,
  print, (const void *)pixel_font_print,
  attr, (const void *)pixel_font_attr
);

}
