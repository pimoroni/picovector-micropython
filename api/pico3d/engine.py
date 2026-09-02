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
