# D084 - obSCEne may consult other projects. orbistoun may not. The asymmetry is load-bearing


Status: correction to advice this project gave.

The notes sent to the orbistoun side recommended generating its identifier-to-name table
from a public database, on the grounds that this project's hash reproduces all 78,372
pairs. That recommendation was **declined, correctly, and should not be repeated.**

orbistoun obtains names by proposing candidates and letting the hash confirm them, and
consults nothing. That is not an oversight to be optimised away - it is the property that
repository exists to have. Adopting a table would retroactively poison every name already
found, because afterwards nobody could tell a name the sweep produced from one the table
supplied. The property is all-or-nothing, and their sweep yield makes the cost concrete:
249,506 candidates for one confirmed name.

**The asymmetry is deliberate and this project is on the permissive side of it.** obSCEne
may read emulator source, public databases and toolchain headers, and does. That freedom is
exactly what makes its advice about provenance untrustworthy in the other direction, and
the mistake here was assuming a rule that suits one project transfers to the other.

Also recorded from their reply: a finding shaped as "here is a mistake to avoid" is
something they can act on; one shaped as "here is how another loader implemented X" is not.
Everything sent so far was the first kind. The line is theirs to hold and worth knowing
before writing the next set of notes.

