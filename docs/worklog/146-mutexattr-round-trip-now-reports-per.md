# 2026-09-01 - mutexattr-round-trip now reports per-type read-back


`015-sync/mutexattr-round-trip` reported only the count of types that round-trip (4 of 5 on hardware) -
which cannot say WHICH type does not, the one fact an emulator needs to match it. Added a per-type measure
(`type-N-read-back`) reporting what `Gettype` reads back after `Settype(type)`, or -1 where Set refused or
Get failed - using the already-declared mutexattr functions, so no surface/import change, and only
appending measure lines (report-interface safe). `make host` builds clean (-Werror) and runs: the host
stub round-trips 0-3 and refuses 4 (res stays 4). A hardware run will now name orbistoun's target: which of
the five the console normalises or rejects. `make check` passes (EXIT 0).

