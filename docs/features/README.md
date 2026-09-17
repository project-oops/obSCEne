# obSCEne Features & Probing Modes

Feature documentation and execution modes for the obSCEne hardware conformance suite.

| Execution Mode | Target Artefact | Privileges & Context | Documentation Page |
| :--- | :--- | :--- | :--- |
| **User Guide** | `obscene-tool --help` | System setup, telemetry format, ports | [user-guide.md](user-guide.md) |
| **Raw Socket Payload** | `build/obscene-probe-prospero.elf` | Direct `:9021`, previous-generation compatibility sandbox (`ps4_mode`) | [payload.md](payload.md) |
| **Full-Screen BIG_APP** | `build/prospero/<TITLE_ID>/eboot.bin` (via `make native`) | Direct HDMI screen ownership, AGC GPU | [eboot.md](eboot.md) |
| **Retail Package** | `build/obscene-probe-orbis.pkg` | Retail sandbox, report file sealed `0600` inside it | [pkg.md](pkg.md) |
