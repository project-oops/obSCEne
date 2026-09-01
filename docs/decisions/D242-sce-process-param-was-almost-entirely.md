# D242 - `sce_process_param` was almost entirely empty, and one field's comment was evidence-free


*status: measured*

Read side by side against a title that launches:

```text
        +0x0c entry_count   +0x10 sdk_version
ours    0                   0
real    5                   0x08008011
```

The comment on `sdk_version` said a real launching executable declares zero too. It came from
the kernel log line `SDK vesion: PS4:00000000 PPR:00000000` - **which was this program's own
run.** A build's own output was read as evidence about somebody else's build, and then written
down as a fact about the platform.

That is the fourth instance of one shape (D232, D233, D235): an observation recorded as the
thing it was evidence for. The distinguishing feature every time is that no second source was
consulted, and a second source existed.

Both fields are now set from the oracle. What that did is D244.

