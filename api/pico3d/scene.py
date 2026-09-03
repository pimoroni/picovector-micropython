"""scene — geometry gathered for one frame, so it can be drawn in bands."""

from __future__ import annotations

from pv import api, cpp, native


@api(field="", module="pico3d",
     print=("scene(%d meshes, %d verts, %d tris)", "(int)self->sc.sub_count",
            "(int)self->sc.vert_count", "(int)self->sc.tri_count"))
class scene:
    """One frame's worth of geometry, held so the surface can draw it in bands.

    This exists for one reason: the depth buffer. It is the most expensive
    memory a render touches - one read and one write for every pixel covered -
    so it wants to be in the fastest memory the board has. That memory is
    usually nowhere near big enough for a full screen. The way out is to
    depth-test one horizontal band of rows at a time, reusing a small buffer
    down the screen, and that only works if the whole scene is known before the
    first band is rasterised. Hence: gather, then draw::

        view = pico3d.surface(screen, bands=3)
        scene = pico3d.scene(view, meshes=128, vertices=2048, triangles=1200)

        while True:
            scene.reset()
            for thing in world:
                scene.add(thing.mesh, thing.model, view_proj, thing.material)
            view.draw(scene)

    ``add`` does the transform, so it costs what a ``render`` used to; ``draw``
    does the rasterising. Between them the vertices sit in the scene's own
    arena, which is read once per band, in order - a very different demand on
    memory from a depth buffer, and cheap even in slow memory.

    Nothing here allocates after construction. The arrays are sized once, and
    ``add`` returns False when one is full rather than growing it, so a frame
    can never stall on a heap.
    """

    @cpp(emit="native")
    def __init__(self, surface, meshes: int = 64, vertices: int = 1024,
                 triangles: int = 512):
        ("Size the scene's arenas, once, for the surface it will be drawn into - "
         "add() projects to screen coordinates, so it has to know the viewport. "
         "meshes is how many add() calls a frame may make, vertices and "
         "triangles the totals across them. Storage is roughly vertices * 72 + "
         "triangles * 4 bytes, so it is worth sizing to the worst frame rather "
         "than generously.")

    @native
    def reset(self) -> None:
        ("Empty the scene, keeping its storage. Call it once at the top of every "
         "frame; without it the scene fills up and add() starts refusing.")

    @cpp(native=True, kw=True)
    def add(self, mesh, model: mat4, view_proj: mat4, material,
            light=None, view: mat4 = None) -> bool:
        ("Transform, light and project a mesh into the scene. The arguments are "
         "exactly surface.render's, and the cost is the same - this is that "
         "call's first half, deferred.\n\n"
         "Returns False if the scene is full (or the mesh has more triangles "
         "than one submission's scratch allows), having added nothing: a frame "
         "that overruns loses geometry rather than raising mid-render. The mesh, "
         "material and light are held alive until the next reset().")

    @property
    @cpp(get="self->sc.sub_count")
    def meshes(self) -> int: "Meshes added since the last reset (read-only)."

    @property
    @cpp(get="self->sc.vert_count")
    def vertices(self) -> int: "Vertices transformed since the last reset (read-only)."

    @property
    @cpp(get="self->sc.tri_count")
    def triangles(self) -> int: "Triangles gathered since the last reset (read-only)."
