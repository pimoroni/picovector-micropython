"""engine — the settings and counters that belong to the rasteriser itself."""

from __future__ import annotations

from pv import api, cpp, native


@api(field="", module="pico3d")
class engine:
    """Engine-wide settings, which is what makes them a namespace rather than
    something a surface carries."""

    @staticmethod
    @cpp(emit="expr", args=["(pico3d_set_cores(n), pico3d_get_cores())"])
    def cores(n: int) -> int:
        ("Rasterise on one core or two, returning the count actually in effect - "
         "always 1 on a build without core1. Two cores split the screen into "
         "bands and bin each triangle into the bands it touches, so the win is "
         "on scenes that are fill-bound rather than triangle-bound. It borrows "
         "the same core1 the picovector rasteriser uses, so the two never "
         "overlap.")

    @staticmethod
    @cpp(emit="expr", args="pico3d_get_cores()")
    def core_count() -> int:
        "How many cores the rasteriser is currently using."

    @staticmethod
    @cpp(emit="expr", args=["(pico3d_set_work_bands(n), pico3d_get_work_bands())"])
    def work_bands(n: int) -> int:
        ("The fewest bands a two-core draw of a banded surface cuts its rows "
         "into, returning the value in effect. Each core takes the next band "
         "until none are left, so more bands balance the cores better, at the "
         "cost of scanning the scene's triangles once more per band. Default 4.")

    @staticmethod
    @native
    def profile() -> tuple:
        ("Cycle counts accumulated since the last call, and reset by it: "
         "(transform, build, project, planes, edges, fill, bbox_pixels, "
         "pixels). transform is the per-vertex pass, build the per-triangle "
         "assembly, project/planes/edges the three parts of per-triangle setup, "
         "and fill the scanline rasterise. bbox_pixels counts every pixel the "
         "fill stepped over and pixels only those it wrote, so the ratio is how "
         "much of the bounding boxes the triangles actually covered. All zero "
         "on a build without the cycle counter.")

    @staticmethod
    @native
    def profile_detail() -> dict:
        ("The full per-core breakdown behind profile(), accumulated since the last "
         "call to either and reset by this one: a dict of name -> (core0, core1). "
         "Cycles: transform_wall (core0's wall time for the vertex pass), xform "
         "(the transform itself), extents (add's cull and row-extent pass), "
         "add_wall, draw_wall, clear (depth), bin, pass2 (triangle assembly and "
         "raster), raster (the part inside the rasteriser), project/planes/edges/"
         "fill (within raster), wait (core0 idle waiting for core1). Counts: "
         "verts, tris_in, tris_drawn, tris_clipped, bbox_px, px.")
