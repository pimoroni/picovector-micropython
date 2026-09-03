"""surface — a render target: a picovector image, plus the depth buffer."""

from __future__ import annotations

from pv import api, cpp, native


@api(field="", module="pico3d",
     print=("surface(%d x %d)", "self->w", "self->h"))
class surface:
    """What the engine draws into: a picovector ``image``, plus the 16-bit depth
    buffer and per-vertex scratch that go with it.

    The image supplies the pixels, so 3D and 2D share one framebuffer - render a
    scene, then draw over it with the ordinary picovector API. The surface owns
    the depth buffer, sized to the image and reused every frame, which is why
    it is worth building once and keeping (or pass ``depth=`` a buffer of your
    own to say where it lives)::

        screen = picovector.image(320, 240)
        view = pico3d.surface(screen)

        while True:
            screen.clear()
            view.clear_depth()
            view.render(cube, model, view_proj, mat, sun)

    The image's clip rect bounds the render, so ``screen.clip = ...`` frames a
    3D viewport the same way it frames anything else.
    """

    @cpp(emit="native")
    def __init__(self, image, depth=None, bands: int = 0):
        ("Wrap an RGBA image as a render target, allocating a depth buffer to "
         "match. The image is held alive by the surface; a palettised one is "
         "refused, since the engine writes pixels rather than indices.\n\n"
         "depth=False builds a surface with NO depth buffer: nothing is "
         "allocated (150 KB of heap at 320x240), every render takes the "
         "rasteriser's depth-free path, and clear_depth becomes a no-op. Draw "
         "order is then yours to get right - back to front, painter's style - "
         "which for a scene of separate convex pieces is a sort, and which also "
         "makes alpha blending come out correct for free.\n\n"
         "depth borrows a buffer of your own for the depth store instead of "
         "allocating one: any writable, 2-byte-aligned buffer of at least "
         "width * height * 2 bytes. It is the fastest memory a board has to "
         "spare that is wanted here - the depth buffer is read and written once "
         "a pixel for every pixel covered, so on a board whose heap is external "
         "it dominates the frame, and handing over a slab of on-chip RAM is "
         "worth more than any tuning inside the rasteriser. The buffer is held "
         "alive by the surface, and a surface whose image later grows past it "
         "raises rather than overrunning it.\n\n"
         "bands is the other way to place the depth buffer, and the one that "
         "gets it into fast memory at any resolution: instead of a buffer for "
         "the whole image, the surface takes a strip of picovector's shared "
         "working buffer - which is on-chip - tall enough for one band, and "
         "draw() runs the geometry past it band by band. bands=3 on a 320x240 "
         "image means an 80-row strip, 51 KB instead of 150 KB, and on a board "
         "whose heap is external that is the difference between depth costing a "
         "fifth of the frame and costing almost nothing. It needs the geometry "
         "up front, so it works with draw(scene), not render().")

    @property
    @cpp(get_raw="MP_OBJ_FROM_PTR(self->source)")
    def image(self) -> None:
        "The image being drawn into (read-only). Holding the surface keeps it alive."

    @property
    @cpp(get="self->w")
    def width(self) -> int: "Width in pixels (read-only)."

    @property
    @cpp(get="self->h")
    def height(self) -> int: "Height in pixels (read-only)."

    @native
    def clear_depth(self, value: int = 65535) -> None:
        ("Reset the depth buffer to value (0 is the near plane, 65535 the far "
         "one, which is the default). Call it once a frame before the first "
         "render, or everything is depth-tested against last frame.")

    @cpp(native=True, kw=True)
    def draw(self, scene, clear: int = 65535) -> int:
        ("Rasterise a whole scene, in bands if the surface was built with them. "
         "Returns the number of triangles filled - note that when both cores are "
         "in use they split each band by row, so a triangle covering both halves "
         "is filled by both and counted twice. It is fill work, not distinct "
         "triangles.\n\n"
         "The depth buffer is cleared per band as it goes, so there is no "
         "clear_depth to call - and with bands there could not be, since the "
         "buffer only ever holds one band at a time. clear sets what it is "
         "cleared to (65535 = the far plane).")

    @cpp(native=True, kw=True)
    def render(self, mesh, model: mat4, view_proj: mat4, material,
               light=None, depth: bool = True, view: mat4 = None) -> int:
        ("Transform, light and rasterise a mesh. model places it in the world "
         "and view_proj is the camera (projection times view). Returns the "
         "number of triangles actually drawn, the rest having been culled.\n\n"
         "light may be omitted, which renders as if the material were UNLIT. "
         "Pass depth=False to skip the depth buffer entirely for this call: no "
         "per-pixel depth read or write, which is a real saving on a convex, "
         "back-face-culled mesh that cannot occlude itself. view is the view "
         "matrix on its own, and only matcap and specular materials need it - "
         "they resolve against the camera, so without it a matcap is fixed in "
         "world space and a highlight will not appear.")
