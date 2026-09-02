"""api.pico3d — the pico3d MicroPython surface, as type-annotated stubs.

A separate module (``import pico3d``) rather than more names in ``picovector``:
the engine is a fixed-function 3D rasteriser with its own vocabulary, and a
``surface`` next to an ``image`` in one namespace would only read as a synonym.
The two still share types where it matters - a texture is a picovector ``image``
and every colour is a picovector ``color``.

Imported in dependency order, so each type is registered before the stubs that
annotate against it.
"""

from . import vec3        # noqa: F401
from . import mat4        # noqa: F401
from . import mesh        # noqa: F401
from . import material    # noqa: F401
from . import light       # noqa: F401
from . import surface     # noqa: F401
from . import engine      # noqa: F401
