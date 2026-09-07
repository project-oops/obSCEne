# D323 - the eboot resolves its output by name, and never calls a raw import

**assumed** - 2026-09-07

> **Correction (see D324).** This entry read the title's `rip = 0x2` fault as an unresolved-import
> fault in the *output sink*. That was wrong: the real cause was `sys_call_init` calling through a
> title's loader-handoff struct (mistaken for a `payload_args`), upstream of any output, and D324
> fixes it. A title's imports resolve normally, so the premise below - that the sink was jumping
> through an unbound `JUMP_SLOT` - did not hold. The changes this entry describes
> (`obs_bootstrap_title_output`, the eboot raw-channel gating) still build and run - the confirmed
> title reaches its klog records through the resolved `s_fn_debug_out` this added - but they
> addressed a symptom that was not the fault. Kept for now as a second, dlsym-resolved output path;
> revisit whether the eboot raw-channel gating (which drops the raw `sceKernelWrite` channel Sep 4
> used) is worth keeping once D324 has settled.

Once the phantom `libkernel_sync_on_address` needed-module was removed (D322-adjacent revert,
uncommitted with this work) the native title stopped dying on load and reached its entry - and
then faulted immediately, before a single record, with `rip = 0x2` and `rdi = 1`, called from
inside the eboot's own text. That is a write-shaped call - `fn(1, buf, len)` - jumping to an
unresolved-import sentinel. This records why the fix is where it is and not where it looks like
it should be.

## What actually faults, and why the existing guard cannot see it

The eboot is compiled `-DOBSCENE_TARGET_MODULE=1`, which excludes the syscall-wrapper block in
`runtime.c` (the `#if !defined(OBSCENE_TARGET_MODULE)` region). So in the eboot - unlike the
payload - `sceKernelWrite`, `write`, `puts` and `putchar` are **pure imports**, and the sink's
`obs_send` calls them directly.

The relocations tell the rest. The sink's imports carry **two** relocations each: a
`R_X86_64_GLOB_DAT` (the data slot `&fn` reads) and a separate `R_X86_64_JUMP_SLOT` (the
linkage slot the *call* goes through). Measured on the built eboot:

```
0x344820 GLOB_DAT  sceKernelDebugOutText     0x344b50 JUMP_SLOT sceKernelDebugOutText
0x344828 GLOB_DAT  sceKernelWrite            0x344b58 JUMP_SLOT sceKernelWrite
0x344830 GLOB_DAT  puts                      0x344b60 JUMP_SLOT puts
0x344838 GLOB_DAT  write        (no JUMP_SLOT - call and guard share the data slot)
```

`obs_address_is_callable((const void *)&fn)` reads the GLOB_DAT slot. The call goes through the
JUMP_SLOT. A loader that binds the data slot but leaves the linkage slot at its unresolved
sentinel - `0x2` on this platform - passes the guard on the GLOB_DAT and faults on the call.
The one-symbol harness guard has PLT-decoding logic for exactly this (it follows the stub's own
`ff 25` indirection to the real target), but its bounds are established in
`obs_relocate_payload_got`, which runs inside `obs_bind_dynamic_symbols` - **after** the first
boot note. The first write happens with the guard disarmed, and the first write is where it
died.

This was a regression, not a permanent state: the Sep 4 `run-native-title.txt` and
`run-ps4-pkg.txt` are complete reports, delivered over klog (`OBS|sink|none`). So dlsym and the
system-log channel demonstrably work on this console; something between then and now left an
import the sink calls at `0x2`.

## The fix: resolve by name, call through a data pointer, and on the eboot never call a raw import

Two parts.

**Resolve the output entry points before the first write.** A payload bootstraps `s_fn_write`
and `s_fn_debug_out` from its dlsym gadget; a title has no payload args, so
`obs_bootstrap_title_output` resolves the same two by name through the loader's own
`sceKernelDlsym` (`obs_module_symbol`), exactly as every section resolves a platform call, and
`start.c` calls it before `obs_boot_note`. A resolved pointer is a plain datum the sink
null-checks and calls through - it is not a linkage slot, so the GLOB_DAT/JUMP_SLOT split cannot
reach it. This is min.c's arrangement (an imported `sceKernelWrite`, proven on hardware)
generalised to the full probe.

**On the eboot, the raw-import channels are off unconditionally.** `OBS_RAW_IMPORT_CHANNELS_OK()`
is `0` under `-DOBSCENE_TARGET_EBOOT` (added to `EBOOT_CFLAGS`) and `!obs_module_resolution_works()`
otherwise. A module keeps the raw channels, because an emulator that stubs dlsym but binds
direct imports has no other way out; an eboot never uses them, because on a console a raw call is
the fault above and where dlsym cannot resolve the pointers the honest outcome is no output, not
a jump to `0x2`.

## Why not the alternatives

- **Gate the raw channels on `!obs_module_resolution_works()` alone** (no eboot special-case).
  That treats "resolution fails" as "emulator with real direct imports" - but on a console where
  resolution is broken it is *also* false, and the raw call faults again. The eboot case has to
  be unconditional.
- **Arm the PLT bounds early so the existing guard decodes the JUMP_SLOT.** It would make the
  guard correct, but it needs `obs_relocate_payload_got`'s base/offset arithmetic to be right for
  a fixed-address eboot, and that path has only ever run on the payload - it has never executed
  on the eboot, because the crash precedes it. Resolving by name needs no relocation arithmetic
  at all.
- **Pre-apply / bake relocations at package time (the reloc-count hypothesis).** Ruled out
  upstream of this: min.c (5 relocations) and the full eboot (40k) both reach `EXEC`, and the
  Sep 4 full reports prove the loader binds the eboot's relocations. The fault is one unresolved
  linkage slot the sink calls too early, not the relocation table's scale.

## What the host build settles, and what it cannot

`make check` (host harness + gates) passes with the change, and host/module/eboot/pkg all build
clean under `-Werror -Wconversion`. But the host build compiles none of this: `obs_send`,
`obs_bootstrap_title_output` and the macro are all inside the `!OBSCENE_HOST_BUILD` block. The
host cannot exercise a GLOB_DAT/JUMP_SLOT split or a `0x2` slot, so it proves the code compiles
and the harness is unbroken - not that the title now reports. That is why this is `assumed`
until a hardware run shows the eboot's records on the system log; the confirming run and its
klog belong in the worklog beside this entry.
