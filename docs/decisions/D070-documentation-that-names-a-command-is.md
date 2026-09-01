# D070 - Documentation that names a command is checked against the commands that exist


Status: decided.

`make target` was named in the README, in the Makefile's own header, in a decision record
and in CI. There has never been a `target` rule. Prose naming a command that does not
exist fails only when somebody types it, and nobody had.

`scripts/doccheck.py` resolves every `make X`, `scripts/X`, `obscene-tool X` and `src/...`
path named inside backticks or a fenced block. In `verify.sh` and in CI.

**Two things it deliberately does not do.** `DECISIONS.md` and `WORKLOG.md` are exempt,
because they are dated records and the tools they name existed when they were written -
editing a decision to match the present destroys the only thing a decision log is for.
And it only reads code spans: matching bare prose turned "make the comment true" into a
missing build rule, and a checker that cries wolf gets switched off, which would leave the
real `make target` exactly as it was.

