# PicoVector — MicroPython bindings

The MicroPython binding layer for the [PicoVector](../picovector) core library:
the `picovector` C extension module (`vec2`, `rect`, `mat3`, `color`, `brush`,
`shape`, `image`, `font`, `pixel_font`, `algorithm`) plus the glue that wires the
rasteriser onto MicroPython's GC heap.

The bindings are described in Python and generated. The stubs under [`api/`](api/)
read like a `.pyi` of the library *as if it were written in Python* — types,
value ranges and full docstrings — and [`generate.py`](generate.py) emits the
binding `.cpp` from them, synthesising every argument check, conversion, overload
dispatch and the type/locals-dict/module boilerplate, on top of a set of lean
inline helpers in [`runtime/pv_bindings.hpp`](runtime/pv_bindings.hpp).

## Why

A single, human-readable **Python source of ground truth** describes — with
caveats — exactly how the API should look and feel *in the target language
itself*, instead of as a dense wall of C/C++ macro expansions. The generator then
removes the whole class of by-hand argument-count/type bugs that binding glue is
prone to.

## Layout

```
pv.py            authoring surface the stubs import (decorators, pseudo-types, Range)
api/*.py         one stub class per type (vec2, rect, mat3, color, brush, shape,
                 image, font, pixel_font, algorithm) — the human-edited source
model.py         internal IR + the loader that introspects the stubs
generate.py      IR -> C++ (run this to (re)generate)
picovector.config.hpp   MicroPython config: routes the core onto the GC heap and
                        sets the build knobs (selected by the core via __has_include)
runtime/
  pv_bindings.hpp lean inline helpers used by the generated code
  pv_support.cpp  shared glue (attr action, file readers, vec2/rect/brush helpers)
  pv_objs.hpp     picovector MicroPython obj structs + type_* externs
  mp_allocator.hpp  std::vector allocator: scan vs no-scan by element type
  pv_metrics.{hpp,cpp}  optional per-binding call/time metrics (picovector.metrics)
native/*.cpp     hand-written bodies for the few irreducibly-procedural members,
                 plus the companion PNG/JPEG decoders (image_png/jpeg.cpp)
generated/       emitted output: <type>.cpp, types.h, picovector_bindings.c,
                 pv_metrics_table.h, pv_metrics_names.cpp
picovector-micropython.cmake   binding sources, include dirs + build knobs
```

## Authoring (the stub DSL)

A type is a `@pv.api`-decorated class. Methods are plain `def`s with type hints
and docstrings; class-level annotations are read/write fields. Convention covers
the common case (Python name == C++ callee, Python arg order == call order);
the small [`@pv.cpp`](pv.py) decorator handles the exceptions.

```python
@api("vec2_t", field="v", arg_read="mp_obj_get_vec2({0})", arg_type="vec2_t",
     print=("vec(%f, %f)", "self->v.x", "self->v.y"))
class vec2:
    """2D vector / point."""
    x: float
    y: float
    def __init__(self, x: float = 0, y: float = 0): "vec2() or vec2(x, y)."

    def dot(self, other: vec2) -> float: "Dot product with another vec2."
    def normalized(self) -> vec2:        "Unit vector in the same direction."
    def __add__(self, other: vec2) -> vec2: "Component-wise addition."
```

Things convention can't express, declared with `@cpp`/pseudo-types:

* **vec2-or-(x, y)** arguments → the `XY` pseudo-type (also `XYWH`, `ColorStops`,
  `Pattern8`, `PathList`, `ShapeOrList`, `Filter`).
* **argument reordering** (the C++ free function differs from the Python order)
  → `@cpp(args="c.x c.y from_a to_a inner outer")`.
* **overloads** → `typing.overload`; the generator emits guarded dispatch.
* **value ranges** → `Annotated[int, Range(0, 37)]` (raise) or
  `Range(1, None, clamp=True)` (clamp).
* **return semantics** → `-> Self` (return self for chaining) vs `-> vec2`
  (return a new boxed object); `@cpp(emit="mutate")` for in-place writes.
