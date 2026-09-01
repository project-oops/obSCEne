# D259 - The D-pad pages the results; the auto-cycle is the floor, not the mechanism


*status: decided*

The report cycles its pages after the suite ends, three seconds each, thirty on the summary.
Reaching page nine to read one section meant waiting out the cycle, which is its own annoyance.

The D-pad now drives it: left and right page immediately, holding a direction scrolls, circle
jumps back to the summary. `libScePad` binds on hardware (D248), so `scePadReadState` is
promoted from the census to a real declaration and read through the same weak-symbol guard as
everything else - a platform that does not resolve it keeps cycling and loses nothing.

**The auto-cycle stays as the floor**, for the reason the old comment gave and that still
holds: a controller is one more thing that has to work on a platform being tested precisely
because things do not, and a photographed or headless run has nobody to press a button. So the
pages must advance on their own or a capture gets one frame forever.

**The first press latches `driven` and the cycle stops for good.** Not "resets the timer" -
stops. A viewer who has taken the controller and then paused to read is not fought by a screen
that drifts off their page a few seconds later. A relaunch brings the cycle back, which is a
clear and reversible action, so nothing is lost by making the latch permanent.

`scePadReadState` takes a `void *`, not an `OrbisPadData`. Only the button word at offset 0 is
read; declaring the whole structure would be inventing the fields this does not use. The buffer
is over-sized well past the real structure so the call cannot overrun it - under-sizing is the
only real risk, and over-sizing a stack buffer costs nothing. The offset and masks are from the
OpenOrbis SDK, an open-source toolchain and a permitted provenance source. (D008)

