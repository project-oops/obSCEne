# 2. Struct-taking functions are absent from every behavioural check


`sceVideoOutRegisterBuffers`, `scePadReadState`, the whole submission path. Each needs
a struct layout, and D008 refuses to guess one - a wrong layout corrupts the stack and
crashes nowhere near the mistake.

This is the ceiling on depth. The suite cannot check that a frame was presented or a
button was read until it can build the structures those calls take.

**What would close it:** confirmed layouts, one at a time. Each unlocks several
positive checks, which is the scarce kind (D007).

