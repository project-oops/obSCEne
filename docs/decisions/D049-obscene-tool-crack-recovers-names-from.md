# D049 - `obscene-tool crack` recovers names from NIDs by guessing, and says what a miss is worth


Status: decided.

A NID is the first eight bytes of `SHA-1(name ‖ suffix)`. Hashing is one way, so the only
route back is to hash candidates and look for a match. That is the whole tool.

**It is not `decode`, and the two are easy to confuse.** `decode` reverses the
*encoding* - eleven printable characters back into the eight bytes they stand for. Both
directions of that are cheap and neither touches the hash. After decoding you have the
hash and you are exactly as stuck. The confusion is worth naming because the command that
sounds like it does this does not.

**Hits are proof; misses are nothing.** A match is certain, because the hash agrees. A
non-match says only that the candidate list did not contain the name - never that no such
name exists. Presenting the two alike would turn "we did not guess it" into "it is not
there", which is the kind of confident wrongness this project keeps having to design
against. The output states how many candidates were tried so a reader can weigh a miss.

**The generator is measured, not assumed.** `--known` takes pairs already established and
reports how many the candidate list reproduced. A list that cannot regenerate names
already known is not ready to be believed about unknown ones, and the tool warns on
standard error when it falls short. `data/nid-corpus.txt` holds 389 such pairs.

**The output carries its own provenance.** Suffix, candidate count, reproduction rate,
recovery count - because a generated table outlives the run that made it and gets trusted
long after it stops being right. Same argument as provenance on a check (D044).

**Why it did not exist until now.** Every check so far started from a name already known,
so nothing needed the reverse direction. It becomes necessary the moment coverage starts
from the machine's list rather than one this project wrote, which is what the enumerator
(D043) introduced. It was blocked behind that, not overlooked.

**Why it lives here and not in the emulator.** The hasher, the corpus and the validation
set are all here, and an emulator never cracks anything - it resolves NIDs by table
lookup at load time. The complementary job, which only a loader can do, is recording
which NIDs it *failed* to resolve. That list is the ranking worth spending candidates on,
and it does not need names to be useful.

