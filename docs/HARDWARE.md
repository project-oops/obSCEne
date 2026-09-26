# What a real PS5 answers

Findings from running obSCEne on retail hardware, each traceable to a record in
`data/hardware/`. Every claim cites a line, so a consumer - orbistoun above all - can say how
something was determined and reach the same answer. Where a finding rests on inference rather
than a record, it says so.

## The shape of a run

The complete suite, census included, from one binary:

```text
OBS|meta|1|30|528          30 sections, 528 checks
OBS|tally|232|75|192|29    pass / partial / fail / skip
obscene: suite complete
```

Most failures are one thing - a library the census cannot load - and that is a finding, not a
fault. `data/hardware/ps5-full.txt` is a full run; `data/hardware/ps5-imports.txt` carries
`import` records. Each names the build that produced it.

## Mapping and binding are separate

The loader maps a library's address range without binding its symbols. The system log names
each library it maps, with an address range and a fingerprint:

```text
# /<sandbox>/common/lib/libScePad.sprx
#  xotext: 0000000800968000:0000000800980000 nsegs: 4
#  fingerprint: c68f5edf5ba4c424ce9468cbc64b016900000000
```

165 libraries are mapped into the process, and ten of the eboot's twelve are among them; only
`libScePosix` and `libSceVideoRecording` are not. `libScePad` is mapped, at a known address,
while its imports are null. Mapping is present without binding.

### Which is a probe defect and which is the platform's

`061-imports` puts the two axes side by side - did the import bind, and does a run-time lookup
find the same name in the same library:

```text
OBS|import|libScePad|scePadOpen|unlinked|resolvable
```

`unlinked` with `resolvable` is the repairable case: the platform has the symbol and the module
failed to ask for it correctly.

### Weak imports are left unbound

A platform function declared weak is null when absent rather than a link error. A loader treats
an undefined `STB_WEAK` import as optional and does not resolve it if resolving costs anything:

```text
ours   WEAK   FUNC   x203
real   GLOBAL FUNC   x126   GLOBAL OBJECT x13      (a launching title has no weak imports)
```

`libkernel` and `libSceLibcInternal` bind because they are already resident and resolving
against them is free. Every other library is mapped - address range and fingerprint in the log
- and left unbound. Building imports as `STB_GLOBAL` binds them.

Three fields that do not govern this were eliminated against the oracle or the console: the
import-library attribute (`0x1` where a real title writes `0x9`), `sdk_version` (which also
stops `sceKernelDlsym` answering), and `.prx` versus `.sprx` filenames (a real title
writes `.prx` too). The binding is governed by symbol binding, not by any of them. (D248)

### Six imports have no repair here

`sceKernelIsCex` is absent from a library that binds 84 other symbols, so it is simply not
offered. The other five are `libScePosix` names - `posix_getpagesize`, `posix_sigemptyset`,
`posix_usleep`, `posix_pthread_rwlock_init`, `posix_pthread_rwlock_tryrdlock`:

```text
OBS|res|900-surface/posix|fail||this library could not be loaded
```

`libScePosix` does not load at all. It is one of the two declared libraries the loader never
maps, and the census cannot reach it. Nothing in that library resolves under any name, so the
five names are untested rather than wrong.

### A run-time load does not repair an unbound import

```text
OBS|res|060-module/runtime-load-binds-imports|fail||loading the library at run time does not
bind an import the loader left unresolved
```

`libScePad` loads, `sceKernelDlsym` returns `scePadOpen`, and `&scePadOpen` stays null in the
same process. A check that needs an unbound symbol reaches it through a resolved pointer, not
through a startup load.

## What the census measures

`data/hardware/ps5-imports.txt` carries the per-symbol verdicts: symbols confirmed present,
symbols confirmed absent, and libraries fully present, partly present, or unloadable by a
title. About a third of the corpus gets a verdict; the remainder sits inside libraries a title
cannot reach at all, which bounds what an emulator implements for a title of this shape.

## Loading a media codec library kills the process

Isolated one at a time by the iterative sweep (`./bin/obscene hwsweep`), each identified by a
`try` record with no matching `res`:

```text
libSceM4aacEnc   libSceOpusCeltDec  libSceOpusCeltEnc  libSceOpusDec  libSceOpusSilkEnc
libSceSrcUtl     libSceUlt          libSceVideoCoreServerInterface
libSceVideoOutSecondary             libSceVideoRecording
```

Audio and video codecs and media-server interfaces, across the whole corpus.
`sceKernelLoadStartModule` on any of them ends the process rather than returning an error. With
these ten excluded the suite runs to the end.

## Behavioural findings

```text
018-relational/handle-fits-its-out-parameter  the call wrote past the end of the int it was given
035-libc/wide-strings                          wcslen counted the wrong number of wide characters
010-kernel/is-stack                            a stack address and a static one were reported alike
110-modules/names            0x80020016        the platform would not describe any module
070-user/initialise          0x80960003        the user service refused to initialise
080-video/open               0x80290009        the main video output would not open
130-layout/query-size-ladder                   every size accepted, so the size is not validated
```

`080-video/open` is where anything wanting a picture on screen starts: the output does not open
under these arguments, and the code is the platform's own. `is-stack` fails identically on
PS5PCEM, so it is a genuine console behaviour rather than an emulator gap.

## A `ps4_game` title cannot load current-generation libraries

