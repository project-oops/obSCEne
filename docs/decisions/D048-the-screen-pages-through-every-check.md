# D048 - The screen pages through every check, and cycles rather than waiting for input


Status: decided.

The summary shows the shape of a run - seventeen section rows and a set of totals. It
cannot show ninety-five identifiers at a size anyone can read from across a room, and a
screen that has to be squinted at has lost its only advantage over the stream.

So there are two views and they answer different questions. The summary says "is anything
working"; a page says "which one". Eighteen checks per page, six pages, three seconds
each, cycling from the summary and back.

**Cycled, not driven by input.** A controller is one more thing that has to work on a
platform being tested precisely because things do not work on it, and a run being
photographed has nobody there to press a button. Every page reaches the screen on its own,
so any of them can be captured.

**It only pages when there is a display.** Without one `obs_screen_present` returns at
once and the process exits as before, which is what anything automated needs. With one it
never returns - and that is correct, because the report is complete and on record before
the first page is drawn. Nothing is being held back; what follows is for whoever is
looking.

**A collision worth remembering.** `display.h` defined `OBS_PASS`, `OBS_FAIL` and friends
as colours, which are also `obs_status` values. A macro shadowing an enum constant turns a
`switch` over statuses into a switch over integers, and the compiler refused it - loudly,
and only once something switched on a status. The colours are now `OBS_COLOUR_*`. Naming a
thing after what it draws rather than what it is was the mistake.

