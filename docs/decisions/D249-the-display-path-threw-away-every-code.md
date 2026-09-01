# D249 - The display path threw away every code it was given


*status: decided*

Seven places in `src/display.c` gave up, and not one kept the number the platform returned.
The report said `display|failed|the display refused the framebuffer` - a sentence about a
*step*, from a call with five arguments.

Every check in this program reports a code with a failure. The display path grew its own
reporting and never inherited the rule, so the one part a reader can look up or compare between
two consoles was discarded at the moment it was produced.

`obs_give_up_code` carries it, the `display` record has a third field, and the first run with it
produced `0x80290015` - which turned out to be the whole answer, four steps later. The number
had been available on every previous run and thrown away each time.

