# call and read implemented


The interactive primitives (D129). `call <addr> <args>` and `read <addr> <len>` added to the
net.c server, announced as capabilities. Proven live on the host serve build: read returned
the ELF magic from process memory, call returned a real function's value, and call 0x0 left a
lone ack (the died signature) with the process gone.

### Surprises
- Both live bugs were wrong-record, not crashes: a double `0x` in the read id, and a null
  address refused instead of called (which hid the death path). The socket test caught both;
  neither showed up in the build.
- The shadPS4 serving path now dies after the newly-added 160-gpu section on that loader,
  before reaching listen - so call/read were proven on the host socket rather than in shadPS4
  this round. Same net.c either way. The gpu-section crash on shadPS4 is a separate item.

