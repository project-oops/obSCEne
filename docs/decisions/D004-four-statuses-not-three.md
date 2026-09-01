# D004 - Four statuses, not three

**assumed** · 2026-08-19

`pass`, `partial`, `fail` and additionally `skip`.

Red/amber/green was the request. The fourth exists because a failed prerequisite
otherwise cascades: if allocation fails, every check needing memory "fails" too, and
a report of forty reds stops naming the one thing actually broken. `skip` says
nothing was learned, which is different from saying something is wrong.

`partial` earns its place separately: an implementation returning zero for everything
would look perfect without it.

