# picovector-micropython.cmake — the MicroPython side of PicoVector.
#
# Wires the generated bindings + PNG/JPEG decoders into the firmware and
# configures the core picovector library for RP2 MicroPython rasterise on core1
# (PV_DUAL_CORE), enlarge the shared working buffer to fit PNGDEC/JPEGDEC decode
# state, and expose optional metrics/profiling.

if(NOT DEFINED ${PICOVECTOR_DIR})
  set(PICOVECTOR_DIR "${CMAKE_CURRENT_LIST_DIR}/picovector")
endif()

# PicoVector core library
find_package(PICOVECTOR CONFIG REQUIRED)

# PNG/JPEG decoders are bundled in the core picovector checkout, but only the
# bindings use them, so they're wired here rather than in the core library.
set(PNGDEC_DIR "${PICOVECTOR_DIR}/lib/pngdec")
set(JPEGDEC_DIR "${PICOVECTOR_DIR}/lib/jpegdec")
find_package(PNGDEC CONFIG REQUIRED)
find_package(JPEGDEC CONFIG REQUIRED)

set(PV_MP_SOURCES
  # generated module table + per-type bindings
  ${CMAKE_CURRENT_LIST_DIR}/generated/picovector_bindings.c
  ${CMAKE_CURRENT_LIST_DIR}/generated/vec2.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/rect.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/mat3.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/color.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/algorithm.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/tween.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/pv_metrics_names.cpp
  # shared glue + hand-written (native) bodies + companion image decoders
  ${CMAKE_CURRENT_LIST_DIR}/runtime/pv_support.cpp
  ${CMAKE_CURRENT_LIST_DIR}/runtime/pv_metrics.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/font_native.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/pixel_font_native.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/image_native.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/tween_native.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/algorithm_native.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/image_png.cpp
  ${CMAKE_CURRENT_LIST_DIR}/native/image_jpeg.cpp
)

target_sources(usermod_picovector INTERFACE
  ${PV_MP_SOURCES}
)

target_include_directories(usermod_picovector INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}            # picovector.config.hpp (core selects it via __has_include)
  ${CMAKE_CURRENT_LIST_DIR}/generated  # types.h, pv_metrics_table.h
  ${CMAKE_CURRENT_LIST_DIR}/runtime    # pv_bindings.hpp, pv_objs.hpp, mp_allocator.hpp, pv_metrics.hpp
)

target_link_libraries(usermod_picovector INTERFACE pngdec jpegdec)

# The RP2 hardware interpolator acceleration is pico-sdk only. It's on by
# default so Pico builds are unchanged; non-pico targets (e.g. the WebAssembly
# simulator) build the portable C++ rasteriser and set -DPV_HARDWARE_INTERP=OFF.
option(PV_HARDWARE_INTERP "Link the RP2 hardware interpolator (pico-sdk only)" ON)
if(PV_HARDWARE_INTERP)
  target_link_libraries(usermod_picovector INTERFACE hardware_interp)
endif()

# Badge configuration of the core library:
#  - enlarge the shared working buffer to also fit PNGDEC/JPEGDEC decode state
#    ((60 + 20) * 1024 = 81920); the core rasteriser-only default is 60 KB.
target_compile_definitions(usermod_picovector INTERFACE
  PV_WORKING_BUFFER_SIZE=81920
)

# NOTE: PV_GC_MANAGED is deliberately NOT set here. It is not an optional feature
# but a property of the allocator, so it is defined (=1) in picovector.config.hpp
# right next to the PV_MALLOC/PV_FREE = m_malloc/m_free wiring it depends on. That
# keeps the two in lockstep: picovector must not explicitly free or run finalisers
# that free on a tracing-GC allocator (it double-frees during gc_sweep). Defining
# it here instead would let the allocator and this flag drift apart.

# Rasterise on core1. Off by default (the core library is single-core); enable
# per-board with set(PV_DUAL_CORE ON) before find_package(PICOVECTOR_MICROPYTHON
# ...), or cmake -DPV_DUAL_CORE=ON.
if(PV_DUAL_CORE)
  target_compile_definitions(usermod_picovector INTERFACE PV_DUAL_CORE=1)
endif()

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
