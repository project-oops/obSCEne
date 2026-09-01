# D014 - Presence and behaviour are separate questions, measured separately

**assumed** · 2026-08-19

The behavioural sections ask whether a function *works* and cost a confident
signature each. `900-surface` asks only whether it *exists*, which costs a name.

The second question scales and the first does not. One commercial title imports
around 1,400 symbols; thirty-five behavioural checks will never measure coverage of
that, and the ratio of surface present to surface known is the honest headline for an
emulator's progress. Splitting the questions took the census from 30 symbols to 246
in an afternoon, with no risk taken.

**Three things make it safe:**

- **Weak declarations.** An unresolved import becomes a null address rather than a
  link failure, so absence is reportable instead of fatal.
- **Declared as data, not as functions.** Every censused name is `const char`. Only
  its address is ever read - and calling a function whose signature is unknown is the
  exact mistake D008 exists to prevent. Declaring them as data means the *type system*
  rejects the call, so the rule holds without anyone remembering it. A census of
  several hundred symbols would otherwise be several hundred chances to get it wrong.
- **A wrong name is harmless.** It reports absent: a false negative, visible and
  correctable. Contrast a wrong arity in a behavioural check, which corrupts the stack
  and crashes somewhere unrelated. That asymmetry is exactly why this list may cast a
  far wider net than `platform.h` is allowed to.

The same weak declarations were applied to `platform.h`, so the harness now skips a
behavioural check whose symbol is absent instead of jumping to zero and losing every
check after it.

