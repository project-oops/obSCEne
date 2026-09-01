# D203 - `105-record`, not `170-record`: the section numbers carry the layering and the report contract enforces it


The section was first numbered 170 and registered between `110-modules` and `120-measure`,
because the argument for its position is a dependency one - it drives the encoder behind the
same output `080-video` acquires, so a reader wants to know whether that output works first.

`make check` rejected the report: *sections are out of order*. The registry is an explicit
list and will hold any order somebody writes; the contract requires the identifiers to ascend,
and the numbers are what carry the layering rather than the list position.

Renumbering to 105 satisfies both - it sits in the presentation group after `100-input`, which
is where the dependency argument already said it belonged. The two constraints were never in
tension; only the number was wrong.

Renaming check identifiers is normally forbidden (principle 3: they are the key when diffing
runs). These had never run anywhere, so there was no history to break - and doing it now is
what stops there being any later.

