# obSCEne build.
#
# Two shapes from one source tree:
#
#   make module   the freestanding module a loader can run. No libc, no SDK, every
#                 platform function left undefined for the loader to resolve.
#   make host     a native binary with the platform stubbed, so the harness itself
#                 can be run and checked without a console or an emulator.
#
# `make check` builds both, plus the plain-ELF `payload` shape. `make` on its own is
# all three. The full list is in `.PHONY`, which is the only copy that cannot go stale.
#
# Compiling the C needs only clang. That is deliberate: a conformance probe that needs a vendor
# toolchain to build is a probe most people cannot run.
#
# Producing a *format* - the vendor module, the eboot, the package - goes through **selfish**,
# the sibling repository that owns every console file format (`$(SELFISH)`, default `../selfish`).
# obSCEne carries no format code of its own: it deleted `nid.rs`/`dynlib.rs`/`mkself.rs` and its
# copy of `module.ld`, and `obscene-tool` takes selfish's crates as path dependencies. So:
#
#   * `mkmodule`/`mkself` (module, eboot)     -> selfish-container / selfish-elf
#   * the module linker script                -> $(SELFISH)/link/module.ld
#   * payload import resolution               -> selfish `dynamic_symbols` + `Nid`
#   * `make pkg`                              -> selfish-pfs + `selfish pack`
#
# selfish is not optional and not a convenience: it is the single source of truth for the
# formats, so a wrong magic or a stale tag cannot be introduced here in isolation. A checkout
# without `$(SELFISH)` beside it builds `host` but nothing a console or emulator loads.
# `prosperous` (`../prosperous`) is the other sibling this build reaches into, for the
# console transport `hw` uses.

# `?=` would lose to make's own default of `cc`, so this is assigned outright.
# A command-line `make CC=...` still overrides it — which is how a compiler cache is wrapped in:
# `oops-rebuild-pkg.sh` passes `make CC="sccache clang"`, and a plain `make` stays bare clang.
CC := clang
BUILD ?= build

# Which console generation the module declares itself for.
#
# 5 is what this probe is for and the default. 4 exists because the loaders disagree and
# all of them are right to: shadPS4 is a previous-generation emulator and refuses a
# module marked for the current one, while Kyty and craziiEmu read the marker as the
# current generation. Building GEN=4 tells that loader something true rather than
# disguising the module.
GEN ?= 5

# Which dynamic-table convention the module writes: legacy or current.
#
# `legacy` is what this module has always written and what every loader in the toolkit
# accepts. `current` is what all six retail current-generation dumps use - standard ELF tags
# for the standard tables, vendor tags only for vendor concepts. Under `legacy` a reader that
# follows retail modules finds none of our imports at all. (D193)
#
# It follows the generation, because that is what the evidence says it is.
#
# Measured across the five loaders on 2026-08-26, building both ways:
#
#   shadPS4   gen 4   current -> refuses the standard tags, then faults
#   fpPS4     gen 4   current -> no records
#   Kyty      gen 4   current -> works (lenient)
#   PS5PCEM   gen 5   current -> works
#   orbistoun gen 5   current -> works
#
# shadPS4 says why, plainly: `unsupported dynamic tag 0x02 / 0x03 / 0x07 / 0x61000043 /
# 0x61000047`. Those are the standard ELF tags and the high vendor range - a previous-generation
# emulator does not know them, and is right not to, because previous-generation modules do not
# use them.
#
# So the two conventions are not better and worse, they are older and newer, and a module should
# write the one matching what it claims to be. Six retail current-generation dumps use `current`;
# every loader that accepts a previous-generation module wants `legacy`. Override to build a
# module that disagrees with its own generation, which is only useful for testing exactly this.
TABLE ?= $(if $(filter 5,$(GEN)),current,legacy)

WARNINGS := -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
            -Wstrict-prototypes -Wmissing-prototypes -Wvla
STD := -std=c11
INCLUDE := -Iinclude -Isrc

# Stamped into every report so a diff can tell "the probe changed" from "the platform
# changed" — two very different answers to "did that help?".
# Override in CI with the commit: make BUILD_ID=$$(git rev-parse --short HEAD)
BUILD_ID ?= dev
STAMP := -DOBSCENE_BUILD_ID='"$(BUILD_ID)"'

# Which generation this module was built for, visible to the C as well as to `mkmodule`.
#
# It was only ever passed to the tool, which writes it into `EI_ABIVERSION`. The code could
# not see it, so anywhere the two generations expose different entry points the source had to
# infer which it was from whether a symbol resolved - and that inference is wrong on any
# loader that hands back a non-null address for a function it has not implemented, which is
# most of them.
#
# A build flag is the honest place for this. `EI_ABIVERSION` is read before a single guest
# instruction runs, so the generation cannot be negotiated at run time; it is decided here or
# not at all.
STAMP += -DOBSCENE_GEN=$(GEN)

# Checks to leave unrun, space separated, by identifier.
#
# A call that ends the process takes every check after it with it. The default is empty,
# because a crash is a finding and the first run should find it. This is for the second
# run: once it is recorded, excluding it buys the rest of the suite.
#
#   make module EXCLUDE="040-file/open-rejects-null"
#
# An entry with no `/` names a whole section, for a loader that fails a layer rather than a
# check - fpPS4 returns from none of `035-libc`, and one entry says that better than the
# twenty separate skips a per-check walk produces (D145):
#
#   make module EXCLUDE="035-libc 017-posix/rwlock"
#
# Excluded checks are reported as skips with a reason, so they stay visible and a diff
# still sees them stop being run.
EXCLUDE ?=
STAMP += -DOBSCENE_EXCLUDE='"$(EXCLUDE)"'

# The SDK version this module declares in its process parameter.
#
# Not cosmetic and not a build-identity string: the loader reads it before the module runs and
# it changes what the loader then does. A title that launches on retail hardware declares
# `0x08008011`; this project declared zero.
#
# **Zero is kept, and it is measured rather than preferred.** Both were built from identical
# sources with only this flag differing and both were run on the same console:
#
#   * import binding: **no difference at all.** Ninety-eight symbols were measured in both
#     arms and not one changed between linked and unlinked. The version does not gate it.
#   * `sceKernelDlsym`: 105 of 122 symbols resolvable at zero, **0 of 173 at 0x08008011**.
#     Declaring a real SDK version stops run-time module resolution working.
#
# The census is built on run-time resolution, so the second effect costs ten thousand
# measurements to buy nothing. The flag stays because that is a fact about one console on one
# day and the experiment should be one word to repeat. (D244)
#
#   make pkg PROC_SDK=0            run-time resolution works; the census runs
#   make pkg PROC_SDK=0x08008011   what a launching title declares; dlsym stops answering
PROC_SDK ?= 0
STAMP += -DOBS_PROC_PARAM_SDK=$(PROC_SDK)u

# Whether `061-imports` calls an import whose address reads as null.
#
# Off, and measured off rather than assumed off: asked once on hardware, the call did not
# return. That is the finding - the PLT is unbound along with the data slots - and re-learning
# it costs two runs each time, because D181 retries a check that died before it skips it.
CALL_UNBOUND ?= 0
STAMP += -DOBS_CALL_UNBOUND=$(CALL_UNBOUND)

# Which generation's video register/attribute pair the display uses.
#
# 0 keeps the rule in src/probe/display.c (resolved symbols first, build generation breaks a tie);
# 1 forces the previous generation's pair; 2 forces the current one. The rule was written
# against emulators, where only one pair is ever real. On a console both resolve.
#
#   make pkg DISPLAY_PAIR=2      the current generation's entry points, whatever GEN says
DISPLAY_PAIR ?= 0
STAMP += -DOBS_DISPLAY_PAIR=$(DISPLAY_PAIR)

