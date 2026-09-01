# D232 - The run-time census needs a control, because "no" has two meanings


`obs_module_open` failing means either the library is not there, or **this loader does not
resolve modules by name at all** and no library ever would be. The first is a finding; the
second is a fact about the loader that says nothing about any library.

Built without that distinction and run under PS5PCEM, the census reported:

```text
OBS|res|900-surface/kernel|fail||this library could not be loaded
```

from a process that was executing calls into `libkernel` at the time. Every library failed. The
generation probe, on the same cause, reported "neither generation's graphics driver resolves"
about an emulator that implements the current generation's graphics.

**A report that parses, counts, and is wrong throughout** - which is the outcome this project
holds to be worse than no measurement, and it was produced by a change made *to* stop reporting
absences that are true of the build rather than of the platform. The same mistake, one layer
along, inside its own fix.

`obs_module_resolution_works` is the control: `libkernel` must resolve, because this program is
making calls into it while asking. If that fails the mechanism is missing, not the library, and
every census check skips with that reason. The generation probe answers `-1` for "could not
look" and reports it as its own outcome rather than as an absence.

This is the argument `check_control` already makes one level down - an absence has to mean one
thing before a count of absences means anything - and it had to be made again the moment the
census changed how it decides. A control belongs with the *method*, not with the section.

### It was caught by an emulator, which is the point of having one

The console had been down for hours and this would otherwise have been the next thing sent to
it. `bin/obscene check` passes with the bug present: the host build links the census, so the
run-time path it introduced is not exercised there at all. The gate could not have found it.

