# obSCEne User Guide

Cross-cutting operational reference for running the obSCEne conformance probe suite on hardware — network ports, portable mode, telemetry syntax, and JSON reporting. Per-mode pages live alongside; this page covers general operation.

---

## Network Architecture & Ports

obSCEne is dispatched to the console over LAN via Prosperous:

- **Port `9021` (`elfldr`)**: Fast memory payload injection.
- **Port `2121` (`ftpsrv`)**: Uploading title directories and package files.
- **Port `3232` (`klogsrv`)**: Live unbuffered telemetry capture.

---

## Paths and Portable Mode

obSCEne produces logs and JSON reports under `%APPDATA%\OOPS\reports\` (or `~/.local/share/OOPS/reports/`).

When running in portable mode (e.g. running from a portable checkout or passing `--output-dir`), all telemetry, logs, and generated JSON reports are saved directly to `./reports/` beside the tool without modifying the host profile.

---

## Telemetry Grammar

obSCEne emits unbuffered lines with structured prefixes:
- `try <symbol>`: Check is about to execute.
- `res <symbol> <code|hex>`: Check completed with returned code.
- `OBS|measure|<key>=<val>`: Verified hardware measurement.
- `OBS|bytes|<key>=<hex>`: Verified structure byte dump.

---

## Generating Machine-Readable Reports

Convert text logs into structured JSON:

```bash
obscene-tool report --input reports/hardware/latest.log --output reports/latest.json
```

