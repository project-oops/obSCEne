# D086 - Guest output may arrive on stderr, and reading only stdout would have looked like silence


Status: fixed, before it could cost a run.

`run-emulator.sh` read the emulator's standard output and ignored its error stream. The
orbistoun side put guest bytes on **stderr** deliberately: it speaks a newline-delimited
protocol over its own stdout, and guest output interleaved into that would corrupt the
protocol permanently, since a half-finished line breaks the reader for good.

Reading one stream would have produced a zero-record run - indistinguishable from "the
module never reported" - with the entire report sitting in a file two lines away. That is
the most misleading failure this script could have, and it would have been diagnosed as an
obSCEne problem.

Both streams are now read. Which stream a loader uses is its business.

**And a lesson about diagnosis:** immediately after the change a run reported zero records
and the change looked responsible. It was not - the module in the build VM was still marked
for the current console generation from an earlier experiment, and the previous-generation
emulator was refusing it. Tracing the script showed 792 records passing through it
correctly. Assuming the last edit caused the symptom would have unpicked a correct fix.

