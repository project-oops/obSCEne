//! Registered consoles: which ones this machine knows about.
//!
//! # Why this exists
//!
//! A real console is not like the emulators. An emulator is a path on this machine, it is
//! there or it is not, and running it costs nothing. A console is somewhere on a network, its
//! capabilities depend on which payloads happen to be loaded, and that set changes every time
//! it reboots - a jailbreak does not survive a power cycle, and the chain that comes back
//! depends on a text file somebody edited weeks ago.
//!
//! So "can I talk to the hardware" is not one question but several, and the answers move. The
//! afternoon this was written, a loader died and nothing noticed for hours, because the thing
//! that would have said so was a port nobody had thought to check.
//!
//! # What is left here, and what moved
//!
//! **This file is now the registry and nothing else.** Everything that spoke to a console -
//! the service table, the probe, the log reader, the loader client, the shell client - is in
//! `pros_link`, because orbistoun needs exactly the same code and the wrong next step was a
//! second copy of it. (D189)
//!
//! The registry stayed because it is not transport: it is this project's own file, in this
//! project's own place, and the shared version of it belongs in a crate this tool
//! deliberately does not depend on.
//!
//! # What a registration is, and what it is not
//!
//! It is an address and a name. It is **not** a claim that the console is reachable, jailbroken,
//! or capable of anything - `check` establishes that, freshly, every time it is asked. Storing
//! a capability would be storing something that expires without notice, which is the same
//! mistake as a stale exclusion list.
//!
//! # Where the file lives
//!
//! `hardware.txt` in the collection's directory, hand-parsed, one console per line - the same
//! directory every tool here writes to, resolved by `oops_paths`.
//!
//! It used to be `~/.config/obscene/`, chosen over `%APPDATA%` on the grounds that a tool
//! running inside a packaged container has its writes there redirected into a per-package cache
//! and made invisible. **That hazard belongs to packaged applications and this is a plain
//! executable**, so the platform's own directory is right after all - and being in the shared
//! one means the console registered here is the console Prosperous already knows about.
//!
//! Those two are still separate files with separate formats, which is a thing to fix rather
//! than a design: `targets.txt` and `hardware.txt` record the same fact.
//!
//! Line-oriented and parsed by hand for the reason given in `Cargo.toml`: this is a small
//! table, and splitting is simpler to test than a format crate is to justify.

use std::fmt::Write as _;
use std::path::PathBuf;

/// A console somebody has registered.
pub struct Console {
    /// A short label, used to pick one when several are registered.
    pub name: String,
    /// Host or address. Stored as written, resolved at use.
    pub address: String,
}

/// Where registrations are kept.
///
/// Returns `None` when the home directory cannot be determined, which is not an error worth
/// failing a build over - it means this machine has no place to keep the file and the caller
/// should say so plainly.
#[must_use]
pub fn config_path() -> Option<PathBuf> {
    // Refusing rather than falling back, because this is a file somebody goes looking for:
    // writing it beside wherever they happened to be standing is how it becomes unfindable.
    oops_paths::Paths::resolve_with_options("obscene", oops_paths::Options::new().refusing())
        .map(|paths| paths.data_root().join("hardware.txt"))
}

/// Read every registration. A missing file is an empty list, not a failure.
pub fn load() -> std::io::Result<Vec<Console>> {
    let Some(path) = config_path() else {
        return Ok(Vec::new());
    };
    let text = match std::fs::read_to_string(&path) {
        Ok(text) => text,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => return Ok(Vec::new()),
        Err(error) => return Err(error),
    };
    Ok(parse(&text))
}

/// One console per line: `name<space>address`. Blank lines and `#` comments are ignored.
fn parse(text: &str) -> Vec<Console> {
    let mut out = Vec::new();
    for line in text.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let Some((name, address)) = line.split_once(char::is_whitespace) else {
            continue;
        };
        let address = address.trim();
        if address.is_empty() {
            continue;
        }
        out.push(Console {
            name: name.to_owned(),
            address: address.to_owned(),
        });
    }
    out
}

