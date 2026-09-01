# Decision log

Append-only. Every decision that shaped this codebase, with its reasoning, so it can
survive a context compaction and so nobody re-litigates a settled question from first
principles six weeks later.

**Read this file at the start of any working session.**

> **On the Python tools named in early entries.** Entries before D026 refer to `nid.py`,
> `verify.py`, `pretty.py` and `tools/test-tools.sh`. Those are gone - D026 records
> replacing them with the Rust `obscene-tool`, whose subcommands carry the same names.
>
> The references are left as written, because this file is a dated record of what was
> decided when, and editing a decision to match the present destroys the only thing it is
> for. Live documentation points at the current tool; this file points at what existed at
> the time.
