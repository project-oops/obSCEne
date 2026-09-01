# D216 - obSCEne's end of "the real way" is built and waiting on one selfish command - install and launch via prosperous, into a foreground context


Rendering needs a foreground app (D214), which needs an installed package. The package is
selfish's format work and is assumed here to be arriving (its library functions already build a
signed, encrypted fake package - `selfish-pfs::outer::build` and `selfish-pkg::write::build`;
only a CLI wrapper is missing). Everything downstream of that is obSCEne's, and it is now built.

### The flow, end to end

```
make pkg                            # selfish builds obscene.pkg   (assumed; make pkg is wired)
obscene-tool hw install obscene.pkg # push over FTP, then pkg_install via shsrv
obscene-tool hw launch OBSC00001    # foreground app - owns the display and a user session
obscene-tool hw pull /data/obscene-report.txt   # the report the running app wrote
```

`hw push`, `hw install` and `hw launch` are new, and each is nothing but an existing prosperous
call: `pros_link::files::store` (FTP STOR) and `pros_link::shell::run` (shsrv, which carries
`pkg_install` and `launch`). No store account is involved - a jailbroken console installs a fake
package directly. The push transport is proven (a file put and read back); install and launch
are the same two calls and are left unrun because they act on the operator's console.

### Why this is the whole answer to "can we skip the package"

No. This console launches only **installed** apps as foreground - an installed homebrew app
keeps its eboot inside an encrypted `app.pkg`, mounted read-only, and there is no loose-eboot
launcher present. So the eboot must be packaged. But the package is buildable (selfish), and
`pkg_install` takes it, so the path is real - it is not blocked on anything obSCEne owns.

When obSCEne runs as that foreground app, its display code (`src/display.c`, which renders on
PS5PCEM) has a user session and owns the scanout - the two things the injected background
payload was refused (D214) - so it should render, and it writes its report to
`/data/obscene-report.txt` as well.

Status: **assumed** - push proven on hardware; install/launch built and unrun; the package they
consume is selfish's to deliver.

