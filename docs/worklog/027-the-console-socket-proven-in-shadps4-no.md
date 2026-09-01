# The console socket, proven in shadPS4 - no Steam Deck needed


User's insight: shadPS4 implements libSceNet over real host sockets, so obSCEne can serve the
command protocol *inside the emulator* and a client connects from the host. That turns the
networking from "needs a Deck" into "needs the emulator we already run in".

Done and demonstrated:
- `net_target.c` implemented against `sceNet*` (D107), signatures confirmed from OpenOrbis +
  shadPS4 both, moved out of the census into real platform.h declarations.
- `SERVE=1` build flag; `obscene_start` runs the suite, announces `net|listening|<port>`,
  then serves in a reconnect loop.
- Built `GEN=4 SERVE=1` (with the sweep exclusions - the serve loop is after the suite, so a
  known mid-suite crash must be excluded or it never listens), run in shadPS4, host port 9803
  opened, connected from Windows, drove hello/report/bye. Transport reported `scenet`.
- `verify: ok` throughout.

### Surprises
- The serving module has to carry the exclusion list. The first two runs died at
  `040-file/open-rejects-null` before reaching the listen, exactly where the sweep says.
- `sceNetSocket` takes a name string as its first argument - confirmed by both sources, and
  the sort of arity detail that would have been a stack-corrupting guess under D008.

