# obSCEne User Guide

Cross-cutting operational reference for running the obSCEne conformance probe suite on hardware — network ports, portable mode, telemetry syntax, and JSON reporting. Per-mode pages live alongside; this page covers general operation.

---

## Network Architecture & Ports

obSCEne is dispatched to the console over LAN via Prosperous:

- **Port `9021` (`elfldr`)**: Fast memory payload injection.
- **Port `2121` (`ftpsrv`)**: Uploading title directories and package files.
- **Port `3232` (`klogsrv`)**: Live unbuffered telemetry capture.

---

## Paths

obSCEne's sink tries several on-target paths in order and records which one answered:
`/data/obscene-report.txt`, `/download0/obscene-report.txt`, `/mnt/usb0/obscene-report.txt`,
or a relative `obscene-report.txt` beside the process. There is no AppData/XDG report location
and no portable-mode flag - see `docs/ARTIFACTS.md` ("Where a report comes out").

On the operator's own machine, `obscene-tool report` captures the console's records into
`reports/hardware/console-klog.txt` by default (`--into` overrides it).

---

## Telemetry Grammar

Every line is pipe-separated and begins `OBS|` (the full contract is `docs/OUTPUT.md`):
- `OBS|try|<check-id>|<library>|<symbol>`: Check is about to execute.
- `OBS|res|<check-id>|<status>|<value>|<detail>|<provenance>`: Check completed with a verdict.
- `OBS|measure|<check-id>|<symbol>|<quantity>|<value>|<unit>`: A measurement, no verdict attached.
- `OBS|bytes|<check-id>|<symbol>|<label>|<offset>|<hex>`: One line of a structure byte dump.

---

## Capturing Reports

`obscene-tool report` captures obscene's own `OBS|` records off the console system log into a
plain text file - it is not a JSON converter and takes no `--input`/`--output` file arguments:

```bash
obscene-tool report --seconds 120 --into reports/hardware/console-klog.txt
```

