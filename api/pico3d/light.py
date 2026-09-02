"""light — one directional or point light, plus an ambient term."""

from __future__ import annotations

from pv import api, cpp


@api(field="light", module="pico3d")
class light:
    """One light. Directional by default; give it a ``position`` and it becomes a
    point light with distance falloff.

    A scene gets one light, because that is what the vertex stage resolves in a
    single pass. Fill in the rest with ``ambient``, which is added everywhere
    regardless of facing - a black ambient leaves everything facing away from
    the light completely dark.
    """

    @cpp(emit="native")
    def __init__(self, direction=None, color=None, ambient=None,
                 position=None, atten: float = 1.0):
        ("light(direction=vec3(...), color=..., ambient=...) and every setting "
         "by keyword. direction defaults to vec3(0, 0, -1) (away from the "
         "camera), color to white and ambient to black. Passing position makes "
         "it a point light instead, at that world position, and atten then sets "
         "the falloff.")

    @property
    @cpp(get="self->light.direction", set="self->light.direction = {0}")
    def direction(self) -> vec3:
        ("The direction the light travels, in world space. Ignored by a point "
         "light. Need not be normalised.")

    @property
    @cpp(get_raw="pv::box_pico3d_rgb(self->light.color)",
         set="self->light.color = pv::pico3d_rgb_of({0})")
    def color(self) -> None:
        "Diffuse colour, as a picovector color, scaled by how much a face faces the light."

    @property
    @cpp(get_raw="pv::box_pico3d_rgb(self->light.ambient)",
         set="self->light.ambient = pv::pico3d_rgb_of({0})")
    def ambient(self) -> None:
        ("Ambient colour, as a picovector color, added everywhere whatever the "
         "facing. This is the floor on how dark an unlit face gets.")

    @property
    @cpp(get="self->light.position", set="self->light.position = {0}")
    def position(self) -> vec3:
        ("World position of a point light. Only read when point is set, which "
         "passing position to the constructor is what does.")

    @property
    @cpp(get="(bool)self->light.point", set="self->light.point = {0} ? 1 : 0")
    def point(self) -> bool:
        ("True for a point light, False for a directional one. Set by passing "
         "position to the constructor; set it back to False to go directional "
         "again without rebuilding the light.")

    @property
    @cpp(get="self->light.atten", set="self->light.atten = {0}")
    def atten(self) -> float:
        ("Point-light falloff: brightness scales by 1 / (1 + atten * distance^2). "
         "0 is no falloff at all, so the light reaches everything; larger values "
         "pull the lit region in closer.")
