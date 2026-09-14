# Retail Package (PKG) Mode

Executing conformance probes within an installed retail PFS sandbox.

Some system functions behave differently when executed from a signed, encrypted retail package installed to `/user/app/` under strict OS sandbox restrictions.

---

## How to Build & Run

```bash
# 1. Compile package in WSL/Docker
cd obscene
make pkg

# 2. Upload and install via Prosperous
pros restore build/obscene-probe-orbis.pkg /data/pkg/obscene-probe-orbis.pkg
```

---

## Probing Scope
- Encrypted title save mounting (`sceSaveDataMount`).
- Sandbox directory traversal restrictions.
- Dynamic linking behavior inside retail application partitions.

