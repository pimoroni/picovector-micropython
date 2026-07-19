// AUTO-GENERATED from api/vector_font.py by generate.py — do not edit by hand.
#include "pv_bindings.hpp"
#include "types.h"

extern "C" {


static void vector_font_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  mp_printf(print, "font()");
}

MP_DEFINE_CONST_OBJ_TYPE(
  type_vector_font,
  MP_QSTR_vector_font,
  MP_TYPE_FLAG_NONE,
  print, (const void *)vector_font_print
);

}
