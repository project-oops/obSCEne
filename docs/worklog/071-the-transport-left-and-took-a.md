# The transport left, and took a duplicated guard with it


`src/hardware.rs` was 435 lines doing two jobs. It is 229 now, and does one.

Everything that spoke to a console - the service table, the probe, the log reader, the loader
client, the shell client - is `pros-link`, a std-only crate in the `prosperous` repository
beside this one, taken here as a path dependency. The emulator takes the same crate. (D189)

### What was proved, and how

A console was not to hand, so the tool was run end to end against a stand-in written in Python
- deliberately not the crate's own Rust fake, because two halves of a proof that share an
implementation only establish that it agrees with itself.

In an isolated home directory, so no real registration was touched:

- `hw check` reported four services up and the closed one down, with its 1508 ms timing
- `hw logs --seconds 3` ended on its own clock against a stream that never ends
- `hw sh "ps"` came back without the banner in it
- `hw send` on a vendor module was refused before a byte left the machine, naming both the
  shape found and this project's own two build outputs
- `hw send` on a payload arrived whole

228 tool tests, clippy at `-D warnings`, `cargo fmt --check` and `guards --root ..` all pass.

### Two surprises worth recording

**The shared crate's own fake had the defect this project keeps meeting.** A connection
accepted from a non-blocking listener inherits the non-blocking mode, so every read in it
returned immediately whether or not anything had arrived - and the loop written to wait for a
client to go quiet was not waiting at all. Its tests passed anyway, because the payloads in
them are small enough to arrive in the first few reads. A mechanism whose *did nothing* output
is identical to its *changed nothing* output, again, in a tool two projects were about to
trust.

**CI will not build this crate as it stands.** The workflow checks out one repository and the
path dependency points outside it. Named in D189 rather than papered over; the fix is a second
checkout step, and it needs a remote that does not exist yet.

