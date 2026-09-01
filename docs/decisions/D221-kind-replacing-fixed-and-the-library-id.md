# D221 - `--kind`, replacing `--fixed`, and the library id base that goes with it


Three `e_type` values and the flag was a bool. The third was not foreseen: `0xFE18` for a
library a title bundles, alongside `0xFE10` for a module a loader maps and `0xFE00` for an
eboot. Each wrong value is refused **by name**, which is the only reason they are knowable.

It is not only `e_type`. A shared library declares an export library, which takes library id
zero and pushes its imports up by one; an executable declares none, so its first import library
*is* zero (D217). The manifest cannot know which is being built, so `Imports::parse` takes it,
and `mkmodule` derives it from the same `--kind` that decides the `e_type`. Getting it wrong is
not diagnosable from the output: either two libraries claim id zero, or nothing does.