```text
### ERROR: ABIVERSION mismatch. /<sandbox>/common/lib/libSceAgcDriver.sprx
[rtld] ERROR self_load_shared_object:2826: B: res 0 (libSceAgcDriver.sprx)  val 2
```

`val 2` is the library's `EI_ABIVERSION`; the eboot's is 0. A launching homebrew eboot is also
`EI_ABIVERSION 0`, measured directly from its container, so this is a category constraint rather
than a build defect: a `ps4_game` title cannot be given the current generation's graphics
libraries.

The route to those libraries is native process injection. `./bin/obscene inject` deploys
`obscene-injector.elf` into a running native retail title and runs obSCEne in
`payload/ps5-native` mode, where `libSceAgc` and native Prospero APIs execute unrestricted. See
[`docs/INJECTOR.md`](INJECTOR.md) for the runbook.

## The screen works, and the constraint is allocation alignment

![obSCEne running on a retail PS5, drawing its own report](screenshots/ps5-hardware.png)

The photograph is through a KVM rather than captured on the console: the framebuffer is
write-only from where the drawing happens, so a capture path would be a second thing that could
be wrong. obSCEne draws its report on the console's own display:

```text
OBS|display|ready|1920x1080 framebuffer|0x0
OBS|display|presenting|a submitted frame reached the display|0x0
```

`presenting` is measured rather than assumed: the frame counter moves, which is stronger than
the flip being accepted.

The scanout constraint is the framebuffer's allocation alignment, in an argument to
`sceKernelAllocateDirectMemory` rather than to anything in `libSceVideoOut`:

```text
onion,0x4000,1     0x80290015
garlic,0x4000,1    0x80290015
garlic,0x10000,1   0x0          accepted
```

`0x4000` is not coarse enough for a scanout buffer; `0x10000` is. Memory type does not matter -
onion and garlic both refuse at `0x4000`. Two codes moving is what proves an attribute is parsed
and that the baseline passes; that is what separates the parsed attribute from the allocation
argument that actually carries the fault. Both tables come from one run each of
`085-videobuf`, which varies one argument per call inside the probe.

### Codes for `libSceVideoOut`

Each is produced by a call that differs from a working one in a single known way, so each is
measured rather than looked up:

| code | produced by |
|---|---|
| `0x8029000b` | any call with an invalid handle - two negative checks pass on it |
| `0x80290015` | registering a buffer aligned to `0x4000` |
| `0x80290003` | registering with the pixel format cleared |
| `0x80290008` | registering with a different aspect ratio |
| `0x80290004` | the current generation's `sceVideoOutRegisterBuffers2` with these arguments |
| `0x80290009` | opening an output this process already holds |

`0x80290009` is a leaked-handle result: the output opens when this process is not already
holding it.

## The report exists only while the title does, and only the title can read it

**The sandbox mount is torn down when the title exits.** `/download0` appears outside the
sandbox at `/mnt/sandbox/download/<TITLEID>`; it holds `obscene-boot.txt` and
`obscene-report.txt` during a run and is gone after it. Retrieval happens during a run or through
a channel that leaves the sandbox as it goes - the system log, which is the second reason
that log is written unconditionally. Read records off the live log with
`./bin/obscene report`, or capture them across launch with `./bin/obscene deploy`. (D269)

**Persistent sinks survive title exit.** For native runs (`eboot` / `BIG_APP`) and payload runs,
the sink probe also mounts persistent storage candidates (`/mnt/usb0/obscene`,
`/data/homebrew/PPSA90000`, `/data`). When one is reachable, obSCEne writes a timestamped run
archive (`report-<timestamp>.txt`) and `report.txt` at mode `0666`, and both are pulled post-run
with `./bin/obscene pull-log`.

**The reader is never the writer.** A title writes its report as the user the platform runs
titles as; the shell server and the file-transfer server are a different user. A file created
`0600` returns `Permission denied` to them though they can see it in a listing, so the report is
written `0666`.

### `/download0` is writable from inside a title

```text
OBS|sink|/download0/obscene-report.txt
```

`/data` is not: it is reachable by an elfldr payload and not by a sandboxed title. The sink probe
tries candidates in order and names the one that worked, so this is measured rather than assumed.

## The console state a run depends on

`pldmgr` reads `autoload.txt` and loads the payload chain. With `pldmgr` absent, four of six
payloads never load, including `kstuff-lite`, which patches NPDRM for fake licences. The symptom
is that every fake-licensed package and every retail dump refuses to launch:

```text
[SceLncService] PrepareProcessLaunchDirCheck()
scePs4AppCategoryGetForTitleId return 3
CheckPrepareProcessLaunchPkgApp() ret = 80a40086
preLaunchCheck: LNC_ISOK::0x80a40086          → CE-105773-3 on screen
```

Nothing about the package is implicated. Check `ps` for `kstuff.elf` before concluding anything
about a launch failure.

## Bundled modules are looked up by name

`/app0/sce_module` must contain the modules the system looks for. A well-formed module under a
name a title never bundles produces the same complaint as an empty directory:

```text
# === Lack of a .prx file in /app0/sce_module is detected!!! ===
```

with no `rtld` line naming it, because nothing looked at it. The names that work are the ones a
real package uses - `libc.prx`, `libSceFios2.prx`. A real title also imports from `libc`, so a
stub under that name is safe only while nothing in the build imports from it; `sce-module-guard`
enforces that against the eboot's own manifest.
