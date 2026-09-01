# D087 - `000-boot/stack-alignment`, offered by the orbistoun side and worth taking


Status: added.

x86-64 SysV requires sixteen-byte stack alignment at a `call`. Compiled code assumes it and
places aligned spills on that basis, so a violation does not fail at the boundary - it fails
the first time something spills a vector register, possibly in an unrelated function, and
only once an implementation is large enough to want one.

That makes it invisible until it is expensive. The orbistoun side found 370 of 372 guest
calls arriving misaligned for the entire life of the project, undetected until exactly that
threshold was crossed.

**No loader passes this by accident**, and one that jumps to an entry point rather than
calling one gets it wrong with no other symptom. It costs one check.

It reads its own frame, so it measures the alignment this program runs with - what a
platform function called from here inherits. It cannot see how a *callback* is entered, so a
loader correct for ordinary calls and wrong for callbacks would pass. Stated in the check
rather than implied.

An earlier external review raised entry-stack alignment as a hypothesis and it was set aside
as resting on a stale README. The hypothesis was sound and the reasoning for dismissing it
was about the wrong evidence.

