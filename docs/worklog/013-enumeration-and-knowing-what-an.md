# Enumeration, and knowing what an expectation is worth


Two pieces of the foundation, in the order that avoids rework.

**`110-modules` asks the platform what it has** rather than testing a list this program
carries (D043). The first result against an emulator is exactly the kind of finding the
suite exists for:

    110-modules/list    pass  0x1          one module loaded
    110-modules/names   fail  0x80020016   EINVAL - it will not describe it

No user report, no reading anyone's source. On hardware the same check becomes the
authoritative inventory, and the difference between the two lists is the gap.

The section refuses to guess. When the assumed layout finds no name it reports the front
of the structure word by word, so an offset gets derived from what a platform actually
wrote - the same argument that fixed the swapped dynamic tags.

**Every check now says where its expectation came from** (D044). 51 assumed, 30 spec,
**0 hardware** - and `pretty` says so, with the caveat that an `[assumed]` failure may be
this suite's belief rather than a bug. Done at 68 checks because the cost of retrofitting
it rises with every one added.

### A trap, caught twice

`verify.sh` rebuilt the module without the exclusion list, and both `build-all.sh` and
`make check` do the same - so a verify run quietly handed the next step a module that
walked straight into a known crash, twice, costing 400 records each time. The sweep
rebuild is now the last thing verify does, and says so.

