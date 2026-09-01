# D229 - The census resolves at run time, so an unloadable library is a finding


D228 stopped the eboot linking the census and left it reporting a skip. That removed the crash
and the measurement together, which is half a change. This is the other half.

With `OBS_CENSUS_LINKED=0` the table rows keep their **names** and lose only their addresses -
the address is the part that imports anything. `census` then:

1. `sceKernelLoadStartModule` the library, trying a short list of path forms;
2. `sceKernelDlsym` each name through the handle;
3. reports each result exactly as before.

Same section, same checks, same `sym` records. What changes is how presence is decided, not
what is asked or how it is answered - so a report from an eboot and a report from a module are
comparable, which they would not be if this had become a different measurement.

### The new record is the point

A library that will not load returns an error from `sceKernelLoadStartModule`, and the check
reports **failed, with a reason**. Under the linked census the same condition was a console that
died inside the loader with nothing on record at all (D226). That is the whole trade: one call
that returns beats one assertion that cannot.

It also announces first, one check per library, so a library that takes the process down with it
names itself in the log - which the loader did not do for 352 at once.

### The path is discovered, not assumed

Three prefixes tried in order, the bare name first. This project cannot state from a citable
source which form a loader accepts, and the sandbox prefix is randomised per boot - a crash dump
showed `/muU0ZXGZGP/common/lib/libkernel.sprx` - so no absolute path can be written down here at
all. `sink.c` already answers an identical question the same way: try the candidates, report
which worked, and the guessing becomes a measurement.

`obs_module_path` builds the string by hand rather than with `snprintf`, for the reason
`host_main.c` gives about `atoi`: this program measures that function, and a runtime that
borrowed it would be reporting on itself.

### What is still unproven

That any of it survives contact. The eboot now requires 14 libraries, of which six are known
loadable by a title from a crash dump; `libSceAgc` and `libSceAgcDriver` are current-generation
graphics and this title's category is `ps4_game`, so they are the two most likely to be refused.
The first hardware attempt therefore excludes `900-surface` at run time: prove the eboot boots
and reports, then turn the census on. A staged answer beats a fast one when each attempt costs
an hour of somebody's jailbreak.

