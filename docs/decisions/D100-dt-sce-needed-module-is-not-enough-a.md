# D100 - `DT_SCE_NEEDED_MODULE` is not enough. A loader keys its implementations on the ordinary `DT_NEEDED` filename, and obSCEne emits none


Status: derived - from the loader's registration table and a string-table diff against the
control.

With the segment fault fixed (D099) obSCEne runs under fpPS4, and every import resolves to a
logging stub:

```
[main:26024] nop nid:libkernel:E304B37BDD8184B2:sceKernelWrite
```

That stub is installed at `fpPS4.lpr:388`, on the one condition `if (Result=nil)` - the
lookup found nothing. Yet fpPS4 implements the function and registers the exact NID we ask
for, at `ps4_libkernel.pas:1897`.

The registration is reached from `Load_libkernel`, and `Load_libkernel` is dispatched from:

```pascal
ps4_app.RegistredPreLoad('libkernel.prx',@Load_libkernel);
```

**Keyed on the filename, with the extension.** That name reaches the table through
`_add_need`, which is fed by `DT_NEEDED` and nothing else. A string-table diff settles it:

| | control | obSCEne |
|---|---|---|
| `libkernel` | yes | yes |
| `libkernel.prx` | **yes** | **no** |
| `libkernel.so` | yes | no |

So obSCEne declares which modules it needs in the vendor tag and never in the ordinary one.
fpPS4 creates the library object from `DT_SCE_NEEDED_MODULE` - which is why its message names
`libkernel` correctly - and then never preloads anything into it, because nothing ever asked
for `libkernel.prx`. An empty library resolves nothing, and every symbol falls through to the
stub.

The fix is in `mkmodule`: one `DT_NEEDED` per imported library, pointing at a
`<library>.prx` string in the table. Six tags today, sixteen strings.

**Why shadPS4 never needed it** is worth stating, because it is the reason this survived
this long: shadPS4 resolves from `DT_SCE_IMPORT_LIB` directly and never consults `DT_NEEDED`.
One loader's tolerance is not a format, and the only reason to think our tags were complete
was that the one loader we developed against accepted them.

