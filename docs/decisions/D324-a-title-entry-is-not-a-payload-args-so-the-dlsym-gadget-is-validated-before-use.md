# D324 - a title entry is not a payload_args, so the dlsym gadget is validated before use

**assumed** - 2026-09-07

The native title (pkg and eboot) faulted at `rip = 0x2` before a single record, on every
delivery, from `e251612` onward. It was a regression: the Sep-4 `run-native-title.txt` and
`run-ps4-pkg.txt` are complete reports, built from `24036b6`.

## What faults

The crash backtrace lands in `obscene_start` immediately after `call sys_call_init`, `rdi = 1`,
`rsi` pointing at the string `"getpid"`, `rip = 0x2`. That is `sys_call_init` calling
`pargs->sys_dynlib_dlsym(1, "getpid", &out)` through a dlsym gadget whose value is `0x2`.

`e251612` added this to the entry, for the payload / injector path:

```c
const payload_args_t *pargs_init = obs_get_payload_args();
if (pargs_init != NULL) {
    sys_call_init(pargs_init);
}
```

`24036b6` had no `sys_call_init` call at all, which is exactly why Sep 4 ran.

## Why `pargs_init` is non-null on a title, and garbage

A payload is entered by elfldr with `rdi` pointing at a real `payload_args`. A **title** is
entered by the system loader with `rdi` pointing at the loader's *own* handoff struct - and it
is real, mapped, aligned memory, so the shape check in `obs_capture_payload_args` (non-null,
canonical, 8-aligned) passes and the struct is copied. Read as a `payload_args`, its first word
- `sys_dynlib_dlsym` - is a small non-pointer; `0x2` was measured. `obs_bootstrap_payload_output`
already rejects this (its own `word0 < 0x10000` guard), so it no-ops correctly; but
`obs_capture_payload_args` marked the copy valid regardless, and `obs_get_payload_args` then
handed it to every payload-gated path. Most only *read* it (`pargs->kexport_table != NULL`), so
the fault stayed latent until a caller *called through* it.

`kernelprobe.c`'s own header states the invariant this broke: *"It calls no function pointer out
of the struct and issues no primitive. Nothing here can take a machine down."* True of this
file; `sys_call_init`, a new caller in another file, is what issued the primitive.

## The fix, and why here

`obs_capture_payload_args` now sets `s_have_payload_args` only when
`obs_address_is_callable(sys_dynlib_dlsym)` - the one field anything dereferences. A title's
`0x2` (below `OBS_LOWEST_CALLABLE`, `0x1000`) is rejected, so the struct is never accepted as a
payload. `obs_get_payload_args` drops its raw-pointer fallback, which re-exposed the same
unvalidated pointer whenever the copy was refused.

This is the right layer: **every** payload-gated path - `sys_call_init`, the kexport walk in
`start.c`, the census's kexport shortcut, `obs_relocate_payload_got` - keys off
`obs_get_payload_args()`, so validating once fixes all of them and restores the file's stated
"issues no primitive against a non-payload" guarantee at the source rather than guarding each
caller. A per-caller guard on `sys_call_init` alone would leave the next reader of a garbage
struct to find the next latent fault.

## What this corrects about D323

D323 read the same `rip = 0x2` as an unresolved-import (`GLOB_DAT`/`JUMP_SLOT`) fault in the
output sink and rebuilt the sink around dlsym-resolved pointers. That was a misread: a title's
imports resolve normally (Sep 4 ran the whole suite through them), and the fault was never in
the sink - it was `sys_call_init` on a non-payload struct, upstream of any output. The D323
changes (`obs_bootstrap_title_output`, the eboot raw-channel gating) are not wrong, but they
addressed a symptom that did not exist; the eboot gating in particular removes the raw
`sceKernelWrite` channel that Sep 4 used. Whether to keep the klog bootstrap as a belt-and-braces
second channel or revert the D323 changes wholesale is recorded for the confirming run to settle.

## Confirmation status

`assumed` until a hardware run shows the title reaching its records. The host build cannot
exercise this: it never has a `payload_args`, and `obscene_start` is target-only. The disassembly
(`call sys_call_init` at the fault's return address, the `"getpid"` operand) and the commit diff
(`sys_call_init` present in `e251612`, absent in `24036b6`) are the evidence the fix targets the
real cause; the run and its klog belong in the worklog beside this entry.
