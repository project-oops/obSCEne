# 2026-08-31 - TSC-frequency band + more export candidates (D274)


The hardware-vs-orbistoun diff showed `sceKernelGetTscFrequency` "refuted" in 139-exports, and the
cause was ours: the check did an exact `== 0x5f259b8e` where the console answered `0x5f259bdd` (~79
Hz of boot-calibration drift). Replaced the equality with a tight band (`0x5f259000..0x5f25a000`) -
strong enough to be a signature, loose enough to survive recalibration. Added four more export
candidates (getsid, sceKernelReadTsc, sceKernelGetProcessTimeCounter,
sceKernelGetProcessTimeCounterFrequency) so the next payload run confirms more libkernel offsets for
orbistoun's firmware table to consume. Reached by base+vaddr, so no platform.h/imports.c churn.
host + check green.

Surprise worth keeping: the struct-layout data orbistoun most needs is largely already here.
`130-layout/system-software-version` already dumps the SwVersion bytes, and the last hardware run
shows the function writes the string at offset 8 and the version int at 0x24 but leaves offset 0
(the caller's size) untouched - which is a correction for orbistoun, whose new implementation writes
that size field. And `sceKernelGetModuleInfo` cannot be dumped on the current path at all: it
refuses in ps4-mode (`0x8002_0016`), so the module-info layout waits on a native-mode run, not a new
probe. The attribute objects (mutexattr, thread attr) are `void *` platform allocations of unknown
size, so dumping them would risk the over-read D008 exists to forbid - left alone.