# Which direct-memory type the framebuffer comes from: 0 write-back onion, 3 write-combined
# garlic, 10 write-back garlic. A GPU scans out of garlic; this asked for onion and never
# varied it, so a refusal could never be attributed.
#
#   make pkg DISPLAY_MEM=3
DISPLAY_MEM ?= 3
STAMP += -DOBS_DISPLAY_MEM=$(DISPLAY_MEM)

# The visual flip probe in `085-videobuf`: fills the two framebuffers with different flat
# colours and alternates, so somebody watching can say whether a flip swaps buffers at all.
#
# Off by default because it replaces the report on screen while it runs. Nothing else can
# answer the question - every call in the display path returns success while the screen shows
# a single-buffered image.
#
#   make pkg DISPLAY_PROBE=1
DISPLAY_PROBE ?= 0
STAMP += -DOBS_DISPLAY_PROBE=$(DISPLAY_PROBE)

# Libraries `061-imports` must not open, derived from the committed record of what a sweep
# proved ends the process.
#
# That section asks the resolver whether the platform has a symbol, and asking means loading
# the library. Ten libraries end the process when loaded (data/hardware/crashers.txt), and the
# section walked into one on its first full run: the report stops mid-walk, one library after
# `libScePad`, and the next library in registry order is `libSceVideoRecording`.
#
# `EXCLUDE` could not cover this. It matches *check ids*, and this is one check opening many
# libraries — which is the same shape as the mistake in D235, this time in the code written to
# explain D235. The library names come out of the record's check ids, whose last underscore-
# separated field is the library.
UNSAFE_LIBRARIES := $(shell sed -e 's/#.*//' -e '/^[[:space:]]*$$/d' data/hardware/crashers.txt 2>/dev/null | sed 's/.*_//' | tr '\n' ' ')
STAMP += -DOBS_UNSAFE_LIBRARIES='"$(UNSAFE_LIBRARIES)"'

# The blind prober: every censused symbol called with nothing. Off by default.
#
# Off because it is the one section written in the expectation that it will not return -
# a function that dereferences a null argument takes the process with it - and the default
# suite has to be something that runs to completion. See scripts/bulk-sweep.sh, which
# drives it and raises BULK_START past whatever ended the previous run.
#
#   make module BULK=1                 # the whole surface, from the start
#   make module BULK=1 BULK_START=291  # resume past the one that died
#   make module BULK=1 BULK_LIMIT=20   # twenty at a time, for bisecting
BULK ?=
BULK_START ?= 0
BULK_LIMIT ?= 0
ifneq ($(BULK),)
STAMP += -DOBS_BULK -DOBS_BULK_START=$(BULK_START)u -DOBS_BULK_LIMIT=$(BULK_LIMIT)u
endif

# HARDWARE=1 — a build intended for a real console, which BULK may never enter.
#
# The blind prober calls every resolvable symbol with six zero arguments. On an emulator a
# bad call kills a process. On a console the list it walks contains
# `sceSystemServiceRequestPowerOff`, `sceLncUtilSystemShutdown`, `sceShellCoreUtilRequestShutdown`
# and ten more of the same kind, and calling one of those with zeros is a plausible way to
# turn somebody's console off mid-run.
#
# A blocklist cannot fix it: of the 67,053 entries the sweep walks, 50,344 are unnamed NIDs.
# Three quarters of the list cannot be screened, because nobody knows what those functions
# are. So the guard is total rather than selective.
#
# Today's protection is that BULK is off by default, which is a convention - and a convention
# is exactly what fails at one in the morning when somebody reuses the last command line they
# had in their shell. This makes it a compile error instead. (D179)
HARDWARE ?=
ifneq ($(HARDWARE),)
ifneq ($(BULK),)
$(error HARDWARE=1 and BULK=1 are mutually exclusive: the blind prober calls unnamed \
symbols with zero arguments, and the list it walks includes system power-off and reboot \
functions. Build without BULK for anything going near a console)
endif
STAMP += -DOBSCENE_HARDWARE
endif

# How many create-and-join cycles 015-sync/thread-churn performs.
#
# Overridable so the count can be bisected. A crash that happens at forty and not at two
# has a threshold somewhere between, and "fails above N live threads" is a bug report
# where "fails sometimes" is a complaint.
#
#   make module CHURN=8
CHURN ?=
ifneq ($(CHURN),)
STAMP += -DOBS_THREAD_CHURN=$(CHURN)
endif

# Serve the command protocol after the report, over the console socket.
#
# Off by default: the ordinary run emits its report and finishes, and a build that always
# blocked waiting for a driver would break every existing sweep. On means the module runs
# the suite, announces a listening port, and waits for the driver - which is how the
# networking is exercised inside an emulator whose net layer reaches the host (shadPS4).
#
#   make module GEN=4 SERVE=1     # build a shadPS4 module that listens after reporting
SERVE ?=
ifneq ($(SERVE),)
STAMP += -DOBS_SERVE_ON_START
endif

# The blob/run/reset escape hatch. Off by default, and pointedly so.
#
# `blob` uploads machine code and `run` executes it - arbitrary code from the socket, which is
# the one thing the read-only posture exists to keep off the wire unless the owner asks for it.
# A default build does not compile the storage or the executable mapping at all and refuses the
# three verbs as `unsupported`; ON announces the `blob` and `reset` capabilities and honours
# them, executing on the host and refusing on the console (no confirmed executable mapping).
#
#   make host HATCH=1            # the blob/run/reset verbs live on the host, for the tests
HATCH ?=
ifneq ($(HATCH),)
STAMP += -DOBS_NET_ESCAPE
endif

# GPU compute probe. Off by default.
#
# Off because it pulls in a GPU backend and, on the host, links Vulkan - neither of which a
# plain report run wants. On means the 160-gpu section dispatches its compute kernels and
# reports what the device computed. The backend is chosen by target: Vulkan on the host and
# the Deck (gpu_vulkan.c, -lvulkan), a refusing stub on the console (gpu_gnm.c).
#
#   make host GPU=1               # dispatch on the host GPU / llvmpipe, prove the pipeline
#   make module GEN=4 GPU=1       # console module; the Gnm backend refuses until confirmed
GPU ?=
ifneq ($(GPU),)
STAMP += -DOBS_GPU
endif

# The mined census: 35,000 symbols from firmware and emulator export tables. On by default.
#
# Off is for loaders the census overwhelms. It emits one record per symbol through whatever
# text channel the platform has, and on a slow one that is the whole run - fpPS4 reached
# check 11 of 499 in fifteen minutes, where it used to finish. The curated census is 375
# symbols and leaves the behavioural sections dominant again.
#
# This is a property of the loader, not of the probe: the same source builds both.
#
#   make module CORPUS=0     # curated census only
CORPUS ?= 1
ifeq ($(CORPUS),0)
STAMP += -DOBS_NO_CORPUS
endif

# The nameless census: identifiers with no recoverable name. Off by default.
#
# Off because it is a weaker claim than the rest of the report. A named symbol reporting
# absent says "this function is missing"; an identifier reporting absent says "something is
# missing and we cannot tell you what". Worth having deliberately, not by default.
#
#   make module NIDS=1
NIDS ?=
ifneq ($(NIDS),)
STAMP += -DOBS_NIDS
endif

# The Rust tooling. Built into a VM-local target directory for the same reason the C
# build is: cargo writes by rename, and a mounted Windows share refuses it.
TOOL_TARGET ?= /tmp/obscene-tool-target
TOOL := $(TOOL_TARGET)/release/obscene-tool

