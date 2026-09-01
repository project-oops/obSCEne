# D220 - The bundled modules are built here, never copied


With the process parameters supplied the eboot loads four platform libraries and then:

```text
# exception: 0xa0020102 (PRX_SCE_MODULE_LOAD_ERROR)
# === Lack of a .prx file in /app0/sce_module is detected!!! ===
```

A title ships its own modules there. A real package carries two, and **those are vendor modules
which this project does not redistribute**, so what goes in the directory is built by this
toolchain out of `src/sce_module.c` - a stub that loads, returns success and does nothing else.

### Are we then probing our own stubs?

**No, and it is now a build error rather than a promise.** Resolution is by the library name
written into the eboot's tables. obSCEne names `libkernel`, `libSceLibcInternal`, `libScePosix`
and nine others - every one a system library, loaded from the system path, as the crash dumps
show. It names `libc` nowhere and `libSceFios2` nowhere; its string and math calls go to
`libSceLibcInternal`.

That is an observation about today's manifest, not a property, and the failure it would become
is the worst kind this project has: an import resolving against a stub shipped in its own
package would report that a platform function exists and returns zero. The probe would measure
itself and say nothing was wrong. So `sce-module-guard` fails the build if a bundled name ever
appears in `src/imports.c`.

The names match a real package's because the system loads that directory by name - an
honestly-named file it does not look for satisfies nothing.

