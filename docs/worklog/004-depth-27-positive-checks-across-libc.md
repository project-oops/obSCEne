# 2026-08-19 - Depth: 27 positive checks across libc and maths


**Done.** Added behavioural depth where signatures are certain, rather than more
census breadth.

- `035-libc` extended from 10 to **20** checks: `strcpy`/`strcat`, `strncpy` padding,
  `strstr`, `memchr`/`strrchr`, `strspn`/`strcspn`, `strtok`, `bsearch`, ctype,
  seeded `rand`, `strtoul` base detection.
- New `037-math`: **7** exact-value checks. No tolerances anywhere - every value is
  exactly representable, because an epsilon is where a wrong answer hides.
- 24 names promoted out of the census into real declarations.
- `docs/BACKLOG.md` gained item 4b (setjmp/longjmp, deliberately absent).

**Verified.** 14 sections, 77 checks. `035-libc` **20/20** and `037-math` **7/7**
against real glibc. 400 imports, report well-formed.

**Surprises.**

- **A weak undefined reference does not make the linker search libraries.** Adding
  `-lm` changed nothing: no strong reference to libm existed, so `--as-needed` (the
  default) dropped the dependency and every maths symbol stayed null. `--no-as-needed`
  forces the `DT_NEEDED` entry, after which the weak references bind at load. The
  distinction is visible in the symbol table - resolved imports read
  `FUNC WEAK UND strlen@GLIBC_2.2.5`, unresolved ones read a bare `NOTYPE WEAK UND`
  with no version and no type.

- **The first failure to fix was mine, and `make -n` found it in one line.** The
  recipe never referenced `$(HOST_LIBS)`; an earlier edit had defined the variable and
  not wired it in, so two rounds of linker theorising were spent on a build that was
  never passing the flag at all. Printing the command should have been the first move,
  not the fourth.

- **Seven skipped checks were the harness working, not failing.** Before libm was
  linked, `037-math` skipped every check with "the symbol is not present on this
  platform" rather than jumping to null and taking the process down. That is exactly
  what the weak-symbol presence check was added for, demonstrated by accident.

**Not done.** Everything blocked stays blocked: struct-taking functions, NID
verification, and running under an emulator at all. The positive/negative ratio has
moved a long way but only inside two sections - the vendor subsystems are still
checked almost entirely from the failure side.

