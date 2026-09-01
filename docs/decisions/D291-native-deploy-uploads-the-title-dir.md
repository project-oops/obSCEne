# D291 - native deploy uploads the title dir to a scan root via prosperous


`deploy` builds and installs the *package* (an HTTP fetch the console pulls in). The native title is
a different shape - a directory of `eboot.bin` + `sce_sys/{param.json,icon0.png}` - so it needs a
different deploy: a directory upload, not a package fetch. `pkg` had one and `native` did not.

`obscene-tool hw install-native <dir>` is the native counterpart to `hw install`. It reads the id
from the directory's own name (`make native` writes `<BUILD>/native/<TITLE_ID>/`) and uploads the
tree to `<base>/<TITLE_ID>/` through prosperous's `transfer::upload` - an FTP STOR per file,
directories made as needed, the same call `pros-cli restore` uses. All out-connections, so it runs
from WSL; only the package `hw install` needs the Windows side. `scripts/native-deploy.sh`
orchestrates it and `./bin/obscene native --deploy` is the verb; `--deploy-only` skips the build,
`--into` picks the base, `--name` the target.

**The base is a scan root, not `/user/app`.** The instinct is to copy to `/user/app/<id>`, where
installed titles live - but that directory is where an auto-mounter (ShadowMountPlus) *registers
titles to*, and it is never *scanned*, so a copy there is inert. The auto-mounter's scan roots are
`/mnt/usb0`..`usb7`, `/mnt/ext0`, and `/user/data`. So the default base is `/user/data`: land the
directory there and the mounter finds it (`sce_sys/param.json` at scan-depth 1), mounts it, and
registers it via `AppInstallTitleDir` - which is what a copy alone cannot do. `--into /mnt/usb0`
(or any absolute path) targets a USB drive or a titles subfolder instead.

**Proven end to end, and it surfaced two facts.** The first run uploaded to the live console
(`ps5 192.168.1.211`) and the files landed - confirmed by `hw ls`. That first run targeted
`/user/app/OBSC00001`, which already held `app.pkg`/`app.json`/`app.pbm`/`app.xml` from an earlier
*package* install of the same id: (1) `/user/app` is the wrong target, now corrected to a scan root;
and (2) OBSC00001 is already registered as a download-stub package, whose launcher mounts `/app0`
from `app.pkg`, so even once registration works, a title id already claimed by a package will not
launch the native eboot - a clean native test wants a fresh id, or the package uninstalled first.
