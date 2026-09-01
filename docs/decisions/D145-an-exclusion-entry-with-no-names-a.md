# D145 - An exclusion entry with no `/` names a whole section, because a loader can fail a layer rather than a check


The exclusion list held check ids and nothing else, and the fpPS4 sweep showed what that
costs. It found `035-libc/strchr` hung, then `snprintf`, then `qsort`, `bsearch`, `numeric`,
`strtoul-bases` - twenty consecutive checks in one section, each costing two runs to
establish, ending in a report whose libc section is twenty separate skips.

Twenty skips invite a reader to look for twenty causes. There is one: **this loader does not
return from `libSceLibcInternal`.** One entry says that, and says it more accurately than the
list of checks that happened to be reached before the round budget ran out.

`EXCLUDE="035-libc"` now matches any id beginning with `035-libc/`. The slash is required
rather than a bare prefix test, so a section name cannot swallow a longer one that starts the
same way.

**It is a claim, so it is not made automatically.** The obvious next step - have `sweep.sh`
collapse to a section after N consecutive exclusions in it - is deliberately not taken. The
skip reason reads "known to end the process on this platform", and for a section entry that
asserts it of every check in the section, including any the sweep never reached. Against
fpPS4 that would be a claim about ten `035-libc` checks nothing has run.

`017-posix` is the case that proves the caution warranted: exactly five of its checks hang and
the sweep walked past the rest into the next section, so collapsing it would have reported
working checks as fatal. A section entry is right when a section fails and wrong when part of
one does, and only the completed walk tells them apart.

