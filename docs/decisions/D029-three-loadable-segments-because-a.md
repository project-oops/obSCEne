# D029 - Three loadable segments, because a fourth is refused


Status: decided, on evidence.

The first linker script gave the ELF header and the link tables a read-only segment of
their own, which was the tidy layout: four `PT_LOAD` segments, each with one job. A
loader accepted three and refused the fourth outright -

    Attempting to add too many segments!

The script now puts `FILEHDR PHDRS` and the link tables at the front of the text
segment, which is the layout vendor modules have, and the refusal is gone.

Worth recording because the tidy version was not obviously wrong and nothing in the
file format says otherwise. The constraint is in the loader.

