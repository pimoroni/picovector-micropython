"""tween — interpolate between two values, shaped by an easing curve.

A tween maps progress to a value between two endpoints. It holds no clock: the
caller owns the progress and reads a value with ``at()``. Endpoints may be a
float, a vec2, a rect or a mat3; ``at()`` returns the matching type. A mat3 is
interpolated through a decomposed transform (translate/rotate/scale) so rotation
and scale blend correctly. The type-dispatching constructor, ``at()`` and the
endpoint accessors can't be expressed by the DSL convention, so their bodies live
in ``native/tween_native.cpp``.
"""

from __future__ import annotations

from pv import api, cpp, native, const


@api("tween_t", includes=("tween/tween.hpp",))
class tween:
    """Interpolate between two values over a normalised 0..1 progress, or over a
    duration in seconds, shaped by an easing curve."""

    # ── easing curves (Penner set) ──────────────────────────────────────────
    # in = accelerate from rest, out = decelerate to rest, inout = both.
    LINEAR = const("(uint8_t)easing_t::linear", "Constant rate.")

    QUAD_IN = const("(uint8_t)easing_t::quad_in", "Quadratic ease-in.")
    QUAD_OUT = const("(uint8_t)easing_t::quad_out", "Quadratic ease-out.")
    QUAD_INOUT = const("(uint8_t)easing_t::quad_inout", "Quadratic ease-in-out.")

    CUBIC_IN = const("(uint8_t)easing_t::cubic_in", "Cubic ease-in.")
    CUBIC_OUT = const("(uint8_t)easing_t::cubic_out", "Cubic ease-out.")
    CUBIC_INOUT = const("(uint8_t)easing_t::cubic_inout", "Cubic ease-in-out.")

    QUART_IN = const("(uint8_t)easing_t::quart_in", "Quartic ease-in.")
    QUART_OUT = const("(uint8_t)easing_t::quart_out", "Quartic ease-out.")
    QUART_INOUT = const("(uint8_t)easing_t::quart_inout", "Quartic ease-in-out.")

    QUINT_IN = const("(uint8_t)easing_t::quint_in", "Quintic ease-in.")
    QUINT_OUT = const("(uint8_t)easing_t::quint_out", "Quintic ease-out.")
    QUINT_INOUT = const("(uint8_t)easing_t::quint_inout", "Quintic ease-in-out.")

    SINE_IN = const("(uint8_t)easing_t::sine_in", "Sine ease-in.")
    SINE_OUT = const("(uint8_t)easing_t::sine_out", "Sine ease-out.")
    SINE_INOUT = const("(uint8_t)easing_t::sine_inout", "Sine ease-in-out.")

    EXPO_IN = const("(uint8_t)easing_t::expo_in", "Exponential ease-in.")
    EXPO_OUT = const("(uint8_t)easing_t::expo_out", "Exponential ease-out.")
    EXPO_INOUT = const("(uint8_t)easing_t::expo_inout", "Exponential ease-in-out.")

    CIRC_IN = const("(uint8_t)easing_t::circ_in", "Circular ease-in.")
    CIRC_OUT = const("(uint8_t)easing_t::circ_out", "Circular ease-out.")
    CIRC_INOUT = const("(uint8_t)easing_t::circ_inout", "Circular ease-in-out.")

    BACK_IN = const("(uint8_t)easing_t::back_in", "Back ease-in (overshoots the start).")
    BACK_OUT = const("(uint8_t)easing_t::back_out", "Back ease-out (overshoots the end).")
    BACK_INOUT = const("(uint8_t)easing_t::back_inout", "Back ease-in-out (overshoots both ends).")

    ELASTIC_IN = const("(uint8_t)easing_t::elastic_in", "Elastic ease-in (overshoots).")
    ELASTIC_OUT = const("(uint8_t)easing_t::elastic_out", "Elastic ease-out (overshoots).")
    ELASTIC_INOUT = const("(uint8_t)easing_t::elastic_inout", "Elastic ease-in-out (overshoots).")

    BOUNCE_IN = const("(uint8_t)easing_t::bounce_in", "Bounce ease-in.")
    BOUNCE_OUT = const("(uint8_t)easing_t::bounce_out", "Bounce ease-out.")
    BOUNCE_INOUT = const("(uint8_t)easing_t::bounce_inout", "Bounce ease-in-out.")

    # ── construction / evaluation (type-dispatched → native) ────────────────
    @cpp(emit="native")
    def __init__(self, start, end, duration: float = 1.0, easing: int = 0):
        "tween(start, end) interpolates over a normalised 0..1 progress; pass "
        "duration (seconds) to drive it with elapsed time instead. start/end set "
        "the value type: float, vec2, rect or mat3 (a mat3 blends as a decomposed "
        "translate/rotate/scale, discarding shear). easing is one of the tween.* "
        "curves (default LINEAR)."

    @native
    def at(self, t: float):
        "Value at progress t: a normalised 0..1 fraction, or elapsed seconds if a "
        "duration was set. Progress clamps to the endpoints; returns the endpoint "
        "type (float, vec2, rect or mat3)."

    # ── properties (read-only, type-dispatched → native) ────────────────────
    @property
    @cpp(get_raw="tween_box_start(self)")
    def start(self) -> None: "Start endpoint (read-only)."

    @property
    @cpp(get_raw="tween_box_end(self)")
    def end(self) -> None: "End endpoint (read-only)."

    @property
    @cpp(get_raw="tween_box_duration(self)")
    def duration(self) -> None: "Duration in seconds (read-only; 1.0 = normalised 0..1)."
