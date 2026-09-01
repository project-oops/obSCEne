# D098 - The missing program headers come from `link/module.ld`, not from the link mode or the packaging. Two hypotheses were tested and both were wrong


Status: derived - each was settled by building the thing and looking.

D097 produced a list of four program headers a standard OpenOrbis module carries and
obSCEne does not: `PT_INTERP`, `PT_TLS`, `PT_GNU_EH_FRAME`, `PT_SCE_RELRO`. Two plausible
sources were proposed for them, and neither survived contact:

**"The vendor packaging adds them."** Running obSCEne's module through `create-fself`, the
same tool the control uses, added **none** of the four. It failed at the last step -
`Failed to build FSELF: no symbol section` - which is a second finding in itself: obSCEne
emits `e_shoff 0`, `e_shnum 0`, no section header table at all, and the vendor tooling
requires one. The control keeps its sections.

**"`-pie` adds them."** The reasoning was that lld emits `PT_INTERP` for an executable
against a freebsd triple and not for a shared object. Building `LINKMODE=-pie` produced a
program header table **identical** to the `-shared` one - six entries, no interpreter.

The actual answer was in this project's own linker script the whole time. `link/module.ld`
contains an explicit `PHDRS` block, so **it** decides which segments exist; lld emits what
the script names and nothing else. The link mode cannot add a header the script does not
declare, and neither can a post-processing tool that only rewrites what it is given.

So the work is: declare `interp PT_INTERP` in `PHDRS`, place an `.interp` output section
holding the platform's interpreter path, and repeat for the other three. Small, and
knowable only because two cheaper explanations were tried first and failed visibly.

The `LINKMODE` knob added to test the second hypothesis was removed once it was answered. A
build option that changes nothing is worse than the note explaining why it changes nothing.

