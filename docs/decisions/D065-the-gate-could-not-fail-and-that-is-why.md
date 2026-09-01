# D065 - The gate could not fail, and that is why everything else survived


Status: bug, and the root cause of most of an external review.

`scripts/verify.sh` - "everything that has to pass before a change is done" - always
exited 0. Four independent ways, each reasonable-looking on its own:

```sh
(cd tool && cargo test --quiet 2>&1 | tail -2)   # status is tail's
sh scripts/build-all.sh ... || true               # swallowed twice
make check ... && echo ok || echo FAILED          # prints FAILED, returns 0
sh scripts/sweep-build.sh 2>&1 | head -1          # status is head's
```

`scripts/lint.sh` was worse: clippy's status was discarded by a pipe, and if clippy failed
to *run* the grep matched nothing, the `||` fired, and it printed `clean`. A tree nobody
had linted looked identical to a clean one - which the file's own comment says to avoid.

**Both are now written so no filter sits between a command and its status**, and every
stage's result is checked. `cargo fmt --check` was added because CI ran it and nothing
local did, so the tree had drifted out of format and stayed there.

The general rule: **a gate that cannot fail is worse than no gate**, because it is
reported as evidence. Every "verify.sh green" in this project's log up to this point meant
only that the script reached the end.

