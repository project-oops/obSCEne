# D239 - A run-time module load does not repair an import the loader left unresolved


*status: measured*

Twenty-four checks skip because their symbol is null. The repair depends entirely on one
question, and the two answers are very different sizes: if loading the library at run time
makes the outstanding import bind, the fix is to load early and every skipped section works.
If it does not, each check has to resolve and call through a pointer.

`060-module/runtime-load-binds-imports` asks it directly. On hardware:

```text
OBS|res|060-module/runtime-load-binds-imports|fail||loading the library at run time does not
bind an import the loader left unresolved; a check can only reach it through a resolved pointer
```

`libScePad` loaded, `sceKernelDlsym` returned `scePadOpen`, and `&scePadOpen` was still null in
the same process moments later. **Loading and binding are separate, and doing the first does
not do the second.**

The check was written before anything was built on either answer, which is the only useful
time to ask a question like this.

