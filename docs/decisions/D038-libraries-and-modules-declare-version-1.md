# D038 - Libraries and modules declare version 1, and they pack it differently


Status: decided, from a loader's own parser.

The identity values were emitted with version zero throughout. A loader builds its
symbol lookup key from the version *the module declares* and matches it against the
version the library was registered with, so a module claiming version zero against a
library registered as version one matches nothing - and every symbol from it fails to
resolve with no error anywhere, because as far as the loader is concerned it was asked
for something that does not exist.

The two shapes are not the same, which is why there are now two functions rather than
one with a comment:

| | 63-48 | 47-32 | 31-0 |
|---|---|---|---|
| library | id | version (16 bits) | name offset |
| module | id | major (8) ‖ minor (8) | name offset |

**Honest about what this bought.** It is what the loader's parser says, and the values
we emit now match what it registers. It has not yet changed observed behaviour in
either emulator: the one that runs the module resolved 928 imports before and after,
and the other still does not bind. It is a correctness fix, not a demonstrated one.

