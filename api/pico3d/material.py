"""material — how a mesh's surface is coloured and shaded."""

from __future__ import annotations

from pv import api, cpp, const


@api(field="mat", module="pico3d")
class material:
    """How a surface is coloured: a base colour, an optional texture, and which
    of the shading models resolves the light.

    The scalar settings are all writable, so one material can be retuned per
    frame without rebuilding it. The image settings (``texture``, ``normal_map``,
    ``matcap``) are fixed at construction: each one is a view onto the image's
    pixels, and the material holds that image alive.

    The shading extras stack in one order of precedence, because each one takes
    over the per-pixel normal: a ``normal_map`` wins over a ``matcap``, and
    ``specular`` adds on top of whichever is in play.
    """

    # How a light resolves across a face.
    FLAT = const("PICO3D_FLAT", "One face normal, so one light value per triangle.")
    GOURAUD = const("PICO3D_GOURAUD",
                    "Per-vertex normals, with the light interpolated across the face.")
    UNLIT = const("PICO3D_UNLIT", "No lighting; the colour (times the texture) as-is.")

    # How a texture is sampled.
    NEAREST = const("PICO3D_NEAREST", "Texture filter: nearest texel (fastest).")
    BILINEAR = const("PICO3D_BILINEAR", "Texture filter: bilinear (smooth).")

    @cpp(emit="native")
    def __init__(self, color=None, texture=None, shading: int = None,
                 filter: int = None, double_sided: bool = False,
                 alpha_cutoff: int = 128, normal_map=None, matcap=None,
                 specular=None, shininess: int = 32):
        ("material(color=..., texture=...) and every other setting by keyword. "
         "color defaults to white and tints whatever the texture samples; "
         "shading defaults to FLAT and filter to NEAREST. See each property for "
         "what the rest do.")

    # ── the surface ─────────────────────────────────────────────────────────
    @property
    @cpp(get_raw="pv::box_pico3d_rgb(self->mat.color)",
         set="self->mat.color = pv::pico3d_rgb_of({0})")
    def color(self) -> None:
        ("The base colour, as a picovector color. It multiplies the texture "
         "sample, so white leaves a texture alone and a mesh with per-vertex "
         "colors ignores this entirely. Alpha is not used - the engine writes "
         "opaque pixels.")

    @property
    @cpp(get_raw="self->texture_ref")
    def texture(self) -> None:
        ("The RGBA image sampled through the mesh's uvs, or None (read-only). "
         "Edge-clamped, so uvs outside 0..1 stretch the border rather than "
         "wrapping. Set at construction.")

    @property
    @cpp(get="self->mat.filter", set="self->mat.filter = (pico3d_filter_t){0}")
    def filter(self) -> int:
        ("How the texture is sampled: NEAREST (default) or BILINEAR. BILINEAR "
         "costs four texel reads a pixel.")

    @property
    @cpp(get="self->shading", set="self->shading = (pico3d_shading_t){0}")
    def shading(self) -> int:
        ("Which shading model resolves the light: FLAT (default), GOURAUD or "
         "UNLIT. FLAT and GOURAUD both light per vertex, so this costs nothing "
         "per pixel; the per-pixel paths are the ones below.")

    @property
    @cpp(get="self->mat.double_sided", set="self->mat.double_sided = {0}")
    def double_sided(self) -> bool:
        ("Draw back faces too (default False). Back-face culling halves the "
         "triangles a closed mesh sets up, so leave this off unless the geometry "
         "is genuinely open - a flag, a leaf, a plane seen from both sides.")

    @property
    @cpp(get="self->mat.alpha_cutoff", set="self->mat.alpha_cutoff = (uint8_t){0}")
    def alpha_cutoff(self) -> int:
        ("Discard texels whose alpha is below this, 0 to draw them all (default "
         "128). A cutout, not blending: a pixel is either written or skipped, "
         "which is what makes foliage and cards cheap.")

    # ── per-pixel shading ───────────────────────────────────────────────────
    @property
    @cpp(get_raw="self->normal_map_ref")
    def normal_map(self) -> None:
        ("A tangent-space normal map, or None (read-only). Needs mesh tangents "
         "and a light, and moves lighting to per-pixel using the perturbed "
         "normal. Set at construction.")

    @property
    @cpp(get_raw="self->matcap_ref")
    def matcap(self) -> None:
        ("A matcap / spherical environment map, or None (read-only). Needs mesh "
         "normals, and shades per-pixel by sampling this image with the "
         "interpolated normal - a whole material look baked into one texture, "
         "with no light involved. The sample multiplies color, so white gives a "
         "pure reflection and a coloured base a tinted one. Set at construction.")

    @property
    @cpp(get_raw="pv::box_pico3d_rgb(self->mat.specular)",
         set="self->mat.specular = pv::pico3d_rgb_of({0})")
    def specular(self) -> None:
        ("Blinn-Phong highlight colour, as a picovector color, or black for no "
         "highlight (the default). Needs a light and the view matrix passed to "
         "render(); adds on top of the diffuse, per-pixel.")

    @property
    @cpp(get="self->mat.shininess", set="self->mat.shininess = {0}")
    def shininess(self) -> int:
        ("Blinn-Phong exponent (default 32). Higher is a tighter, sharper "
         "highlight. Only read when specular is set.")