* **special callees** → `@cpp(call="rgb_color_t", emit="free")` (a plain
  temporary, e.g. boxed by value), `emit="mnew"` (GC `m_new_class`),
  `emit="new"` (raw `new` — **not** GC-tracked, avoid for owned heap),
  `recv="src"` (call on an argument, e.g. `image.blit`).

## Memory & GC

Boxed values are kept lean and GC-safe:

* **colour is stored by value** in `color_obj_t` (only its premultiplied `_p` is
  used downstream), so `color.rgb(...)` is a single GC-managed allocation — no
  pointer indirection and no `new`-allocated `color_t` leaking off the GC heap.
* The `std::vector` allocator (`runtime/mp_allocator.hpp`) routes backing
  buffers by element type via the **fail-safe** `mp_no_scan<T>` trait: provably
  pointer-free elements skip GC scanning (`m_malloc_no_scan`), everything else is
  scanned (`m_malloc`). "Provably pointer-free" is `is_arithmetic || is_enum`
  (so `uint32_t` palettes/LUTs are automatic), plus audited POD aggregates that
  opt in with `MP_POINTER_FREE(T)` (currently `vec2_t`). It deliberately does
  **not** use `std::is_trivially_copyable`, which is true for raw pointers and
  pointer-bearing PODs and would route them to no-scan — letting the GC free the
  pointee. So `vector<path_t>` (which embeds a nested `std::vector` pointer)
  stays scanned, and the GC can't free a shape's point buffers out from under
  it. Pointer-free font/pixel-font glyph buffers also use `m_malloc_no_scan`.

The MicroPython config — [`picovector.config.hpp`](picovector.config.hpp) — is
what points the core library's allocators at MicroPython's heap (`PV_MALLOC` →
`m_malloc`, `PV_STD_ALLOCATOR` → the allocator above). The core selects it
automatically via `__has_include` when this component is on the include path.

## Generate

```
python generate.py            # writes generated/*.cpp, types.h, picovector_bindings.c
python generate.py --list     # print the surface
```

After editing any `api/*.py` stub, run `python generate.py` to refresh
`generated/`, then rebuild.

## The native tail

A few members build nested Python containers, dispatch by name, decode files or
track a parent view — none reduce to "call one C++ function and box the result".

These are declared in the DSL (so it stays the single source of truth for the
*surface*, registration and docs) but marked `@native`, with bodies in
[`native/`](native/):

* `font.load` / `pixel_font.load` (+ their `__del__`) — binary font parsing.
* `image.load` / `load_into` / `window` / `text` / `measure_text` / `shapes` /
  `batch` — file decode, font branching, parent tracking, batched render, qstr
  dispatch.
* `algorithm.clip_line` / `dda` / `raycast` — in-place mutation and
  lambda-driven nested results.

Everything else — all the vec2/rect/mat3 maths, the colour factories, every
brush, every shape factory (incl. `custom`), and the bulk of `image` (incl.
`blit`'s four overloads and the `XY`/`XYWH` forms) — is generated.

## Metrics (optional)

Built with `-DPV_METRICS=ON`, each binding is wrapped in a tiny call-count + timing
probe and a `picovector.metrics` submodule is exposed:

```python
from picovector import metrics
metrics.reset()
... draw ...
metrics.report()    # per-binding calls, total_ms, % of time, us/call
```

The per-binding indices and human-readable names are emitted by the generator
(`generated/pv_metrics_table.h` + `pv_metrics_names.cpp`); the table, scope guard
and module live in `runtime/pv_metrics.{hpp,cpp}`. With the flag off it compiles
away to nothing and `metrics.enabled` is `False`.

## Wiring into the firmware

[`picovector-micropython.cmake`](picovector-micropython.cmake) compiles the
generated + native + support sources, puts this directory (for
`picovector.config.hpp`), `generated/` and `runtime/` on the include path, links
the PNG/JPEG decoders, and configures the core library for the badge:
`PV_DUAL_CORE=1` (rasterise on core1), `PV_WORKING_BUFFER_SIZE` enlarged to also
fit PNG/JPEG decode state, and the optional `PV_METRICS` / `PV_PROFILE` toggles.

It is included from `board/usermodules.cmake` immediately after
`find_package(PICOVECTOR)` — the core library creates the `usermod_picovector`
target, and this file augments it. The bindings register the `picovector` module
and its types directly.
