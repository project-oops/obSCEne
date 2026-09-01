# No Python left


Seventeen scripts, 4,358 lines, now zero. Every one is an `obscene-tool` subcommand, 202
tests, and nothing in `scripts/`, the `Makefile` or CI invokes an interpreter. Four
languages: C is the product, GLSL is forced by the GPU probes, sh is glue and a documented
choice, Rust is everything else.

**Every single port found something.** Not a formatting difference - a fault that had been
producing a confident wrong answer:

| tool | fault | effect |
|---|---|---|
| `guards` | symbol column had to be an identifier | 8 rows never examined, incl. the blind prober |
| `guards` | runner assumed `check_*` | the blind prober's is `run_bulk` |
| `counts` | id matched `[0-9]{3}-[a-z]+/` | 2 checks uncounted: 134 claimed, 136 real |
| `counts` | `X\((\w+)\)` over the census | a macro *parameter* counted as a symbol: 39,549 vs 39,548 |
| `counts` | split on `L(` | matched inside `…LIBKERNEL(`: 382 libraries vs 372 |
| `doccheck` | `PROPOSED` never enforced | a shipped proposal stayed listed forever |
| `mine` | `read_to_string` on invalid UTF-8 | two shadPS4 files skipped, 58 rows lost |
| `mine` | eighth source undocumented | `ps4libdoc/known_names.txt` missing entirely |
| `mine` | docstring denied its own converter | identifiers normalise on record; the prose said otherwise |
| `surface` | ten names in two mutually-exclusive lists | **the generator refused its own definition and could not run** |
| `surface` | a note hand-added to the output | a working generator would have deleted it |

The `surface` one is the sharpest. It was never gated, so nobody knew it had stopped working;
a generator whose output is committed and whose input nobody runs has already diverged, you
just cannot see it yet. Everything generated is gated now.

**Four data files came out of the code**: `surface.txt`, `gpu-surface.tsv`, `font.txt`, and
the corpus files that were already there. Each was moved by exporting from the Python rather
than retyped, and the one column I did retype - the tri-state "on RDNA2" - I got wrong
immediately, because `'?'` is truthy in Python and became `yes` on six operations. That is
the argument for relocating data rather than transcribing it, made concrete twice.

**Verification was always the same shape**: run both, diff the output, drive the difference
to zero. `surface.h`, `corpus.h`, `nids.h`, `font.c`, `gpu_shaders.gen.h`, the compatibility
table, both corpus files, and the `ps5-gap` report all reproduce byte-for-byte; the decision
index and the GPU census differ only in the attribution line I deliberately changed. Where
that was not possible - the protocol checker - both versions were fed the same thirteen
corruptions and both caught all thirteen. Agreement on clean input proves nothing.

Two dependencies were added, both for the same stated reason as `sha1`: `serde_json`, because
the firmware descriptions are JSON and escapes and nesting are where a hand-rolled reader is
quietly wrong. Everything else is parsed by hand, because line-oriented tables split more
simply than they match.

And a recurring lesson worth naming: **whitespace nobody can see is still content.** A blank
line between a note and its macro; a trailing newline that split a macro across two lines; a
block separator indistinguishable from a blank line the prose wanted; nine spaces of source
indentation that made twelve header lines parse as symbol names; and `#` as a comment marker
in a file where `#` is a lit pixel. Five separate corrections, one cause.

