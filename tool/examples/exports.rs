//! Throwaway: dump a vendor module's exports (NID -> vaddr) and pick out named ones.
//! Used to bootstrap a real hardware payload from getpid's known runtime address.
use std::env;

fn main() {
    let path = env::args().nth(1).expect("path to .sprx/.elf");
    let wanted: Vec<String> = env::args().skip(2).collect();
    let bytes = std::fs::read(&path).expect("read");
    let elf = selfish_elf::Elf::parse(&bytes).expect("parse");
    let (segment, info) = elf.tables().expect("tables").expect("has tables");
    let strings = selfish_elf::dynamic::strings(segment, &info);
    let symbols = selfish_elf::dynamic::symbols(segment, &info).expect("symbols");

    let mut exports = 0usize;
    for s in &symbols {
        if s.is_import() || s.name_offset == 0 || s.value == 0 {
            continue;
        }
        exports = exports.saturating_add(1);
        let name = selfish_elf::dynamic::string_at(strings, s.name_offset).unwrap_or("");
        if wanted.is_empty() {
            if exports <= 20 {
                println!("{:#018x}  {}", s.value, name);
            }
        } else if wanted.iter().any(|w| name.starts_with(w.as_str())) {
            println!("{:#018x}  {}", s.value, name);
        }
    }
    eprintln!("total exports: {exports}");
}
