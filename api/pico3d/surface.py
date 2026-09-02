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
    it is worth building once and keeping::

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
    def __init__(self, image):
        ("Wrap an RGBA image as a render target, allocating a depth buffer to "
         "match. The image is held alive by the surface; a palettised one is "
         "refused, since the engine writes pixels rather than indices.")

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
