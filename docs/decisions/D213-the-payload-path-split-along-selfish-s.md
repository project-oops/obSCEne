# D213 - The payload path split along selfish's charter: format knowledge in selfish, runtime in obSCEne - and multi-library resolution works


The crt0 was briefly in selfish. selfish's charter forbids it: *"a `crt` ... anything that
executes on the console - do not [belong here], and neither does a convenience layer for
writing homebrew."* Correcting that produced a cleaner split and closed the one real
duplication.

### Where each piece landed

| piece | home | why |
|---|---|---|
| `payload/crt0.c` | **obSCEne** | it runs on the console - runtime, not format knowledge |
| `tool/examples/gen_payload_table.rs` | **obSCEne** | build orchestration; the vaddrs it emits are per-firmware measurements |
| `Sections::dynamic_symbols` | **selfish** | reading `.dynsym` is a format primitive, and it was a real gap |

The generator had hand-rolled a `.dynsym` parse because selfish's `symbols()` read only
`.symtab`. That was the duplication. selfish now has `dynamic_symbols()` beside `symbols()`
(shared `symbols_of(kind)`), and the generator calls it - so obSCEne consumes the primitive
rather than reimplementing it. The per-firmware vaddrs stay in obSCEne, exactly as selfish's
own "corpus is a measurement, not a format" ruling requires.

My earlier selfish `D047` entry (arguing a mission expansion to hold the crt0) collided with
the real `D047` (signing) and contradicted the charter; both it and its WORKLOG note were
reverted. selfish keeps only the `dynamic_symbols` primitive.

### Multi-library resolution, validated on hardware

The crt0's `resolve_lib_base` - `sceKernelLoadStartModule` then `sceKernelGetModuleInfo` for a
non-libkernel base - was written but untested. A body calling `strlen` from
`libSceLibcInternal` (library 1) resolved it and returned **12** for a twelve-character string,
clean exit. So the crt0 handles the general case: libkernel from getpid, every other library
loaded and based at runtime.

That unblocks both remaining fronts. The **full suite** imports across many libraries and now
has a resolution path for all of them. **Rendering** resolves `libSceVideoOut` the same way -
load it, base it, resolve by vaddr - so the display API is reachable from a payload; what
remains for rendering is the foreground context, not the resolution.

Status: **hardware** - the split built and ran, and cross-library resolution returned a correct
value on the console, 2026-08-27.

