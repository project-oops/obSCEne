# 4b. setjmp / longjmp


Deliberately absent, and worth stating why because it looks like an easy win.

`setjmp` is the single most valuable control-flow check available - it saves and
restores the stack directly, which is exactly the sort of thing an emulator gets
subtly wrong and nothing else in this suite exercises. It is also the one C library
function whose argument is a struct of unknown size, and `jmp_buf` is not something to
guess at (D008). Over-allocating a "surely big enough" buffer is guessing with extra
steps.

**What would close it:** a confirmed `jmp_buf` size for this platform. One number.

