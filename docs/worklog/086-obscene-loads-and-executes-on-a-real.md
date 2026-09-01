# obSCEne loads and executes on a real console


`<326> EXEC /app0/eboot.bin [user], vm#1, dmem#1 abi=ps4 category=ps4_game`, the splash screen
unloading, and `BigApp[0x2018] Changed state "RUNNING"`. Every format defect between installing
and executing was in `selfish` (its D074-D077); what was here was the library numbering (D217)
and two derivation relations that had been true for the wrong reason.

**The derivation checker earned its keep twice in one session.** Both relations it failed on
were relations that had held only while something else was in the wrong place - "STRTAB is the
start of the segment" and "HASH + HASHSZ == end of segment" - and it failed the build rather
than letting a silently-wrong layout through. Neither was found by thinking about it.

### Where it stops now, and what that is not

The process faults with `SIGSEGV`, writing to `0x28`, at `rip 0x80003333b` - inside `libkernel`,
not inside us. The file-writing variant of `min.c`, built with three imports rather than one and
a different code path, faults at **the same instruction and the same address** and leaves no
file. Identical failure from different code means our code never runs: this is `libkernel`'s
initialisation, before our entry point.

The importless build cannot be the control for that, and finding out why was worth the run: it
does not load at all, because this loader requires the full relocation tag set - `PLTGOT`,
`JMPREL`, `PLTRELSZ`, `RELA`, `RELASZ`, `RELAENT` - whether or not a module has any relocations
to describe. (D218)

The next thing to look at is the process parameter block. Ours is `0x50` bytes and zero after
the magic; a real one is `0x60`, declares an SDK version of `0x08008011` and an entry count of
five, and carries three non-null pointers into its RELRO segment at `+0x38`, `+0x40` and
`+0x48`. A write to `null + 0x28` is what taking one of those and finding zero looks like.

### Console cost

Zero crashes across roughly a dozen installs and six launches. Every failure was a refused load
or a killed process, and the jailbreak survived all of them - which is the difference between
this session and the four console-down stretches before it.

