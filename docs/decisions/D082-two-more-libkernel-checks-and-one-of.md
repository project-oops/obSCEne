# D082 - Two more libkernel checks, and one of them found a real fault


Status: decided, from `reports/gap-checkable.txt`.

**`010-kernel/is-stack`** asks whether an address is on the stack, for a local and for a
static, and requires the two answers to differ. No magic value is asserted - the platform
may use any non-zero convention for yes - and a function answering the same for both is not
reading its argument.

Under shadPS4 it fails: **both return 0**. Not a stub - shadPS4 implements the function and
delegates to its memory manager - so its virtual memory does not classify the guest's own
stack as a stack, and a title asking that question about a genuine stack address gets no.

**`010-kernel/thread-attributes`** sets a detach state and reads it back, requiring the
value that was set. It never needs to know which constant means detached: it writes one and
asks for the same one. An implementation storing nothing returns its initial value and
fails. It passes.

**`sceKernelGetCompiledSdkVersion` was deliberately left out.** The toolchain header
declares it with no arguments and a void return, which is the toolchain saying it does not
know the signature either. Calling it on that basis is what D008 forbids, and the gap
analysis listing it as available does not change that.

