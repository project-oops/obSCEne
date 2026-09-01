# D230 - The generation probe asked by linking, and could have killed what it was probing


`obs_current_present` read `&sceAgcCreateShader != NULL`. Taking that address imports the name,
which puts `libSceAgcDriver` in `DT_NEEDED`, which makes a system loader load it **before this
program runs**.

The two AGC libraries are current-generation graphics and a package installs under a
`ps4_game` category, so of the fourteen an eboot required they were the two a title was least
likely to be given - and a `DT_NEEDED` a title cannot meet is a console that dies with nothing
on record (D226). The check that answers "which console is this?" could have taken the console
down before it could answer.

It asks at run time now, through `obs_module_open` and `obs_module_symbol`. Same two markers,
same meaning, and the driver either loads or does not - either way there is a value to report.
The eboot requires **12** libraries, every one of them a library an ordinary previous-generation
title links.

### The helper moved to the harness, which is where it belonged

`obs_module_open` and `obs_module_symbol` started in `surface.c` and are now in `harness.c`
beside `obs_address_is_callable`, because they answer the same kind of question one layer up.
`obs_address_is_callable` guards *calling a function that may be absent*; these guard *needing a
library that may be absent*, which is the layer D226 showed had no guard at all. A second caller
appearing within the hour is the usual sign a thing was in the wrong place.

`harness.c` includes `platform.h` for the first time as a result. The harness runs checks and
the checks called the platform; asking whether a library is there is not any one check's
business, so the two calls that make it possible live with the harness.

