# 2026-09-08 - Audio channels settled, mouse payload stub, and POSIX net

Hardware sweep `20260908-170447` ran fresh builds across all three launch shapes (`payload`, `pkg`,
`eboot`) on PS5 12.40 and settled the remaining open questions from earlier sweeps. Documented in
`D330`.

## Audio format selectors on the wire

`090-audio/format-selector` opened three audio ports corresponding to selectors 0, 1, and 2, but
earlier sweeps deduped the identical byte records of selector 1 and selector 2. Distinct measurement
labels (`state-sel0`, `state-sel1`, `state-sel2`) and an explicit `channels` measure extracted from
offset 2 of the port state structure put all three selectors on the wire:

- Selector 0: `channels|0x1|count`, state byte 2 = `0x01` (mono).
- Selector 1: `channels|0x2|count`, state byte 2 = `0x02` (stereo).
- Selector 2: `channels|0x2|count`, state byte 2 = `0x02` (stereo, matching selector 1 byte-for-byte).

This settles the eight-channel question: selector 2 does not produce 8-channel audio; both selector
1 and selector 2 configure 2 channels.

## Mouse input and unlinked stubs in payload mode

`101-input-ext/mouse-read` previously skipped in payload mode with "no initial user for mouse". When
the dynamic user lookup and `0xFF` system user fallback were applied, calling `sceMouseInit` at
`0x8002b00d0` in payload mode raised kernel signal `0xa0020101` (`PRX_NOT_RESOLVED_FUNCTION`),
revealing that the `sceMouse*` exports in `libkernel` are dynamic forwarders to `libSceMouse.sprx`,
which is not linked in unsigned elfldr payload mode.

Guarding the payload leg to inspect resolution without calling the unlinked stub allowed the payload
sweep to complete all sections through `910-bulk` cleanly without process termination, while `pkg`
and `eboot` title modes confirmed `sceMouseOpen` (handles `0x650700` and `0x6a0700`) and
`sceMouseRead` (`rc 0x0`).

## POSIX sockets for Porthole (102-net)

Section `102-net` was retargeted to measure the BSD socket layer directly for payload mode:

- Direct raw syscall 97 (`SYS_socket`) and `__sys_socketex` both succeed (returning descriptor `0xc`).
- Non-blocking socket option is `0x1200` at level `0xFFFF` (`SOL_SOCKET`), returning `0x0`.
  `fcntl(s, F_SETFL, O_NONBLOCK)` returns `-1`.
- Connected non-blocking `recv` promptly returns `errno = 0x23` (35, `EWOULDBLOCK` / `EAGAIN`).
- Listener non-blocking `recv` promptly returns `errno = 0x39` (57, `ENOTCONN`).
- Send buffer saturation occurs at 49,032 bytes (`0xbf88`) after 2 sends, returning would-block in
  0 microseconds.
- `bind` succeeds with both `sin_len = 16` and `sin_len = 0` with `sin_vport = 0`.

