# obSCEne Features & Probing Modes

Feature documentation and execution modes for the obSCEne hardware conformance suite.

| Execution Mode | Target Artefact | Privileges & Context | Documentation Page |
| :--- | :--- | :--- | :--- |
| **User Guide** | `obscene-tool --help` | System setup, telemetry format, ports | [user-guide.md](user-guide.md) |
| **Raw Socket Payload** | `build/obscene.elf` | Direct `:9021`, unsandboxed kernel | [payload.md](payload.md) |
| **Full-Screen BIG_APP** | `build/eboot.bin` | Direct HDMI screen ownership, AGC GPU | [eboot.md](eboot.md) |
| **Retail Package** | `build/obscene.pkg` | Retail sandbox (`0600`), save mounts | [pkg.md](pkg.md) |
