# D330 - Audio format selectors, mouse stubs in unsigned payloads, and POSIX sockets for Porthole

**decided** - 2026-09-08

Hardware sweep `20260908-170447` on PS5 12.40 settled three architectural questions across the
three launch shapes (`payload`, `pkg`, `eboot`):

## 1. Audio Format Selectors (`090-audio/format-selector`)

The port state structure returned by `sceAudioOutGetPortState` exposes the configured channel count
at byte offset 2:

- **Selector 0** (`0x0`): configured for **mono** (`channels|0x1|count`, state byte 2 = `0x01`).
- **Selector 1** (`0x1`): configured for **stereo** (`channels|0x2|count`, state byte 2 = `0x02`).
- **Selector 2** (`0x2`): also configured for **stereo** (`channels|0x2|count`, state byte 2 = `0x02`).
  State record is identical to selector 1 (`810002c7ffff05000000000000000000` on eboot,
  `830002c7ffff05000000000000000000` on pkg).

**Conclusion**: Selector 2 does **not** configure 8-channel (7.1) output on hardware; both selector 1
and selector 2 produce 2 channels. Audio output code in `oops-sdk` must use selector 0 for mono and
selector 1 (or 2) for stereo.

## 2. Mouse Input in Payload Mode (`101-input-ext/mouse-read`)

- `libkernel` exports symbols for `sceMouseInit`, `sceMouseOpen`, `sceMouseClose`, and `sceMouseRead`.
- In signed title execution (`pkg` and `eboot`), `sceMouseOpen` succeeds (returning handles such as
  `0x650700` and `0x6a0700`) and `sceMouseRead` returns `0x0`.
- In unsigned elfldr payload mode, `libSceMouse.sprx` is not loaded in the target process. The
  `libkernel` exports are unresolved dynamic stubs.
- Calling any of these stubs in payload mode raises kernel signal `0xa0020101`
  (`PRX_NOT_RESOLVED_FUNCTION`), immediately killing the process.
- User service initialization fallbacks (`sceUserServiceInitialize`, `0xFF` system user) do not help
  because the defect is at the PRX linkage level, not user credentials.

**Conclusion**: Unsigned payload mode cannot read mouse input through `libkernel` without manually
loading and binding `libSceMouse.sprx`. Probes in payload mode must inspect stub resolution and skip
rather than jumping into the unlinked stub address.

## 3. POSIX Sockets for Porthole (`102-net`)

Porthole operates as an elfldr payload and uses the POSIX / BSD socket layer rather than `libSceNet`:

- **Socket creation**: Raw syscall 97 (`SYS_socket(AF_INET, SOCK_STREAM, 0)`) and the `__sys_socketex`
  libkernel export both successfully allocate socket descriptors (fd `0xc`).
- **Non-blocking configuration**:
  - `setsockopt(s, 0xFFFF, 0x1200, &one, 4)` returns `0x0`. Option `0x1200` (`SO_NBIO`) at level
    `0xFFFF` (`SOL_SOCKET`) is the genuine non-blocking socket option on the kernel.
  - `fcntl(s, F_SETFL, O_NONBLOCK)` returns `-1` (`0xffffffff`). `fcntl` flag setting does not enable
    non-blocking behavior on this platform socket implementation.
- **Would-block behavior**:
  - On a connected socket with empty receive buffer: `recv` returns `-1` with `errno = 0x23` (35 =
    `EWOULDBLOCK` / `EAGAIN`) in under 5 microseconds.
  - On an idle listener: `recv` returns `-1` with `errno = 0x39` (57 = `ENOTCONN`).
  - In title mode through `libSceNet`, the error codes are shifted: `0x80410123` (`0x80410100 + 35`) and
    `0x80410139` (`0x80410100 + 57`).
- **Buffer saturation**: Non-blocking `send` on a loopback connection accepts 49,032 bytes (`0xbf88`)
  over 2 calls before returning would-block (`errno = 35` / `0x80410123`) in 0 to 2 microseconds.
- **Sockaddr bind**: `bind` accepts `sin_len = 16` and `sin_len = 0` with `sin_vport = 0`, both
  returning `0x0`.

