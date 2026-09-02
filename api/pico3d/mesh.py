"""mesh — indexed triangle geometry, borrowed from Python buffers."""

from __future__ import annotations

from pv import api, cpp, Buffer


@api(field="mesh", module="pico3d",
     print=("mesh(%d verts, %d tris)", "(int)self->mesh.vertex_count",
            "(int)self->mesh.triangle_count"))
class mesh:
    """Indexed triangle geometry.

    A mesh copies nothing: it points into the buffers it was given and holds
    them alive, so the same arrays can be filled by an .obj loader in Python and
    handed straight to the rasteriser. Which means the arrays stay live and
    writable - animating a mesh is a write into ``positions``, with no rebuild.

    Every array is flat and interleaved by vertex::

        positions = array('f', [x, y, z, x, y, z, ...])   # 3 per vertex
        indices   = array('H', [a, b, c, ...])            # 3 per triangle

    Triangles wind counter-clockwise when seen from the front; a back-facing one
    is culled unless its material is ``double_sided``.
    """

    @cpp(emit="native")
    def __init__(self, positions: Buffer, indices: Buffer, normals: Buffer = None,
                 uvs: Buffer = None, colors: Buffer = None, tangents: Buffer = None):
        ("Build a mesh over existing buffers. positions is an array('f') of 3 "
         "floats a vertex and indices an array('H') of 3 vertex indices a "
         "triangle; both are required. The rest are optional and add what a "
         "material can then use: normals (3 floats a vertex) for lit or matcap "
         "shading, uvs (2 floats) for a texture, colors (one array('I') packed "
         "RGB word a vertex) for per-vertex colour, and tangents (3 floats) for "
         "a normal map.")

    @property
    @cpp(get="self->mesh.vertex_count")
    def vertices(self) -> int: "Vertex count, from the length of positions (read-only)."

    @property
    @cpp(get="self->mesh.triangle_count")
    def triangles(self) -> int: "Triangle count, from the length of indices (read-only)."

    @property
    @cpp(get_raw="self->positions_ref")
    def positions(self) -> None:
        ("The position array this mesh was built over (read-only reference; the "
         "array itself stays writable). Write into it to deform the mesh.")

    @property
    @cpp(get_raw="self->indices_ref")
    def indices(self) -> None: "The index array this mesh was built over (read-only)."

    @property
    @cpp(get_raw="self->normals_ref")
    def normals(self) -> None: "The normal array, or None (read-only)."

    @property
    @cpp(get_raw="self->uvs_ref")
    def uvs(self) -> None: "The texture-coordinate array, or None (read-only)."

    @property
    @cpp(get_raw="self->colors_ref")
    def colors(self) -> None: "The per-vertex colour array, or None (read-only)."

    @property
    @cpp(get_raw="self->tangents_ref")
    def tangents(self) -> None: "The tangent array, or None (read-only)."
