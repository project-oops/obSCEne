# D175 - `e_type` was `0xFE18` - the shared-library type - and the constants were named backwards


```rust
pub const ET_SCE_EXEC_ASLR: u16 = 0xFE10;   // actually the executable type
pub const ET_SCE_DYNEXEC:   u16 = 0xFE18;   // actually the shared-library type
```

Kyty states the pair plainly:

```cpp
constexpr Elf64_Half ET_DYNEXEC = 0xfe10; // Executable file
constexpr Elf64_Half ET_DYNAMIC = 0xfe18; // Shared
```

So `mkmodule` reported "rewritten to `ET_SCE_DYNEXEC`" - which reads as *the dynamic
executable* - while writing `0xFE18`, the library type. `MODULE-FORMAT.md` then repeated the
name back as prose: *"`0xFE18` is the dynamic-executable type, chosen because that is what this
module actually is."*

**A misnamed constant made a wrong value look right, and the documentation confirmed it.**

### The comment beside it was correct the whole time

> `0xFE10` … a loader given a library maps it and waits for something to link against it,
> **which looks exactly like loading a module and declining to run it**.
>
> `0xFE18` … The vendor's dynamic-**library** type. Accepted by a loader, **and not executed**.

Someone worked this out, wrote it accurately, and the code did the opposite because it followed
the names. That is a sharper version of the failure this project keeps meeting: not an absent
check, but a correct statement sitting next to code that contradicts it.

### What it cost

A loader that respects the distinction runs a `0xFE18` module's initialisers and then looks
elsewhere for a process to start. Kyty does exactly that - `StartAllModules` only starts
*shared* modules, `GetEntry` only returns an entry from a *non*-shared one - so obSCEne loaded,
relocated, ran `DT_INIT`, printed `Execute: Main`, called nothing, and exited cleanly. A silent
no-op with no error anywhere.

The three loaders that do not distinguish ran it perfectly well, which is why this survived:
**it can only be detected by a loader that respects the type**, and until Kyty was built from
source there was none.

### The earlier choice was evidence-based and the evidence was misread

`0xFE10` was rejected because Kyty printed `Not implemented (!is_shared && !is_next_gen)`. But
`is_next_gen` is `e_ident[EI_ABIVERSION] == 2` - **that is `GEN=5`**. The test was run at
`GEN=4`, where `!is_next_gen` holds. At `GEN=5` that branch cannot fire, and the comment
recording the decision noted the real lesson without acting on it: *"it was untestable with one
emulator"*.

### Measured on every loader before adopting

| | `0xFE18` | `0xFE10` |
|---|---|---|
| shadPS4 (GEN=4) | 33,597 / ended | **36,566 / ended, zero exclusions needed** |
| PS5PCEM (GEN=5) | 36,434 / ended | 36,435 / ended |
| Kyty (from source) | loads, never enters | **enters, runs, writes report records** |

shadPS4's improvement was not expected: with the correct type it now needs **no exclusions at
all**, where it previously needed two.

