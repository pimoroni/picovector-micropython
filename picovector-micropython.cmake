# picovector-micropython.cmake — the MicroPython side of PicoVector.
#
# Wires the generated bindings + PNG/JPEG decoders into the firmware and
# configures the core picovector library for the badge: rasterise on core1
# (PV_DUAL_CORE), enlarge the shared working buffer to fit PNGDEC/JPEGDEC decode
# state, and expose optional metrics/profiling.
#
# Include this AFTER find_package(PICOVECTOR) so the usermod_picovector target
# already exists (see board/usermodules.cmake).

set(_PV_MP_DIR ${CMAKE_CURRENT_LIST_DIR})
# The core PicoVector library is vendored as a submodule (no-bindings branch).
set(_PV_CORE_DIR ${CMAKE_CURRENT_LIST_DIR}/picovector)

# PNG/JPEG decoders are bundled in the core picovector checkout, but only the
# bindings use them, so they're wired here rather than in the core library.
set(PNGDEC_DIR "${_PV_CORE_DIR}/lib/pngdec")
set(JPEGDEC_DIR "${_PV_CORE_DIR}/lib/jpegdec")
find_package(PNGDEC CONFIG REQUIRED)
find_package(JPEGDEC CONFIG REQUIRED)

set(PV_MP_SOURCES
  # generated module table + per-type bindings
  ${_PV_MP_DIR}/generated/picovector_bindings.c
  ${_PV_MP_DIR}/generated/vec2.cpp
  ${_PV_MP_DIR}/generated/rect.cpp
  ${_PV_MP_DIR}/generated/mat3.cpp
  ${_PV_MP_DIR}/generated/color.cpp
  ${_PV_MP_DIR}/generated/brush.cpp
  ${_PV_MP_DIR}/generated/shape.cpp
  ${_PV_MP_DIR}/generated/image.cpp
  ${_PV_MP_DIR}/generated/font.cpp
  ${_PV_MP_DIR}/generated/pixel_font.cpp
  ${_PV_MP_DIR}/generated/algorithm.cpp
  ${_PV_MP_DIR}/generated/pv_metrics_names.cpp
  # shared glue + hand-written (native) bodies + companion image decoders
  ${_PV_MP_DIR}/runtime/pv_support.cpp
  ${_PV_MP_DIR}/runtime/pv_metrics.cpp
  ${_PV_MP_DIR}/native/font_native.cpp
  ${_PV_MP_DIR}/native/pixel_font_native.cpp
  ${_PV_MP_DIR}/native/image_native.cpp
  ${_PV_MP_DIR}/native/algorithm_native.cpp
  ${_PV_MP_DIR}/native/image_png.cpp
  ${_PV_MP_DIR}/native/image_jpeg.cpp
)

target_sources(usermod_picovector INTERFACE
  ${PV_MP_SOURCES}
)

target_include_directories(usermod_picovector INTERFACE
  ${_PV_MP_DIR}            # picovector.config.hpp (core selects it via __has_include)
  ${_PV_MP_DIR}/generated  # types.h, pv_metrics_table.h
  ${_PV_MP_DIR}/runtime    # pv_bindings.hpp, pv_objs.hpp, mp_allocator.hpp, pv_metrics.hpp
)

target_link_libraries(usermod_picovector INTERFACE pngdec jpegdec)

# Badge configuration of the core library:
#  - rasterise on core1 (the core default is single-core)
#  - enlarge the shared working buffer to also fit PNGDEC/JPEGDEC decode state
#    ((60 + 20) * 1024 = 81920); the core rasteriser-only default is 60 KB.
target_compile_definitions(usermod_picovector INTERFACE
  PV_DUAL_CORE=1
  PV_WORKING_BUFFER_SIZE=81920
)

# Optional per-binding call-count + timing metrics (the picovector.metrics
# module). Off by default (zero cost):  cmake -DPV_METRICS=ON ...
option(PV_METRICS "Instrument picovector bindings with call/time metrics" OFF)
if(PV_METRICS)
  target_compile_definitions(usermod_picovector INTERFACE PV_METRICS=1)
endif()

# Optional rasteriser phase profiling (prints phase timings to the REPL).
option(PV_PROFILE "Enable picovector rasteriser phase profiling" OFF)
if(PV_PROFILE)
  target_compile_definitions(usermod_picovector INTERFACE PV_PROFILE=1)
endif()

set_source_files_properties(
  ${PV_MP_SOURCES}
  PROPERTIES COMPILE_FLAGS
  "-Wno-unused-variable"
)

if(DEFINED PICO_BOARD)
  # jpegdec needs PICO_BUILD on the Pico target
  target_compile_definitions(jpegdec PRIVATE PICO_BUILD)

  set_source_files_properties(
    ${PV_MP_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    "-O2;-fgcse-after-reload;-floop-interchange;-fpeel-loops;-fpredictive-commoning;-fsplit-paths;-ftree-loop-distribute-patterns;-ftree-loop-distribution;-ftree-vectorize;-ftree-partial-pre;-funswitch-loops"
  )
endif()