/// Serialise registrations back out, header and all.
fn render(consoles: &[Console]) -> String {
    let mut out = String::from(
        "# Consoles this machine knows about, one per line: <name> <address>\n\
         #\n\
         # A line here is an address, not a promise. Whether a console is reachable or\n\
         # jailbroken is established by `obscene-tool hw check`, every time, because the\n\
         # answer changes on every reboot.\n\n",
    );
    for console in consoles {
        let _ = writeln!(out, "{} {}", console.name, console.address);
    }
    out
}

/// Add or replace a registration.
pub fn register(name: &str, address: &str) -> std::io::Result<()> {
    let Some(path) = config_path() else {
        return Err(std::io::Error::other(
            "no home directory, so there is nowhere to keep registrations",
        ));
    };
    let mut consoles = load()?;
    consoles.retain(|c| c.name != name);
    consoles.push(Console {
        name: name.to_owned(),
        address: address.to_owned(),
    });
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent)?;
    }
    std::fs::write(&path, render(&consoles))?;
    println!("registered {name} at {address}");
    println!("  {}", path.display());
    Ok(())
}

/// Pick a console by name, or the only one when a name is not given.
pub fn resolve(consoles: Vec<Console>, wanted: Option<&str>) -> Result<Console, String> {
    if let Some(name) = wanted {
        return consoles
            .into_iter()
            .find(|c| c.name == name)
            .ok_or_else(|| format!("no console registered as `{name}`"));
    }
    let count = consoles.len();
    let mut iter = consoles.into_iter();
    match (iter.next(), count) {
        (Some(only), 1) => Ok(only),
        (None, _) => Err("no consoles registered; see `obscene-tool hw register`".to_owned()),
        _ => Err("several consoles registered; name one with --name".to_owned()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_registration_round_trips() {
        let text = "# comment\n\nliving-room 192.168.1.206\ndesk  10.0.0.4\n";
        let found = parse(text);
        assert_eq!(found.len(), 2);
        assert_eq!(found.first().map(|c| c.name.as_str()), Some("living-room"));
        assert_eq!(found.get(1).map(|c| c.address.as_str()), Some("10.0.0.4"));
    }

    /// A line with a name and no address is not half a registration, it is not one.
    #[test]
    fn a_nameless_or_addressless_line_is_skipped() {
        assert!(parse("solo\n").is_empty());
        assert!(parse("name   \n").is_empty());
    }

    #[test]
    fn rendering_is_parseable_again() {
        let consoles = vec![Console {
            name: "ps5".to_owned(),
            address: "192.168.1.206".to_owned(),
        }];
        let found = parse(&render(&consoles));
        assert_eq!(found.len(), 1);
        assert_eq!(
            found.first().map(|c| c.address.as_str()),
            Some("192.168.1.206")
        );
    }

    /// Naming nothing is only unambiguous when there is one to mean.
    #[test]
    fn resolving_without_a_name_needs_exactly_one() {
        let one = vec![Console {
            name: "a".to_owned(),
            address: "1".to_owned(),
        }];
        assert!(resolve(one, None).is_ok());
        assert!(resolve(Vec::new(), None).is_err());
        let two = vec![
            Console {
                name: "a".to_owned(),
                address: "1".to_owned(),
            },
            Console {
                name: "b".to_owned(),
                address: "2".to_owned(),
            },
        ];
        assert!(resolve(two, None).is_err());
    }

    #[test]
    fn registering_the_same_name_replaces_rather_than_duplicates() {
        let mut consoles = parse("ps5 192.168.1.206\n");
        consoles.retain(|c| c.name != "ps5");
        consoles.push(Console {
            name: "ps5".to_owned(),
            address: "192.168.1.9".to_owned(),
        });
        assert_eq!(consoles.len(), 1);
        assert_eq!(
            consoles.first().map(|c| c.address.as_str()),
            Some("192.168.1.9")
        );
    }
}
