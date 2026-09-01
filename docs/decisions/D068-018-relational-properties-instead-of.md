# D068 - `018-relational`: properties instead of values, aimed where no document reaches


Status: decided, on external criticism, and the most useful thing to come out of it.

`007-responsive` compares two calls **to each other** rather than to an expected value.
That is oracle-free testing, and it is the escape from having no hardware - but it was
aimed at 54 libc and maths symbols, which is exactly where ISO C already supplies a free
oracle and the technique is needed least.

The vendor surface has no oracle at all, and relations are still cheap there:

- two live event flags must not share a handle
- create and delete repeatedly must keep working - a leaking handle table fails
- memory released must be allocatable again at the same size
- a counting semaphore must refuse a third claim against two signals
- a thread asked twice for its identity must answer the same thing

**None of these needs a struct layout or a documented error code**, which is what makes
them available now: BACKLOG §2 records struct-taking functions as blocked on layouts and
§5 records the suite as leaning negative, and a relation routes around both rather than
waiting on them.

They are `spec` rather than `assumed`. No document states these particular returns, but
"two live objects do not share one handle" is not this project's opinion.

**Validated on the host**, which meant giving the host build small correct implementations
of event flags and semaphores rather than constants. A check that has never passed a
working implementation is not evidence, and constants would have left the whole section
unexercised.