# Source lists, not preprocessor branches.
#
# What differs between targets is *which files are compiled*, and that stays visible here
# rather than being buried in `#if` inside otherwise-shared code. A file either builds for
# a target or it does not, and the list says which - so adding a target means adding a list,
# not threading another condition through every file it touches.
COMMON_SRC := src/common/freestd.c src/probe/runtime.c src/probe/status.c src/probe/report.c src/probe/harness.c src/probe/registry.c \
              src/probe/imports.c src/probe/display.c src/probe/font.c src/probe/screen.c src/probe/sink.c \
              src/probe/net.c src/probe/sysinfo.c \
              $(wildcard src/probe/sections/*.c)
TARGET_SRC := $(COMMON_SRC) src/probe/start.c src/probe/crt.c src/probe/sink_target.c src/probe/net_target.c
HOST_SRC := $(COMMON_SRC) src/probe/host_main.c src/probe/host_stubs.c src/probe/sink_host.c \
            src/probe/net_posix.c
INJECTOR_SRC := src/injector/injector.c src/injector/loader.c src/injector/procctl.c \
                src/injector/krw.c src/injector/target.c src/common/freestd.c src/common/syscall.c

# On this host the maths functions live in libm, not libc, so without it 037-math
# skips every check with "symbol is not present" - the harness behaving correctly, and
# useless for verifying those checks.
#
# --no-as-needed is required, not decorative. A *weak* undefined reference does not
# make the linker search libraries for a definition, so nothing strongly references
# libm and --as-needed (the default) drops the dependency entirely; the symbols then
# stay null at run time and the checks skip anyway. Forcing the DT_NEEDED entry makes
# the dynamic linker load libm, after which the weak references resolve.
#
# The target build links nothing at all - its loader resolves every import.
HOST_LIBS := -Wl,--no-as-needed -lm

# The GPU backend, added only under GPU=1 so a plain build links no Vulkan and the module
# pulls in no vendor GPU code. The section itself (src/probe/sections/gpu.c) is always compiled -
# it reports a skip when OBS_GPU is unset - but the backend it calls is target-specific and
# conditional: Vulkan on the host/Deck, the refusing Gnm stub on the console.
ifneq ($(GPU),)
HOST_SRC += src/probe/gpu_vulkan.c
HOST_LIBS += -lvulkan
TARGET_SRC += src/probe/gpu_gnm.c
endif

# The guest is x86-64 and its kernel is FreeBSD-derived, so that is the target
# triple. -nostdlib keeps it honest: nothing is linked in that a console would not
# already provide, which is what makes the import list in the finished object an
# accurate statement of what this program needs.
TARGET_TRIPLE := x86_64-unknown-freebsd
#
# -fvisibility=hidden is not an optimisation. Built shared and PIC, a call to one of
# this program's own functions goes through the procedure linkage table, which leaves
# a relocation the loader tries to resolve - by NID, against the libraries the module
# imports from. Our own names are not NIDs and are in none of those libraries, so it
# fails:
#
#   Resolve: Not Resolved obs_write
#   lambda: Function not patched! obs_write
#
# and leaves the slot null. The guest calls address zero the first time it tries to
# write its own output. This module exports nothing, so nothing needs to be visible,
# and hiding it turns those calls back into direct ones.
#
# -Bsymbolic covers what is left: anything still global binds to the definition in
# this module rather than being left for the loader to find elsewhere.
#
# -fno-plt was here, and removing it is what a corrected tag value made possible.
#
# The flag routes a call to an imported function through the global offset table
# instead of the procedure linkage table, which leaves `.rela.plt` empty. It was
# added because one emulator ran the module with it and resolved nothing without it,
# while the other refused the module outright *with* it. That looked like two loaders
# wanting incompatible things.
#
# They did not. DT_SCE_JMPREL and DT_SCE_PLTRELSZ were swapped in our tag table, so
# every module that declared a linkage table told both loaders its size was its
# address. With -fno-plt the table is empty and those two tags are not emitted at
# all, which hid the bug and made the flag look load-bearing.
#
# With the tags corrected, the ordinary PLT build works in both. See D036.
TARGET_FLAGS := -target $(TARGET_TRIPLE) -ffreestanding -fno-builtin -nostdlib \
                -fPIC -fno-stack-protector -fvisibility=hidden
# Undefined symbols are the point, not a mistake: each one becomes an import the
# loader resolves. Building shared keeps a dynamic symbol table, which is where the
# import list lives.
#
# max-page-size forces every loadable segment onto a page boundary. Without it lld
# packs them tightly and the last one lands mid-page:
#
#   LoadModuleToMemory: segment_addr ...: 0x00000000800174e0
#   address_space.cpp:345 SplitRegion: Unreachable code!
#
# A loader that maps segments by page cannot place one that begins part-way into a
# page. 16 KiB is the console's own granularity rather than the host's 4 KiB.
#
# norelro is needed as well: GNU RELRO packs the writable segment hard against the
# read-only part, which defeats the alignment for that one segment - and it is the
# writable one, so it is the segment a loader is least able to fudge.
#
# separate-loadable-segments is what actually finishes the job. max-page-size alone
# only makes segments *congruent* modulo the page size, which is what the ELF
# specification requires and is not the same as starting on a page boundary. A loader
# that maps whole pages needs the stronger property.
# -fuse-ld=lld is not a preference. GNU ld silently ignores
# -z separate-loadable-segments ("warning: ... ignored"), so the segment
# alignment the loader needs never happens and the only sign is a warning
# buried in a successful build.
#
# The linker script lives in the sibling `selfish` repository, which is where the segment
# layout rules and the format tables moved. The repositories are checked out as a set, the
# same convention `tool/Cargo.toml` already depends on.
SELFISH ?= ../selfish

# $(SELFISH)/link/module.ld replaces most of the flag-hunting: it declares the vendor segments
# directly, so they come out of the linker with correct offsets rather than being
# grafted onto a finished binary. max-page-size stays, because the script's ALIGN
# controls addresses and not the alignment lld records in each program header.
# `-shared` rather than `-pie`, and that is not what makes obSCEne differ from a standard
# module. Building `-pie` was tried and changed nothing in the program header table (D098):
# `$(SELFISH)/link/module.ld` declares PHDRS explicitly, so the script decides what segments exist and
# the link mode has no say. Anything missing has to be added to the script.
# Which linker script this link uses, as a variable so the eboot can override it per target.
#
# **Two `-T` flags do not mean "prefer the second".** lld applies both, and the result is sections
# placed by one script at offsets computed by the other: eleven `unable to place section ... check
# your linker script for overflows` errors, none of which name the real cause. So there is exactly
# one `-T` on any link, and which script it names is chosen here.
# The ELF identity an **eboot** declares, which is not the one a module declares.
#
# A console's kernel carries three syscall vectors - a previous-generation one for backward
# compatibility, a FreeBSD one, and a native one - and picks between them from the executable's
# identity. Pick wrong and the first syscall the loader makes is not in the table it selected: the
# kernel returns `ENOSYS` and sends `SIGSYS`, which is exactly how a launch was dying, with the
# process created, the display handed over, and `exited on signal 12` before our entry.
#
# The eboot inside a real package is **previous generation throughout**: previous-generation
# container magic, `EI_ABIVERSION` 0, and the legacy dynamic-table convention. This build was
# emitting a previous-generation container around a module stamped 2 with current-generation
# tables - three identities in one file, which is the shape of thing that selects the wrong
# vector. (measured against a real package's eboot)
#
# `GEN` still decides what the **code** targets, and stays 5: `-DOBSCENE_GEN` picks which entry
# points the probe calls at run time, and a real eboot marked `0` is still a current-generation
# title. What changes here is only what the file says it is.
EBOOT_GEN ?= 4
EBOOT_TABLE ?= legacy

# Which `e_type` the eboot carries, and it is the one thing the console and every emulator
# disagree about.
#
#   fixed       0xFE00   what a real package's eboot carries, and what a console's rtld runs
#   executable  0xFE10   what every emulator and `elfldr` accept, and what rtld refuses by name
#
# `fixed` is the default because the package is the point. `executable` exists so the *same*
# eboot - same sources, same twelve libraries, same run-time census - can be put through an
# emulator before it is put through a console:
#
#   make eboot EBOOT_KIND=executable    # then: PS5PCEM game-run build/eboot.bin
#
# Everything except the four bytes is identical, so a run under an emulator tests the parts
# that are expensive to get wrong on hardware. Nothing else in the build changes. (D231)
EBOOT_KIND ?= fixed

TARGET_LD ?= $(SELFISH)/link/module.ld

# Lazily expanded (`=`, not `:=`) so a target-specific `TARGET_LD` is picked up. With `:=` the
# script name is baked in when this line is read and the eboot silently links as a module.
TARGET_LDFLAGS = -fuse-ld=lld -shared -Wl,-e,obscene_start \
                  -Wl,--unresolved-symbols=ignore-all -Wl,-Bsymbolic \
                  -Wl,-T,$(TARGET_LD) \
                  -Wl,-z,noexecstack -Wl,-z,max-page-size=0x4000 \
                  -Wl,-z,common-page-size=0x4000 -Wl,-z,norelro

# ---- Per-file objects, so a build recompiles only what changed ---------------
#
# Each C target used to compile every one of its sources in a single `clang <all .c>` command,
# so every build recompiled everything - `surface.c` and its thirty-five-thousand-symbol census
# included - and `-j` had nothing to parallelise inside one compile. These rules compile each
# source to its own object instead: a one-file edit rebuilds one file, `-j` compiles files at
# once, and a compiler cache can hit on the ones that did not change.
#
# Objects are per-target because the same source gets different defines for the host, the
# module and the eboot (`OBSCENE_HOST_BUILD` against `OBSCENE_TARGET`, and the eboot's extra
# `OBS_CENSUS_LINKED=0`), so a `.o` cannot be shared between them.
#
# `-MMD -MP` writes each object's header dependencies to a `.d`, included at the foot of this
# file, so a header edit rebuilds exactly the objects that include it - the thing the old
# whole-file compile got for free by always rebuilding everything.
#
# **The `.flags` sentinel is the part that makes this safe.** make decides what to rebuild from
# file times, not from compile flags, so flipping `GEN`, `DISPLAY_MEM` or `EXCLUDE` between
# builds would otherwise reuse objects built with the old value - a silently wrong binary, and
# on this project a wrong binary costs a hardware cycle to notice. Each object depends on its
# target's sentinel, which is rewritten (and so made newer than every object) exactly when the
# flag string changes. The flags are written with make's `$(file ...)` rather than through the
# shell, because they carry embedded quotes - `-DOBSCENE_BUILD_ID='"dev"'` - that no shell
# quoting survives.
#
# Link-only flags (`-nostdlib`, the `-Wl,` group) are kept off the compile line and the `-D`,
# `-I` and warning flags are kept off the link line: each is "unused" in the other phase, and
# `-Werror` turns an unused-argument warning into a failure.
OBJROOT := $(BUILD)/obj

# Compile flags per target. `-nostdlib` is filtered out of the module/eboot set because it is a
# link flag; everything else in TARGET_FLAGS is a compile flag.
HOST_CFLAGS = $(STD) $(STAMP) $(WARNINGS) $(INCLUDE) -DOBSCENE_HOST_BUILD -fno-builtin -O1
MODULE_CFLAGS = $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' $(WARNINGS) $(INCLUDE) \
               $(filter-out -nostdlib,$(TARGET_FLAGS))
EBOOT_CFLAGS = $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' -DOBS_CENSUS_LINKED=0 $(WARNINGS) \
               $(INCLUDE) $(filter-out -nostdlib,$(TARGET_FLAGS))
INJECTOR_CFLAGS = $(STD) $(STAMP) -DOBSCENE_TARGET='"injector"' $(WARNINGS) $(INCLUDE) \
                  $(filter-out -nostdlib,$(TARGET_FLAGS))

# Link flags: the target/link flags only, no `-D`/`-I`/warnings, and unused-argument warnings
# silenced so a compile flag that clang forwards to the linker does not fail the objects-only
# link under `-Werror`.
TARGET_LINK = $(TARGET_FLAGS) $(TARGET_LDFLAGS) -Wno-unused-command-line-argument

HOST_OBJ := $(patsubst src/%.c,$(OBJROOT)/host/%.o,$(HOST_SRC))
MODULE_OBJ := $(patsubst src/%.c,$(OBJROOT)/module/%.o,$(TARGET_SRC))
EBOOT_OBJ := $(patsubst src/%.c,$(OBJROOT)/eboot/%.o,$(TARGET_SRC))
INJECTOR_OBJ := $(patsubst src/%.c,$(OBJROOT)/injector/%.o,$(INJECTOR_SRC))

.PHONY: FORCE_FLAGS
FORCE_FLAGS:

# The object directories, as order-only prerequisites so they exist before anything writes into
# them. This matters for `.flags` in particular: `$(file ...)` is evaluated when make expands
# the recipe, which is before any `mkdir` in that same recipe would run - so the directory has
# to be made by a prerequisite, not by the recipe itself.
$(OBJROOT)/host $(OBJROOT)/module $(OBJROOT)/eboot $(OBJROOT)/injector:
	@mkdir -p $@

$(OBJROOT)/host/.flags: FORCE_FLAGS | $(OBJROOT)/host
	@$(file >$@.tmp,$(HOST_CFLAGS)) cmp -s $@.tmp $@ 2>/dev/null && rm -f $@.tmp || mv $@.tmp $@
$(OBJROOT)/module/.flags: FORCE_FLAGS | $(OBJROOT)/module
	@$(file >$@.tmp,$(MODULE_CFLAGS)) cmp -s $@.tmp $@ 2>/dev/null && rm -f $@.tmp || mv $@.tmp $@
$(OBJROOT)/eboot/.flags: FORCE_FLAGS | $(OBJROOT)/eboot
	@$(file >$@.tmp,$(EBOOT_CFLAGS)) cmp -s $@.tmp $@ 2>/dev/null && rm -f $@.tmp || mv $@.tmp $@
$(OBJROOT)/injector/.flags: FORCE_FLAGS | $(OBJROOT)/injector
	@$(file >$@.tmp,$(INJECTOR_CFLAGS)) cmp -s $@.tmp $@ 2>/dev/null && rm -f $@.tmp || mv $@.tmp $@

$(HOST_OBJ): $(OBJROOT)/host/%.o: src/%.c $(OBJROOT)/host/.flags
	@mkdir -p $(@D)
	$(CC) $(HOST_CFLAGS) -MMD -MP -c -o $@ $<
$(MODULE_OBJ): $(OBJROOT)/module/%.o: src/%.c $(OBJROOT)/module/.flags
	@mkdir -p $(@D)
	$(CC) $(MODULE_CFLAGS) -MMD -MP -c -o $@ $<
$(EBOOT_OBJ): $(OBJROOT)/eboot/%.o: src/%.c $(OBJROOT)/eboot/.flags
	@mkdir -p $(@D)
	$(CC) $(EBOOT_CFLAGS) -MMD -MP -c -o $@ $<
$(INJECTOR_OBJ): $(OBJROOT)/injector/%.o: src/%.c $(OBJROOT)/injector/.flags
	@mkdir -p $(@D)
	$(CC) $(INJECTOR_CFLAGS) -MMD -MP -c -o $@ $<

$(OBJROOT)/injector/blob.o: src/injector/blob.S $(BUILD)/obscene-payload.elf $(OBJROOT)/injector/.flags
	@mkdir -p $(@D)
	$(CC) $(INJECTOR_CFLAGS) -DEMBED_PAYLOAD_PATH='"$(BUILD)/obscene-payload.elf"' -c -o $@ $<

# Three shapes, and the distinction that matters is which loader accepts them.
#
#   module   vendor format: ET_SCE_DYNEXEC, vendor segments, NID-encoded symbols.
#            What the system loader takes, and therefore what an emulator takes.
#   payload  plain ET_DYN ELF with ordinary symbols and DT_NEEDED. What a homebrew
#            ELF loader running on the console takes. Rejected by every emulator.
#   host     native, platform stubbed, for checking the harness itself.
#
# "module" and "payload" rather than "commercial" and "homebrew": the difference is
# the loader, not the author. Homebrew can be built as a module, and the words are
# the ecosystem's own.
.PHONY: all sce-module sce-module-guard eboot-libs-guard module module-min module-min-noimport module-min-debugout payload payload-min eboot eboot-min pkg pkg-min host run pretty clean check diff injector inject

all: module payload host injector

$(BUILD):
	mkdir -p $(BUILD)

# Always invoked, never a file rule. Make cannot know when the Rust sources changed,
# so a file rule sees the binary exists and skips it - which silently runs yesterday's
# tool against today's module. Cargo already does this check properly and is fast when
# there is nothing to do.
.PHONY: tool
tool:
	@cd tool && CARGO_TARGET_DIR=$(TOOL_TARGET) cargo build --release --quiet

# Which library each import comes from.
#
# A module encodes an import as a library id, and an id with no library declared
# against it resolves to nothing. The association lives in the check and census tables
# and nowhere else, so it is asked for rather than written down twice — see
# src/probe/imports.c.
#
# This makes the host build a prerequisite of the module build. It already was in
# principle (D001); now make knows.
$(BUILD)/symbols.txt: host | $(BUILD)
	@$(BUILD)/obscene-host --symbols > $@

# The same manifest without the census, for the eboot.
#
# The census asks whether a symbol exists by taking its address, which imports it and requires
# its library. That is 339 libraries no check calls by name, and a system loader loads every one
# before this program runs. The eboot is compiled with `OBS_CENSUS_LINKED=0` so it takes none of
# those addresses, and this is the list covering the symbols it does still reference. (D227)
$(BUILD)/symbols-no-census.txt: host | $(BUILD)
	@$(BUILD)/obscene-host --symbols-no-census > $@

# The vendor-format build. Emulators accept only this shape, so it is the only one
# testable without a console — which is why it comes first.
module: tool $(BUILD)/symbols.txt $(MODULE_OBJ) | $(BUILD)
	$(CC) $(TARGET_LINK) -o $(BUILD)/obscene.module.elf $(MODULE_OBJ)
	@$(TOOL) mkmodule $(BUILD)/obscene.module.elf --symbols $(BUILD)/symbols.txt --generation $(GEN) --table $(TABLE)

# The one-import control.
#
# Same linker script, same tags, same mkmodule - one import instead of four hundred.
# When the full module fails inside a loader with nothing in the log, this says whether
# the cause is structural or a matter of scale. See src/probe/min.c for what each outcome
# means.
module-min: tool | $(BUILD)
	@echo "libkernel sceKernelWrite" > $(BUILD)/min-symbols.txt
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' $(WARNINGS) $(INCLUDE) \
	    $(TARGET_FLAGS) $(TARGET_LDFLAGS) \
	    -o $(BUILD)/obscene-min.module.elf src/probe/min.c src/probe/crt.c
	@$(TOOL) mkmodule $(BUILD)/obscene-min.module.elf --symbols $(BUILD)/min-symbols.txt \
	    --module-name obscene-min

# The same control, calling a candidate output channel.
#
# An emulator logs an unimplemented function by name when it is called, so running
# this and reading the log says whether the candidate exists - without adding a
# declaration of uncertain arity to the probe itself.
module-min-debugout: tool | $(BUILD)
	@echo "libkernel sceKernelWrite" > $(BUILD)/dbg-symbols.txt
	@echo "libkernel sceKernelDebugOutText" >> $(BUILD)/dbg-symbols.txt
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' -DOBSCENE_MIN_DEBUG_OUT \
	    $(WARNINGS) $(INCLUDE) $(TARGET_FLAGS) $(TARGET_LDFLAGS) \
	    -o $(BUILD)/obscene-min-debugout.module.elf src/probe/min.c src/probe/crt.c
	@$(TOOL) mkmodule $(BUILD)/obscene-min-debugout.module.elf \
	    --symbols $(BUILD)/dbg-symbols.txt --module-name obscene-min

# The same control with nothing to resolve.
#
# Splits the remaining question: if this runs and module-min does not, the fault is
# in resolving imports. If both fail, imports are not involved at all.
module-min-noimport: tool | $(BUILD)
	@: > $(BUILD)/empty-symbols.txt
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' -DOBSCENE_MIN_NO_IMPORT \
	    $(WARNINGS) $(INCLUDE) $(TARGET_FLAGS) $(TARGET_LDFLAGS) \
	    -o $(BUILD)/obscene-min-noimport.module.elf src/probe/min.c src/probe/crt.c
	@$(TOOL) mkmodule $(BUILD)/obscene-min-noimport.module.elf \
	    --symbols $(BUILD)/empty-symbols.txt --module-name obscene-min

# The plain-ELF build. A homebrew ELF loader running on the console takes this; no
# emulator will touch it — a real, working homebrew binary is rejected by both, for
# the same reason our first attempt was.
#
# It uses neither the vendor linker script nor mkmodule, because both exist solely to
# produce the shape this one must not have. Every check is shared with the module
# build: a probe whose checks differ between targets measures two different things.
payload: $(BUILD)
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"payload"' -DOBS_NO_UI $(WARNINGS) $(INCLUDE) \
	    $(TARGET_FLAGS) -fuse-ld=lld -shared -Wl,-e,obscene_start \
	    -Wl,--unresolved-symbols=ignore-all -Wl,-z,noexecstack \
	    -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000 \
	    -o $(BUILD)/obscene-payload.elf $(TARGET_SRC)
	@cp -f $(BUILD)/obscene-payload.elf $(BUILD)/obscene.elf 2>/dev/null || true

# The standalone injector payload.
#
# Consumes session kernel R/W to hijack a target native process, map obscene.elf
# segments, and execute the probe natively.
injector: $(BUILD) payload $(INJECTOR_OBJ) $(OBJROOT)/injector/blob.o
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"injector"' $(WARNINGS) $(INCLUDE) \
	    $(TARGET_FLAGS) -fuse-ld=lld -shared -Wl,-e,injector_start \
	    -Wl,-T,link/injector.ld \
	    -Wl,--unresolved-symbols=ignore-all -Wl,-z,noexecstack \
	    -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000 \
	    -o $(BUILD)/obscene-injector.elf $(INJECTOR_OBJ) $(OBJROOT)/injector/blob.o

inject: injector

# The one-import control, in the shape a console takes.
#
# The minimal builds were all vendor-shaped, which made them emulator-only - so the first
# time a console was available there was no small thing to send it, and the first artifact
# tried was a 13MB vendor module fed to a homebrew loader that had no idea what to do with
# it. The loader exited. Nothing was learnt except that the wrong file had been sent.
#
# Two axes, four builds: minimal or full, vendor-shaped or plain. This is the cell that was
# missing, and it is the one to reach for first on real hardware - if it runs, the transport
# works and the loader is fine; if it does not, nothing about the suite is implicated.
#
# It reports through descriptors 1 and 2 like the other minimal builds, which on a console
# have no parent process to catch them. OBSCENE_MIN_FILE routes the same line to
# /data/obscene-report.txt instead, so the evidence can be fetched back over FTP.
# 16 KB pages, and this is not cosmetic - it is what makes a payload run on hardware at all.
#
# A 4 KB-aligned payload never reached its entry: the console maps with 16 KB pages, so the
# loader's mapping was wrong and it jumped into unmapped memory (SIGSEGV before a single
# instruction of ours). Rebuilt with these flags the same payload arrived and ran. The module
# build always had them; the payload builds did not, and that cost a night of chasing a
# phantom import-resolution bug. (D208)
payload-min: | $(BUILD)
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"payload"' -DOBSCENE_MIN_FILE -DOBS_NO_UI \
	    $(WARNINGS) $(INCLUDE) $(TARGET_FLAGS) -fuse-ld=lld -shared -Wl,-e,obscene_start \
	    -Wl,--unresolved-symbols=ignore-all -Wl,-z,noexecstack \
	    -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000 \
	    -o $(BUILD)/obscene-min.elf src/probe/min.c src/probe/crt.c
	@cp -f $(BUILD)/obscene-min.elf $(BUILD)/obscene-payload-min.elf 2>/dev/null || true

# ---------------------------------------------------------------------------------------
# The loading mechanisms, as artifacts
#
# obSCEne measures a platform by calling it. **How the program got there is part of the
# platform**, and until now every hardware path this project could take bypassed the piece
# of it `docs/BOOT.md` is entirely about.
#
# Shapes, loaders, and their release artifacts:
#
#   obscene-payload.elf   plain ET_DYN      a homebrew ELF loader maps it itself
#   obscene-injector.elf  plain ET_DYN      session R/W injector into foreground native title
#   obscene.module.elf    vendor ELF        emulators, via their "not a SELF" path
#   obscene-eboot.zip     fSELF (eboot.bin) THE SYSTEM LOADER, from an app directory
#   obscene.pkg           package           the installer, then the system loader
#
# The first two are built. They answer "what do the libraries do" and say **nothing** about
# the loader: elfldr maps segments itself, and an emulator's loader is somebody's reading of
# one. Every open question in BOOT.md - whether the console calls DT_INIT, what it does with
# an unresolved import, whether it requires a SELF at all - is unreachable from both.
#
# Only an eboot.bin in an app directory is loaded by the real thing. That is why these are
# separate artifacts rather than packaging conveniences.
# ---------------------------------------------------------------------------------------

# fSELF, not signed.
#
# A retail eboot.bin is a SELF signed with keys nobody outside the vendor has, and that is a
# wall rather than a gap. A *fake* SELF is a different object: the same container with a known
# dummy where the signature goes, which a console running a kernel patch accepts. No signing
# is involved, which is why the "it cannot sign one, and that is not going to change" line in
# BOOT.md was true of retail and wrong about this.
#
# The format came from two independent open-source readers and one open-source writer, none of
# them a vendor binary - see the header of $(SELFISH)/data/self-format.tsv, which is where every constant
# actually lives. `mkself` reads that table; it does not carry a copy. (D180, D182)
#
# Proven against a loader rather than asserted: shadPS4 takes the container, loads the module
# out of it, resolves imports and runs the suite to its end record. Its first rejection is why
# the entry props are set at all - it walks the entries looking for one with the blocked bit,
# and a container full of zero props sent it into UNREACHABLE(). (D183)
# The eboot is built from the same sources as `module` and stamped differently, because it is
# loaded by something else.
#
# Two differences, both measured against the eboot inside a real package rather than chosen:
#
#   * `--fixed` writes `e_type` `0xFE00`. A console's own `rtld` refuses `0xFE10` by name -
#     `Unsupported ELF e_type. /app0/eboot.bin fe10` - having mounted the filesystem and found
#     the file. Every emulator and `elfldr` want `0xFE10`, which is why `module` still writes it.
#   * the container carries the **previous** generation's magic. That is not a special case for
#     this project: thirty-three containers inside real current-generation packages carry it, and
#     `selfish wrap` already defaults to 4 for that reason. This build was passing `GEN`, so a
#     `GEN=5` package got a container no system loader accepts.
#
# Built separately rather than re-stamping `module`, because `mkmodule` rewrites its input in
# place and running it twice over one file would process it twice.
# How many libraries an eboot may require, and why there is a limit at all.
#
# `DT_NEEDED` is an assertion - "this module requires that one" - and a system loader acts on
# it **before a single instruction of ours runs**: it finds and loads every named module, and
# there is nothing we can guard, because we do not exist yet. Every platform symbol here is weak
# and address-checked before it is called, which is why the probe survives a missing *function*;
# that guard is at the wrong layer for a missing *library*.
#
# The census names 352 libraries, so the eboot required 352. A title gets six - `libkernel`,
# `libSceLibcInternal`, `libSceSysmodule`, `libSceDiscMap`, `libSceNet`, `libSceIpmi`, measured
# from a crash dump of a run that worked - and demanding the rest took the console down with no
# report, because it died inside the loader before `EXEC`. (D226)
#
# This does not claim to know which libraries a title may load. It refuses to let the build
# *silently* require hundreds, which is the difference between a failed build and an hour of
# re-running a jailbreak. Raise it deliberately, or set `EBOOT_LIBS=any` to opt out:
#
#   make eboot EBOOT_LIBS=any        # the old behaviour, with the risk taken on purpose
#
# `module` and `payload` are not gated. They run under a homebrew loader with different
# privileges and under emulators that stub everything, where requiring the whole census is
# exactly what is wanted.
EBOOT_LIBS ?= 16

eboot-libs-guard: $(BUILD)/symbols-no-census.txt
	@if [ "$(EBOOT_LIBS)" != "any" ]; then \
	    n=$$(cut -d' ' -f1 $(BUILD)/symbols-no-census.txt | sort -u | wc -l); \
	    if [ "$$n" -gt "$(EBOOT_LIBS)" ]; then \
	        echo "eboot: this build requires $$n libraries; the limit is $(EBOOT_LIBS)." >&2; \
	        echo "       A system loader loads every one before our code runs, so a library" >&2; \
	        echo "       a title cannot load is a crash with no report rather than a finding." >&2; \
	        echo "       See D226. Probe a library at run time; require only what must link." >&2; \
	        echo "       Override with EBOOT_LIBS=<n> or EBOOT_LIBS=any." >&2; \
	        exit 1; \
	    fi; \
	fi

eboot: TARGET_LD := $(SELFISH)/link/eboot.ld
eboot: tool eboot-libs-guard $(BUILD)/symbols-no-census.txt $(EBOOT_OBJ) | $(BUILD)
	$(CC) $(TARGET_LINK) -o $(BUILD)/obscene.eboot.elf $(EBOOT_OBJ)
	@$(TOOL) mkmodule $(BUILD)/obscene.eboot.elf --symbols $(BUILD)/symbols-no-census.txt \
	    --generation $(EBOOT_GEN) --table $(EBOOT_TABLE) --kind $(EBOOT_KIND)
	@# The container generation follows EBOOT_GEN, the same knob the module above uses and the one
	@# EBOOT_GEN's comment already describes as "what the file says it is". It was hardcoded 4 while
	@# mkmodule took the variable, so EBOOT_GEN=5 stamped a gen-5 module inside a gen-4 container.
	@# Default stays 4 (the proven fake-signed container); EBOOT_GEN=5 builds the current-generation
	@# container, whose structure selfish still calls a hypothesis until hardware accepts one. (D289)
	@$(TOOL) mkself $(BUILD)/obscene.eboot.elf --out $(BUILD)/eboot.bin --generation $(EBOOT_GEN)

# The installable package.
#
# Exercises a loader the others do not: the installer. A title arriving through it is
# registered, given a sandbox and launched the way a retail title is, which is the only
# configuration where obSCEne is measuring the platform's own path end to end.
#
# NOT IMPLEMENTED, and gated on eboot above - a package is a container around an eboot.bin,
# not an alternative to one.
#
# What a title needs is no longer guesswork; it was measured on hardware rather than recalled:
#
#     app0/eboot.bin
#     app0/sce_sys/param.json     <- param.json. Not param.sfo, which is the previous console
#
# and param.json is four fields: applicationCategoryType, titleId, and a localizedParameters
# block carrying defaultLanguage and a titleName. (D180)
# The pipeline is selfish's, staged here. It is wired end to end so that the day selfish-pkg
# exposes its filesystem-image builder as a command, this target completes with no change here.
# Until then it runs as far as selfish allows and stops at the first step selfish cannot yet do,
# naming it - which is honest readiness, not a stub that pretends nothing exists.
#
#   eboot.bin + param.json  ->  app tree  ->  pfs image (selfish-pfs)  ->  package (selfish pack)
#
# The image builder is `selfish-pfs::build`, which exists as a library function but is not yet a
# CLI command; `selfish pack` already takes the finished `--image`. See selfish-pkg.
# `sce-module` is a prerequisite for the same reason it is one of `pkg-min`: a title
# with no `/app0/sce_module` is refused with PRX_SCE_MODULE_LOAD_ERROR, so a package
# built without it is never the package anybody wanted. `pkg-min` declared it and this
# did not, which is the whole of the difference - `build-pkg.sh` warned and then died
# several steps later on a missing file, reporting `NotFound` with no path.
pkg: eboot sce-module | $(BUILD)
	@SELFISH=$(SELFISH) GEN=$(GEN) bash scripts/build-pkg.sh $(BUILD)

# obSCEne as a native title entry, which is a different thing from a package.
#
# `pkg` builds a previous-generation package: it installs through the compatibility path and is
# badged accordingly, because that is what it is. `native` builds the current generation's own
# title layout — a directory described by param.json, registered by a payload with kernel
# privileges.
#
# It is a home-screen entry, not an executable title. A native title that ran its own code
# would need a signed eboot, and no fake-signing keyset exists for this generation. obSCEne's
# code runs outside the compatibility sandbox already, as a payload; this gives it somewhere to
# be launched from.
.PHONY: native
# The native title's eboot uses the container a real current-generation title's eboot uses, which
# hardware measurement showed is `4F 15 3D 1D` (the default EBOOT_GEN=4) - NOT the `54 14 F5 EE`
# magic selfish's table (a stated hypothesis) labels "current generation" and that no installed
# title on hardware actually carries: `048-selfaudit/metadata-differential` measured
# gen4_containers=1, gen5_containers=0 on a ps5 native title (PPSA02664). So `native` takes the
# default gen-4 container, same as `pkg`; what makes it a ps5 native title is `param.json` + native
# registration, not the eboot magic. `make native EBOOT_GEN=5` still builds the `54 14 F5 EE`
# variant to try against hardware, but it matches nothing measured so far. (D289, D293)
native: eboot | $(BUILD)
	@SELFISH=$(SELFISH) bash scripts/build-native.sh $(BUILD)

# The same package around the *minimal* module, which is what to send at a console first.
#
# `module-min` is one import and one library, built by the same linker script through the same
# `mkmodule` with the same tags, so everything structural is present and everything about scale
# is gone. Wrapping it changes what a failed launch means: the full package carries an eleven
# megabyte filesystem and four hundred imports, and a console that dies mounting it has too many
# possible causes to reason about.
#
# The immediate reason it exists: a console panicked *inside the app0 mount*, before the eboot
# ran at all - no boot note, no report, and the kernel log stopping mid-mount. The eboot's
# contents cannot be responsible for that, so the thing to vary is the image, and this makes it
# as small as this project can make it. If a two-block image still takes the console down, the
# fault is in the filesystem writer and nothing about obSCEne is implicated.
#
#   make pkg-min GEN=5 HARDWARE=1
pkg-min: eboot-min sce-module | $(BUILD)
	@SELFISH=$(SELFISH) GEN=$(GEN) bash scripts/build-pkg.sh $(BUILD)

# The same two stamps as `eboot`, for the same reasons, around the one-import module.
#
# `MIN_DEFINES` selects which of the variants in `src/probe/min.c` gets built, and it is the split
# that matters once an eboot *loads*: a fault inside `libkernel` is either the platform
# library's own initialisation or our first call into it, and those need opposite fixes.
#
#   make eboot-min MIN_DEFINES=-DOBSCENE_MIN_NO_IMPORT   # calls nothing, only spins
#   make eboot-min MIN_DEFINES=-DOBSCENE_MIN_FILE        # leaves evidence a console keeps
#
# The importless build is the control: it still loads `libkernel`, still carries the vendor
# segment and the process parameters, and makes no call. Faulting anyway puts the cause
# before our first instruction. (D218)
MIN_DEFINES ?=

# The modules a title ships in `/app0/sce_module`, which it is required to have.
#
# A real package carries vendor modules there and **this project does not redistribute them**,
# so these are built here out of `src/probe/sce_module.c` - a stub that loads and does nothing. The
# names match what a real package uses, because the system loads that directory by name and an
# honestly-named file it does not look for satisfies nothing. (D220)
# **Named after the modules a real package bundles**, because the system looks for those names.
#
# This was `obscene-placeholder`, on the reasoning that the loader enumerates the directory: when
# the stubs were malformed it named `libSceFios2.prx` in the error, so it had plainly read them.
# That was an inference and it was wrong - naming a file it looked for is equally consistent with
# a fixed list of names. Renamed to something a title never bundles, a well-formed module in the
# right place produced the same complaint as an empty directory:
#
#     # === Lack of a .prx file in /app0/sce_module is detected!!! ===
#
# with no `rtld` line naming anything, because nothing was looked at. (D234)
#
# ### Why the names are safe now and were not before
#
# Import resolution is by library name, so a stub named after a library the probe imports from
# would answer the probe's own imports - the probe measuring itself and finding no fault. The
# **census** imports `libc` and `libSceFios2` among its 352, which is what made these names
# dangerous when the eboot linked it.
#
# The eboot no longer does (D228). It links twelve libraries and neither is among them, so the
# collision is gone - and `sce-module-guard` checks that against the eboot's own manifest rather
# than against a list the eboot does not use.
SCE_MODULES ?= libc libSceFios2

# A stub must never share a name with a library the probe imports from.
#
# Resolution is by the library name written into the eboot's tables, so today this cannot
# collide: obSCEne names `libkernel`, `libSceLibcInternal`, `libScePosix` and nine others, and
# `libc` is not among them - its string and math calls go to the platform's own
# `libSceLibcInternal`, loaded from the system path.
#
# "Cannot collide today" is not a property, it is an observation, and the failure it guards
# against is the worst kind this project has: a probe that resolves an import against a stub
# shipped in its own package reports that the platform function exists and returns zero. It
# would measure itself and say nothing was wrong. So the collision is a build error, checked
# against the manifest rather than remembered. (D220)
sce-module-guard:
	@for name in $(SCE_MODULES); do \
	    if grep -q "{\"$$name\"," src/probe/imports.c; then \
	        echo "sce-module: '$$name' is BOTH a bundled stub and a library the probe imports" >&2; \
	        echo "            from (src/probe/imports.c). Imports would resolve against the stub and" >&2; \
	        echo "            the probe would be measuring itself. Rename the stub." >&2; \
	        exit 1; \
	    fi; \
	    if [ -f $(BUILD)/symbols-no-census.txt ] \
	        && cut -d' ' -f1 $(BUILD)/symbols-no-census.txt | grep -qx "$$name"; then \
	        echo "sce-module: '$$name' is BOTH a bundled stub and a library the eboot imports" >&2; \
	        echo "            from ($(BUILD)/symbols-no-census.txt). Imports would resolve" >&2; \
	        echo "            against the stub and the probe would be measuring itself." >&2; \
	        exit 1; \
	    fi; \
	done

# A bundled library is the third layout, and neither of the other scripts produces it: the
# headers must sit outside the first segment (which `module.ld` does not do) and the image must
# be based at zero (which `eboot.ld` does not do). See `link/library.ld`. (D222)
sce-module: TARGET_LD := $(SELFISH)/link/library.ld
# The same generation and table convention as the eboot, because they load together and a
# loader checks. It took the defaults and the eboot did not, which the console named exactly:
#
#     ### ERROR: ABIVERSION mismatch. /app0/sce_module/libSceFios2.prx
#     [rtld] ERROR self_load_shared_object:2826: B: res 0 (libSceFios2.prx)  val 2
#
# `val 2` is `EI_ABIVERSION`, and the eboot's is 0. (D222)
sce-module: sce-module-guard tool | $(BUILD)
	@echo "libkernel sceKernelWrite" > $(BUILD)/sce-module-symbols.txt
	@# Emptied, not just created. `build-pkg.sh` ships whatever is in here, so a module that
	@# has been renamed leaves its old name behind and the package carries both - which is how
	@# a build after renaming the stubs shipped `libc.prx` and `libSceFios2.prx` alongside the
	@# replacement that exists precisely so those names are not used. (D224)
	@rm -rf $(BUILD)/sce_module
	@mkdir -p $(BUILD)/sce_module
	@for name in $(SCE_MODULES); do \
	    $(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' $(WARNINGS) $(INCLUDE) \
	        $(TARGET_FLAGS) $(TARGET_LDFLAGS) -Wl,-e,module_start \
	        -o $(BUILD)/$$name.module.elf src/probe/sce_module.c || exit 1; \
	    $(TOOL) mkmodule $(BUILD)/$$name.module.elf \
	        --symbols $(BUILD)/sce-module-symbols.txt --module-name $$name --kind shared \
	        --generation $(EBOOT_GEN) --table $(EBOOT_TABLE) || exit 1; \
	    $(TOOL) mkself $(BUILD)/$$name.module.elf \
	        --out $(BUILD)/sce_module/$$name.prx --generation 4 || exit 1; \
	done

eboot-min: TARGET_LD := $(SELFISH)/link/eboot.ld
eboot-min: tool | $(BUILD)
	@printf 'libkernel sceKernelWrite\nlibkernel sceKernelOpen\nlibkernel sceKernelClose\nlibkernel sceKernelDebugOutText\n' \
	    > $(BUILD)/min-symbols.txt
	$(CC) $(STD) $(STAMP) -DOBSCENE_TARGET='"module"' $(MIN_DEFINES) $(WARNINGS) $(INCLUDE) \
	    $(TARGET_FLAGS) $(TARGET_LDFLAGS) \
	    -o $(BUILD)/obscene-min.eboot.elf src/probe/min.c src/probe/crt.c
	@$(TOOL) mkmodule $(BUILD)/obscene-min.eboot.elf --symbols $(BUILD)/min-symbols.txt \
	    --module-name obscene-min --generation $(EBOOT_GEN) --table $(EBOOT_TABLE) --kind $(EBOOT_KIND)
	@$(TOOL) mkself $(BUILD)/obscene-min.eboot.elf --out $(BUILD)/eboot.bin --generation 4

# -fno-builtin matters here for the same reason it does on the target, and it is not
# a workaround. The census declares standard C names as data so they can only be
# probed and never called; clang knows those names as builtins and rejects the
# declaration as "a different kind of symbol". Telling it not to assume it knows them
# is exactly true - on the guest they are whatever the platform library provides.
host: $(HOST_OBJ) | $(BUILD)
	$(CC) -o $(BUILD)/obscene-host $(HOST_OBJ) $(HOST_LIBS)

# The Steam Deck build.
#
# The Deck is x86-64 Linux with an RDNA2 GPU and **no vendor libraries**, so it is the
# host-shaped build - platform calls stubbed, POSIX socket and file backends, the same
# source lists - not a console-shaped one. What makes it worth a target of its own is not
# the CPU side (that is the host build) but the GPU: it is the project's first and only
# RDNA2 execution oracle, which is why this always builds `GPU=1` (real silicon through
# Vulkan, `gpu_vulkan.c`).
#
# It serves the command protocol the same way the host build does, at runtime:
# `obscene-deck --serve`. There is no SERVE build flag here - that is a console-module
# concept (the module has no argv); a Linux binary takes the flag.
#
# Named and separate from `host` deliberately: "build for the Deck" is a first-class thing,
# and the machine identity a corpus is graded by is operator-asserted anyway (D108), so the
# binary being distinct is what stops a Deck result and a dev-machine result being confused
# by habit.
.PHONY: deck
deck:
	$(MAKE) host GPU=1 BUILD=$(BUILD)
	cp $(BUILD)/obscene-host $(BUILD)/obscene-deck
	@echo "built $(BUILD)/obscene-deck - copy to a Deck and run: ./obscene-deck --serve"

# The host build is run from $(BUILD), and that is a performance fix, not tidiness.
#
# The sink's last resort is a bare `obscene-report.txt`, which lands in the working directory.
# Under WSL the working directory is the repository on a Windows 9p mount, so the host binary
# writes 36,000 records unbuffered across that mount: **8.9 seconds becomes nearly six minutes**,
# and `verify.sh` runs the host more than once. That is where a fifty-minute verification went.
#
# D012 already says BUILD must be Linux-local. The sink does not follow BUILD - it follows the
# working directory - so the rule has to be applied here too, at every site that runs the host.
# Runs the host build. Exits non-zero on any red, so it works as a gate.
run: host
	cd $(BUILD) && ./obscene-host

# The same run, rendered in colour and grouped by section.
pretty: host tool
	-cd $(BUILD) && ./obscene-host > current.txt
	@$(TOOL) pretty $(BUILD)/current.txt

# What CI runs: both shapes build, the tooling passes its own tests, and the host
# harness behaves.
check: module payload host
	@echo "--- tooling tests ---"
	@cd tool && CARGO_TARGET_DIR=$(TOOL_TARGET) cargo test --quiet
	@echo "--- host harness ---"
	-cd $(BUILD) && ./obscene-host > host-report.txt
	@$(TOOL) verify $(BUILD)/host-report.txt
	@echo "--- target object ---"
	@$(TOOL) imports $(BUILD)/obscene.module.elf

# Compares the current run against a saved one. This is the loop the report format
# exists for: change something, re-run, ask whether it helped.
#
#   make host && ./build/obscene-host > baseline.txt
#   ...change the emulator...
#   make diff BASELINE=baseline.txt
BASELINE ?= $(BUILD)/baseline.txt
diff: host tool
	-cd $(BUILD) && ./obscene-host > current.txt
	@$(TOOL) diff $(BASELINE) $(BUILD)/current.txt

clean:
	rm -rf $(BUILD)

# Header dependencies recorded by -MMD, so a header edit rebuilds the objects that use it.
# Absent on the first build; -include ignores what is not there yet.
-include $(HOST_OBJ:.o=.d) $(MODULE_OBJ:.o=.d) $(EBOOT_OBJ:.o=.d)


