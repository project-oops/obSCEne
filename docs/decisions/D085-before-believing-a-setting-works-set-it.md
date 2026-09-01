# D085 - Before believing a setting works, set it to something that must visibly break


Status: adopted from the orbistoun side, and applied.

They found their stub policy had been wired to nothing for months: the default and every
per-symbol override were consulted nowhere between the configuration file and the guest.
Documented-and-true and implemented-and-false at once, invisible to reading the code, and
caught by setting the default to a value that must change behaviour and observing that
nothing changed.

That is a better version of a lesson this project keeps relearning. `verify.sh` could not
fail for its entire existence; `guards.py` was written because a rule in CLAUDE.md was
enforced nowhere; the `multipass` stderr hazard was documented in one script and absent from
another. Every one of those is the same shape: **a mechanism believed to work because it was
written down.**

Applied to this project's own knobs, none of which had ever been tested for effect:

| knob | control | result |
|---|---|---|
| `EXCLUDE` | exclude a check that otherwise passes | skipped, with the right reason |
| `CHURN` | build at 3, read the reported count | `0x28` becomes `0x3` |

Both wired. **Nothing was found**, and that is worth recording rather than skipping: the
thread-churn bisect (BACKLOG §6c) rested on `CHURN` doing something and had never confirmed
it, so a negative result there retires a real risk to a conclusion already drawn.

