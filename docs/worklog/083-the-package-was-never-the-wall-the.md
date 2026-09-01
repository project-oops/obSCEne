# The package was never the wall - the install context is (fw12, measured on hardware)


The `0x80b211c8` chase resolved to a different diagnosis, found via `phantomptr/ps5upload`'s
fw12 research and confirmed on the console.

### What `0x80b211c8` actually is

`shsrv`'s `pkg_install` calls **`sceAppInstUtilInstallByPackage`** (an http/URL installer), which
a console's PlayGo HTTP pre-flight gates. That pre-flight is what returns `0x80b211c8` /
`0x80B2116F` - a **firmware-12 AppInst/PlayGo incompatibility with the jailbroken context the
install is issued from**, not a verdict on the package. ps5upload's header names the escape:
`sceAppInstUtilInstallByPackage` is *"gated by Sony's PlayGo HTTP pre-flight"*, while
**`sceAppInstUtilAppInstallPkg(path)`** - a bare local path, no URI parse, no HTTP - *"works
without the PlayGo gate"*. It is the path Itemzflow (LightningMods) and elf-arsenal install
through.

So the package built here is, as far as the console's structural and content checks go, done:
header, digests, RSA signature, entry layout, playgo chunk/manifest/sha and inner-image size all
match a real package (the whole prior worklog). What remained was **how** the install is issued.

### The installer, and the last gate

`payload/installer.c` is an elfldr payload that loads `libSceAppInstUtil` and calls
`sceAppInstUtilInitialize` + `sceAppInstUtilAppInstallPkg("/data/pkg/obscene.pkg")` - the ungated
local path, from the same elfldr context shsrv installs from. It runs (`OBS_INSTALL: begin`), and
its three dependencies (`libSceLibcInternal`, `libSceRegMgr`, `libSceIpmi`) load cleanly. But
**`libSceAppInstUtil.sprx` itself is refused**: `sceKernelLoadStartModule` returns `0x80020008`,
rtld logs `Loadability error libSceAppInstUtil.sprx 13`. The dependencies loading while the
library does not is an **authority** gate - that library needs `SYSTEM_AUTHID`, which shsrv's
signed-and-exec'd bundle holds and an injected payload does not.

This is the same reason every scene DPI daemon (cy33hc, ps5upload, elf-arsenal) **self-escalates
to `SYSTEM_AUTHID` via kernel R/W before touching AppInstUtil**. The remaining step is exactly
that: escalate this payload's own process credentials with the kpipe/kdata primitives elfldr
already hands us in `payload_args`, using the published fw-12.40 proc/ucred offsets, then load the
library and call `AppInstallPkg`. That is a bounded, known task - and the point at which a wrong
kernel offset is a real risk, so it is worth being deliberate about rather than guessing.

