"""vector_font — a scalable vector font loaded from a .af file.

Instances are produced by ``font.load()`` (the loader sniffs the file and
returns a ``vector_font`` or a ``pixel_font``); there is no ``vector_font.load``.
Parsing the binary .af container is procedural file/heap work done in
``native/font_native.cpp``. The DSL owns only the type's registration and
docstring, so instances answer ``isinstance(f, vector_font)``.
"""

from __future__ import annotations

from pv import api


@api("font_t", field="font", print=("font()",),
     arg_read="&((vector_font_obj_t *)MP_OBJ_TO_PTR({0}))->font", arg_type="font_t *")
class vector_font:
    """A scalable vector font (.af). Draw at a point size via image.text()."""
