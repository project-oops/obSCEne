# D031 - The module is built with hidden visibility and symbolic binding


Status: decided, on evidence.

Built shared and position-independent, a call to one of this program's own functions
goes through the procedure linkage table. That leaves a relocation, and the loader
tries to resolve it the only way it knows - by NID, against the libraries the module
imports from. Our own names are not NIDs and are in none of those libraries:

    Resolve: Not Resolved obs_write
    lambda: Function not patched! obs_write

The slot is left null and the guest calls address zero the first time it tries to write
its own output. The report never appears, and the crash is nowhere near the cause.

This module exports nothing, so nothing needs to be visible. `-fvisibility=hidden`
turns those into direct calls; `-Wl,-Bsymbolic` binds anything still global to the
definition here.

Worth recording because the failure looked like a loader problem for some time. It was
ours, and it was a linker flag.

