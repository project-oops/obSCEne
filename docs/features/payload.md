# Raw Socket Payload Mode

Executing conformance probes as bare ELF memory payloads over port `9021`.

Payload mode sends a raw ELF binary directly to `elfldr` listening on TCP port `9021`. The payload runs unsandboxed with full kernel visibility.

---

## How to Build & Run

```bash
# 1. Compile payload in WSL/Docker
cd obscene
make payload

# 2. In terminal 1: Stream kernel logs
pros logs

# 3. In terminal 2: Dispatch payload to target (or use ./bin/obscene payload)
pros send build/obscene-probe-prospero.elf 9021
```

---

## When to Use Payload Mode
- Probing direct FreeBSD/Prospero kernel syscalls.
- Inspecting physical and virtual memory allocators (`sceKernelAllocateDirectMemory`, `mmap`).
- Probing raw POSIX socket options and errno mappings.

