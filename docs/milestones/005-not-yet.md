# Not yet


Kept here so the list stays honest about what it does not include.

- **No picture from the GPU.** Everything drawn so far is written by the CPU a pixel at a time.
  `165-gnm` passes its dispatch checks; nothing has yet put a GPU-produced frame on screen.
- **No network from inside a title.** `sysinfo|ip` is `unconfirmed`. The protocol server has
  never answered on hardware.
- **`libScePosix` does not load at all**, so five checks behind it are untested rather than
  failed.
- **The generation is unknown, not detected.** The markers are `EI_ABIVERSION 2` libraries a
  `ps4_game` title is refused, so the probe cannot look. It says so now rather than guessing,
  which is a fix and not an answer. (D255)
