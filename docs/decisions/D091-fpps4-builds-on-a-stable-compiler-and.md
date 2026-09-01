# D091 - fpPS4 builds on a stable compiler, and reads our module perfectly


Status: a fourth loader running, and the strongest corroboration the format work has.

Its README asks for Free Pascal 3.3.1 trunk built via fpcupdeluxe. **3.2.2 from a package
manager works**, which turns a source build of a compiler into two installs. At runtime it
needs FFmpeg 4.x *shared* libraries - `avutil-56`, where current releases ship `avutil-59`,
so no package manager has them - and the project's own CI names the archive it uses. Using
what a project uses beats finding an equivalent.

**It parses every `DT_SCE_*` tag in our module correctly**: `NEEDED_MODULE`, `IMPORT_LIB`
and `IMPORT_LIB_ATTR` for all sixteen libraries, with attributes, then maps the segments,
spawns a thread and reaches the entry point.

That matters more than a report would. fpPS4 is a wholly independent implementation in
another language, and its agreement about the vendor segment is better evidence that the
module format is right than any number of runs under the loader the format was debugged
against.

**And then no guest output at all** - identically at 100 and 180 seconds, so it stops
rather than crawls. Its `sceKernelWrite` calls a real `_sys_write`, so descriptor 1 ought to
reach the captured stdout. Whether the module faults at entry or the write lands elsewhere
is **not established**, and it is recorded that way. An earlier version of this project
would have guessed; the guess would have been "stack alignment", which is now a check and
would answer it if the module got far enough to run one.

