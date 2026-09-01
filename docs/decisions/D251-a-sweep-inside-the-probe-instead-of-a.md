# D251 - A sweep inside the probe, instead of a rebuild per guess


*status: decided*

`0x80290015` from a five-argument call says which step refused and nothing about which argument.
Finding out by editing a constant costs a package build, an install and a launch per guess -
about five minutes each, with a healthy console needed for every one. Four guesses is most of an
hour and produces four numbers.

The same four numbers come out of one run if the varying happens inside the probe. `085-videobuf`
does that: one call per variation, **each differing from the baseline in exactly one argument**,
each announced before it is made and each reporting its own code.

This program existed to do precisely this and the display path was doing it the expensive way,
because the display is not a section and never got a section's treatment. The first sweep:

```text
baseline        0x80290015
tiling=0        0x80290015
format=0        0x80290003     <- the argument is read
aspect=1        0x80290008     <- so is this one
720p            0x80290015
pitch=width*4   0x80290015
```

Two codes moved, which proves the attribute is being parsed and the baseline values are
accepted. And `0x80290015` survived every attribute change, which proves it is not about the
attribute at all - a conclusion no amount of varying the attribute one rebuild at a time would
have reached, because each of those runs answers only "still refused".

`DISPLAY_PAIR` and `DISPLAY_MEM` are flags for the same reason (D244's): the rule choosing
between the two generations' entry points was written against emulators, where only one pair is
ever real. On a console both resolve.

