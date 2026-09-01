# D256 - Bounded flip wait ensures display flip takes effect


*status: measured*

After submitting a frame buffer to the video output, the display flip takes effect at the next vblank. Waiting for the flip ensures buffers are not overwritten prematurely.

