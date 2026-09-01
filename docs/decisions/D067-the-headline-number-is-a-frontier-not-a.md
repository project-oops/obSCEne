# D067 - The headline number is a frontier, not a sum


Status: decided, on external criticism.

"65 pass, 6 partial, 38 fail, 7 skip" has no denominator and no shape. Worse, it discards
the one piece of structure this suite deliberately has: checks depend on capabilities that
other checks establish, so a failure near the foundation matters more than one at the top,
and summing treats them alike.

It also rewards the wrong work - adding three hundred census entries moves the total and
changes nothing about whether a bug would be found.

`OBS|frontier` reports three numbers: capabilities established, **checks that never ran
because a capability they needed was not**, and the deepest wholly-green section. The
middle one is the point: it counts the suite sitting behind the floor, and it is the
number an emulator author can move.

**Measured over the capability graph, not section order.** Section order is a reading
order; `requires_caps`/`provides_caps` is the real dependency structure. The green-section
number is reported as well because it is the one a person can point at, but it is a proxy
and the capability counts are the measurement.

