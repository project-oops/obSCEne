# D095 - Loaders that share a codebase count once. The compatibility table records lineage


Status: assumed.

SharpEMU was added to the roster expecting an independent current-generation data point,
and it is not one: its loader and craziiEmu's differ by 93 lines in 2906, and craziiEmu's
copyright header names SharpEmu first. They fail obSCEne in the same place for the same
reason, which reads as corroboration and is not.

The rule: **before a result from a new loader is treated as independent, its loader is
diffed against the ones already in the table.** Cheap - one `diff -w` - and the failure it
prevents is the expensive kind, where a shared bug looks like a property of the format.

This is the third instance of one shape of error in this project: counting a skip as an
opinion (consensus), counting an absence as a disagreement (compat.py), and now counting a
copy as a witness. In each case something that carried no information was summed as though
it did. Worth naming, because the next one will not look like either.

