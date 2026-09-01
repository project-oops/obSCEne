# Mining in Rust, and four faults the diff exposed


`obscene-tool mine` replaces `mine-nids.py`. Verified the only way a miner can be: run it,
diff both corpus files against the Python's, and drive the difference to **zero** - 166,956
names and 1,130,742 unnamed identifiers, byte for byte. Getting there took four corrections,
and every one was a real fault rather than a formatting difference.

**A whole source was missing.** `ps4libdoc/known_names.txt` is an eighth input the SOURCES
and FLAT tables do not mention - a list of names with no identifiers beside them, which is
still worth having because obSCEne hashes the name itself. Ported by reading the Python
rather than by trusting its own table of sources.

**Identifiers are normalised on record, and the Python's docstring denies it.** Two sources
publish hex and four publish the encoded form; the file has always held hex. The docstring
says "Both are recorded as found and neither is converted here" one screen above the call to
the converter. Comparing the notations as written is what once made 5,094 symbols look
disputed.

**`read_to_string` fails on invalid UTF-8, and two shadPS4 files carry it.** Skipping a file
that fails to decode cost fifty-eight real rows from `np_manager.cpp` alone, and the only
symptom was one source missing from a column nobody diffs. The Python read with
`errors='replace'`; the Rust now reads bytes and converts lossily. A stray byte in a comment
must not cost the table.

**The fifth argument of `LIB_FUNCTION` is the implementation, not the export.** Two shapes
prove it: `Libraries::Net::sys_connect` is bound to an export claiming `XVL8So3QJUk` while
`sys_connect` hashes to `L3GvCsVw1ZQ`, and `ORBIS(posix_pthread_attr_destroy)` unwraps to a
name hashing to `7w+8HkdPgUQ` against a claimed `62KCwEMmzcM`. Recording either would put a
symbol in the corpus that resolves to nothing. The Python dropped both **by accident** - its
name pattern was `\w+`, which happens to match neither. The rule is now stated, so a new
shape gets recognised rather than silently captured.

`serde_json` is a new dependency, and the only one. The firmware descriptions are JSON, where
escapes, nesting and encodings are exactly where a hand-rolled reader is quietly wrong - the
same reasoning already written down for `sha1`. Every other format is parsed by hand, because
those are line-oriented tables where splitting is simpler to read and easier to test than a
pattern.

Also ported: `shaders` (50 kernels, 2 diff lines, both attribution) and `gpusurface`, whose
sixty-row classification table moved to `data/gpu-surface.tsv` rather than being retyped -
and the one thing I did retype, the tri-state "on RDNA2" column, I got wrong immediately:
`'1' if value else '0'` promoted the string `'?'` to `yes` on six operations, because `'?'`
is truthy. Caught by the diff. The table is data now, and the export that moved it is
written down beside it.

