# 2026-09-01 (freestanding headers) - removed <sys/types.h> from injector headers and defined freestanding types


Fixed freestanding cross-compilation failure under `-nostdlib` / `HARDWARE=1`:
- `src/common/freestd.h`: Defined freestanding `pid_t`, `off_t`, and `ssize_t` types guarded by `_PID_T_DECLARED`, `_OFF_T_DECLARED`, `_SSIZE_T_DECLARED`.
- `src/common/krw.h`, `src/injector/procctl.h`, `src/injector/target.h`, `src/injector/loader.h`: Replaced `#include <sys/types.h>` with `#include "common/freestd.h"`.
- `src/injector/injector.c`: Added `/data/obscene-payload.elf` to fallback search path.

Verified: `make payload injector HARDWARE=1` and `./bin/obscene inject --build-only` compile 100% clean with zero warnings or errors. `bash scripts/verify.sh` green.

