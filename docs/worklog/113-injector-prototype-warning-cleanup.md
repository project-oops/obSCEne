# 2026-09-01 (injector prototype & warning cleanup) - added injector_start prototype and cleaned unused consts


- `src/injector/injector.h`: Created header declaring `int injector_start(payload_args_t *args);` resolving `-Wmissing-prototypes` error.
- `src/injector/injector.c`: Included `"injector/injector.h"`.
- `src/injector/krw.c`: Removed unused `KERNEL_OFFSET_UCRED_CR_UID` and `KERNEL_OFFSET_UCRED_CR_PRISON` resolving `-Wunused-const-variable` error.

Verified: All C files in `src/injector/` and `src/common/` compile cleanly under `-Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla`.



