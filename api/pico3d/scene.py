"""scene — geometry gathered up front, so it can be drawn a band at a time."""

from __future__ import annotations

from pv import api, cpp, native


@api(field="", module="pico3d",
     print=("scene(%u/%u meshes)", "self->scene.sub_count", "self->scene.sub_cap"))
class scene:
    """A frame's geometry, transformed and projected once and then rasterised in
    horizontal bands.

    ``surface.render()`` draws a mesh the moment you hand it over, which means
    depth-testing against a buffer as tall as the whole screen. A scene defers
    instead: ``add()`` transforms and projects each mesh into the scene's own
    arena, and ``draw()`` runs the finished geometry past one band of rows at a
    time, so the depth buffer only ever needs to be one band tall and can sit in
    fast memory::

        view = pico3d.surface(screen, bands=4)
        geometry = pico3d.scene(view, meshes=96, vertices=2048, triangles=3072)

        while True:
            geometry.reset()
            for thing in world:
                geometry.add(thing.mesh, thing.model, view_proj, thing.material, sun)
            view.draw(geometry)

    Nothing here allocates after construction, so a frame can never stall on the
    heap: the three capacities are fixed when the scene is built and ``add()``
    returns False rather than growing.
    """

    @cpp(emit="native")
    def __init__(self, surface, meshes: int = 16, vertices: int = 512,
                 triangles: int = 512):
        ("Size a scene against the surface it will be drawn into. meshes is how "
         "many add() calls one frame may hold, vertices and triangles the totals "
         "across all of them - a mesh costs its own vertex and triangle count "
         "whatever it is drawn with, so add up the geometry you expect on screen "
         "at once and leave some room. The surface is held alive by the scene.")

    @property
    @cpp(get_raw="MP_OBJ_FROM_PTR(self->target)")
    def surface(self) -> None:
        "The surface this scene projects for (read-only)."

    @property
    @cpp(get="self->scene.sub_count")
    def meshes(self) -> int: "Meshes added since the last reset (read-only)."

    @property
    @cpp(get="self->scene.vert_count")
    def vertices(self) -> int: "Vertices held, across every mesh added (read-only)."

    @property
    @cpp(get="self->scene.tri_count")
    def triangles(self) -> int: "Triangles held, across every mesh added (read-only)."

    @property
    @cpp(get="self->scene.sub_cap")
    def mesh_capacity(self) -> int: "How many meshes it was built to hold (read-only)."

    @property
    @cpp(get="self->scene.vert_cap")
    def vertex_capacity(self) -> int: "How many vertices it was built to hold (read-only)."

    @property
    @cpp(get="self->scene.tri_cap")
    def triangle_capacity(self) -> int: "How many triangles it was built to hold (read-only)."

    @native
    def reset(self) -> None:
        ("Empty the scene, ready for the next frame. Call it before the frame's "
         "first add(), not after draw(): the geometry has to stay put until it "
         "has been rasterised.")

    @cpp(native=True, kw=True)
    def add(self, mesh, model: mat4, view_proj: mat4, material,
            light=None, view: mat4 = None) -> bool:
        ("Transform, light and project a mesh into the scene. The arguments are "
         "surface.render()'s, and mean the same things.\n\n"
         "Returns False if the scene is full, having added nothing - check it if "
         "the geometry on screen varies, since the alternative is silently "
         "dropping whatever came last. A mesh outside the frustum is culled "
         "whole, before any of its vertices are transformed, and reports True: "
         "nothing failed, there was simply nothing to add.")
