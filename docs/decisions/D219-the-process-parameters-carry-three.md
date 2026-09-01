# D219 - The process parameters carry three structures, and the libc one is not optional


`libkernel` reads this block before a single instruction of the program runs. Ours declared a
size, a magic and nothing else, and the process died before its entry point:

```text
# signal: 11 (SIGSEGV)
# reason: page fault (user write data, page not present)
# fault address: 0000000000000028
# rip: 000000080003333b            <- inside libkernel, not inside us
```

A real executable's block is `0x60` bytes, not `0x50`, and carries pointers at `+0x38`, `+0x40`
and `+0x48` to three self-describing structures - each begins with its own size, `0xa8`, `0x38`
and `0x10`. The first has a **zero at exactly `+0x28`**, a slot the platform library fills in.
Ours was a null pointer, so the same store landed on absolute `0x28`.

**The libc one was the one hoped to be optional and is not.** This program has no libc of its
own - its string and math calls go to the platform's `libSceLibcInternal`, a system library - and
declaring libc parameters is what makes the system go looking for the title's own `libc.prx`
(D220). So it was worth a run to find out: built with only that pointer nulled and the other two
in place, the process faults again at the same instruction and the same address. It is required.

The structures are otherwise zero. One of them carries an entry count, a version and two
pointers in a real executable; this program allocates nothing, so zero is the honest value for
all of it. The size fields are what make that safe - a structure that states its length can be
read forward by something that knows more fields than we do.

