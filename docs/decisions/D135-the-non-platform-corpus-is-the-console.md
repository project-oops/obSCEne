# D135 - The non-platform corpus is the console's own software, and it is a different target rather than a worthless one


Status: correction to a judgement this project made and stated twice.

The 1,130,742 unnamed identifiers were dismissed here as "application internals" and "not
conformance surface". The first half was wrong. Reading what actually exports them:

| library | module | exported symbols |
|---|---|---|
| `app` | `app.exe.sprx` | 54,332 |
| `mscorlib` | `mscorlib.dll.sprx` | 20,983 |
| `ScePlayStationPUI` | `Sce.PlayStation.PUI.dll.sprx` | 14,152 |

Every one is `is_export: true` - a published interface other modules import, not private
detail. The console's shell is React Native on a managed runtime, and that is precisely
what these are.

So the split is by **target**, not by worth:

- **Running titles** - a game imports `libSce*` and none of this. A managed title ships
  its own runtime rather than binding the firmware's.
- **Emulating the shell** - impossible without them. They are the shell.

Both are legitimate goals and this project had quietly assumed the first.

### Why obSCEne has to be the carrier

Only when there is no firmware to read. These identifiers come from firmware modules, so a
project holding those modules should sweep them directly - first-hand, with per-module
attribution, and no synthetic file in between.

Without dumps that route does not exist. The identifiers are then available only as public
metadata, and a project forbidden to consult databases (D084) cannot take them from there.
A module obSCEne builds is the one path, and it is legitimate for identifiers because the
rule governs *names*: a hash carries none, and identifiers are already harvested freely
from any module swept.

