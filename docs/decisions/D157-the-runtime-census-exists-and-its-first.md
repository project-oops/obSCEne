# D157 - The runtime census exists, and its first run settled what the address census could not


D149 argued that "present" meaning *a symbol address resolved* is close to worthless on a
loader that stub-resolves whatever it cannot find, and that `sceKernelLoadStartModule` asks a
question with a real answer instead. Both calls were already declared and neither had ever
been used. `110-modules/load` and `110-modules/symbol` now use them.

One run of shadPS4 makes the case better than the argument did. Same loader, same run:

| instrument | what it reported |
|---|---|
| address census | **373 of 373 symbols present** |
| module load | **`0x80020002` for all five paths** |

`0x8002_0002` is the vendor scheme established this morning - `0x8002_0000 | errno` - so that
is **ENOENT: no such module**, five times, specifically. The loader says it has no modules at
all while the census on the same run says it has everything.

Only one of those is a measurement. A resolved address is the loader's opinion and there is
no way for the guest to tell a stub from an implementation by looking at one; an error code
came from the platform being asked a question it had to answer.

### What is recorded, and what is not

Nothing states an expectation. This program does not know which modules a given firmware
ships, and a list of "should be present" would be a guess where the report should carry a
fact - the same reasoning `130-layout` and `140-oracle` rest on. Each attempt emits a
`measure` record with the code, and the verdict is only about whether the mechanism answered
at all. **Zero modules loaded is a real answer**, not a failure: a firmwareless loader has
none to give and said so five times.

`110-modules/symbol` skips when nothing loaded, because asking a handle nobody obtained is
not a question. On a console it becomes the other half - a handle says a module exists, and
resolving a name through it says the module carries that symbol, which is precisely what the
address census claims and cannot substantiate.

### Five paths, not five hundred

Every one is a name this project has already seen in a loader diagnostic from its own runs -
`libc.prx` and `libSceFios2.prx` are the two shadPS4 names when it refuses the corpus build
(D149). Naming a module is free; the cost is that a call which *starts* a module runs its
initialiser. Five known names establishes whether the instrument answers, and widening it is
a decision to make once it has - which it now has.

