//! Extract ABI structure layouts from hardware prologue dumps and out-param buffers.
//!
//! # Why this exists
//!
//! `oops-sdk` provides headers for platform subsystems (e.g. `oops/videodec.h`,
//! `oops/audiodec.h`, `oops/keyboard.h`, `oops/mouse.h`), but adhering to CONVENTIONS §1
//! ("nothing is invented"), it will not ship guessed struct layouts.
//!
//! This tool inspects 256-byte machine code function prologues dumped directly from real
//! hardware (`OBS|bytes|...`), decodes `x86_64` memory access instructions
//! referencing System V AMD64 argument registers (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`),
//! and parses measured out-parameter extents to generate
//! an authoritative, citable `decode-layout.tsv` (`OBS_FROM_HARDWARE`).

use std::collections::{BTreeMap, BTreeSet};
use std::fmt::Write as _;
use std::fs;
use std::io::{self, Write};
use std::path::Path;

/// An argument structure access observed from machine code or an out-param buffer.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct ArgAccess {
    /// Function export symbol name.
    pub symbol: String,
    /// 0-indexed argument index (0 for `rdi`, 1 for `rsi`, etc.).
    pub arg_index: u8,
    /// Byte offset within the structure.
    pub offset: i64,
    /// Access width in bytes (1, 2, 4, 8, 16).
    pub width: usize,
    /// Type of access ("read", "write", "addr", or "out-param").
    pub access: &'static str,
    /// Source provenance (`"OBS_FROM_HARDWARE"`).
    pub provenance: &'static str,
}

/// Convert hex string to byte vector.
fn hex_to_bytes(text: &str) -> Vec<u8> {
    if text.is_empty() || !text.len().is_multiple_of(2) {
        return Vec::new();
    }
    let bytes = text.as_bytes();
    let mut out = Vec::with_capacity(text.len() / 2);
    let mut index = 0;
    while index < bytes.len() {
        let Some(slice) = bytes.get(index..index.saturating_add(2)) else {
            break;
        };
        let Ok(pair) = std::str::from_utf8(slice) else {
            break;
        };
        let Ok(val) = u8::from_str_radix(pair, 16) else {
            break;
        };
        out.push(val);
        index = index.saturating_add(2);
    }
    out
}

/// Disassemble a 256-byte function prologue to find argument structure accesses.
#[allow(clippy::too_many_lines)]
fn scan_prologue(symbol: &str, code: &[u8]) -> Vec<ArgAccess> {
    let mut accesses = Vec::new();
    let mut aliases: BTreeMap<u8, u8> = BTreeMap::new();

    // Map System V AMD64 argument registers to arg index:
    // rdi = 7, rsi = 6, rdx = 2, rcx = 1, r8 = 8, r9 = 9
    let reg_to_arg: BTreeMap<u8, u8> = [
        (7u8, 0u8),
        (6u8, 1u8),
        (2u8, 2u8),
        (1u8, 3u8),
        (8u8, 4u8),
        (9u8, 5u8),
    ]
    .into_iter()
    .collect();

    let mut pos = 0;
    while pos < code.len() {
        let mut prefix_op_size = false;
        let mut rex_w = false;
        let mut rex_r = false;
        let mut rex_b = false;

        // 1. Consume legacy prefixes and REX prefix
        while let Some(&b) = code.get(pos) {
            if b == 0x66 {
                prefix_op_size = true;
                pos = pos.saturating_add(1);
            } else if (0x40..=0x4F).contains(&b) {
                rex_w = (b & 0x08) != 0;
                rex_r = (b & 0x04) != 0;
                rex_b = (b & 0x01) != 0;
                pos = pos.saturating_add(1);
            } else if matches!(
                b,
                0x2E | 0x36 | 0x3E | 0x26 | 0x64 | 0x65 | 0xF0 | 0xF2 | 0xF3 | 0x67
            ) {
                pos = pos.saturating_add(1);
            } else {
                break;
            }
        }

        if pos >= code.len() {
            break;
        }

        let Some(&op1) = code.get(pos) else { break };
        pos = pos.saturating_add(1);

        let (opcode, is_two_byte) = if op1 == 0x0F && pos < code.len() {
            let Some(&op2) = code.get(pos) else { break };
            pos = pos.saturating_add(1);
            (op2, true)
        } else {
            (op1, false)
        };

        // Determine width and access type based on opcode
        let mut default_width = if rex_w {
            8usize
        } else if prefix_op_size {
            2usize
        } else {
            4usize
        };
        let mut access_type = "read";
        let mut has_modrm = false;

        if is_two_byte {
            match opcode {
                0xB6 => {
                    // movzx r, r/m8
                    has_modrm = true;
                    default_width = 1;
                    access_type = "read";
                }
                0xB7 => {
                    // movzx r, r/m16
                    has_modrm = true;
                    default_width = 2;
                    access_type = "read";
                }
                0xBE => {
                    // movsx r, r/m8
                    has_modrm = true;
                    default_width = 1;
                    access_type = "read";
                }
                0xBF => {
                    // movsx r, r/m16
                    has_modrm = true;
                    default_width = 2;
                    access_type = "read";
                }
                0x10 | 0x28 => {
                    // movups / movaps xmm, m128
                    has_modrm = true;
                    default_width = 16;
                    access_type = "read";
                }
                0x11 | 0x29 => {
                    // movups / movaps m128, xmm
                    has_modrm = true;
                    default_width = 16;
                    access_type = "write";
                }
                0xAF => {
                    // imul r, r/m
                    has_modrm = true;
                    access_type = "read";
                }
                _ => {}
            }
        } else {
            match opcode {
                0x88 => {
                    // mov r/m8, r8
                    has_modrm = true;
                    default_width = 1;
                    access_type = "write";
                }
                0x89 => {
                    // mov r/m, r
                    has_modrm = true;
                    access_type = "write";
                }
                0x8A => {
                    // mov r8, r/m8
                    has_modrm = true;
                    default_width = 1;
                    access_type = "read";
                }
                0x8B => {
                    // mov r, r/m
                    has_modrm = true;
                    access_type = "read";
                }
                0x8D => {
                    // lea r, [r/m]
                    has_modrm = true;
                    access_type = "addr";
                }
                0xC6 => {
                    // mov r/m8, imm8
                    has_modrm = true;
                    default_width = 1;
                    access_type = "write";
                }
                0xC7 => {
                    // mov r/m, imm32
                    has_modrm = true;
                    access_type = "write";
                }
                0x38..=0x3B => {
                    // cmp
                    has_modrm = true;
                    access_type = "read";
                }
                0x84 | 0x85 => {
                    // test
                    has_modrm = true;
                    access_type = "read";
                }
                0x80 | 0x81 | 0x83 => {
                    // arithmetic r/m, imm
                    has_modrm = true;
                    if opcode == 0x80 {
                        default_width = 1;
                    }
                    access_type = "read";
                }
                _ => {}
            }
        }

        if !has_modrm || pos >= code.len() {
            continue;
        }

        let Some(&modrm) = code.get(pos) else { break };
        pos = pos.saturating_add(1);

        let mode = (modrm >> 6) & 0x03;
        let reg = ((modrm >> 3) & 0x07) | if rex_r { 8 } else { 0 };
        let rm = (modrm & 0x07) | if rex_b { 8 } else { 0 };

        if mode == 3 {
            // Register-to-register move: check if an argument register is copied to a scratch reg
            if opcode == 0x89 {
                // mov rm, reg -> rm gets reg
                if let Some(&arg) = reg_to_arg.get(&reg).or_else(|| aliases.get(&reg)) {
                    aliases.insert(rm, arg);
                }
            } else if opcode == 0x8B {
                // mov reg, rm -> reg gets rm
                if let Some(&arg) = reg_to_arg.get(&rm).or_else(|| aliases.get(&rm)) {
                    aliases.insert(reg, arg);
                }
            }
            continue;
        }

        // Memory operand
        let (base_reg, has_sib) = if (modrm & 0x07) == 4 {
            // SIB byte
            if pos >= code.len() {
                break;
            }
            let Some(&sib) = code.get(pos) else { break };
            pos = pos.saturating_add(1);
            let sib_base = (sib & 0x07) | if rex_b { 8 } else { 0 };
            (sib_base, true)
        } else {
            (rm, false)
        };

        let disp: i64 = match mode {
            0 => {
                if !has_sib && (modrm & 0x07) == 5 {
                    // RIP-relative: skip 4 bytes
                    pos = pos.saturating_add(4);
                    continue;
                }
                0
            }
            1 => {
                if pos >= code.len() {
                    break;
                }
                let Some(&b) = code.get(pos) else { break };
                pos = pos.saturating_add(1);
                let d = b.cast_signed();
                i64::from(d)
            }
            2 => {
                if pos.saturating_add(4) > code.len() {
                    break;
                }
                let b0 = u32::from(*code.get(pos).unwrap_or(&0));
                let b1 = u32::from(*code.get(pos.saturating_add(1)).unwrap_or(&0));
                let b2 = u32::from(*code.get(pos.saturating_add(2)).unwrap_or(&0));
                let b3 = u32::from(*code.get(pos.saturating_add(3)).unwrap_or(&0));
                pos = pos.saturating_add(4);
                #[allow(clippy::cast_possible_wrap)]
                let val = ((b3 << 24) | (b2 << 16) | (b1 << 8) | b0) as i32;
                i64::from(val)
            }
            _ => 0,
        };

        // Skip immediate values if applicable
        if !is_two_byte {
            match opcode {
                0xC6 | 0x80 | 0x83 => {
                    pos = pos.saturating_add(1);
                }
                0xC7 | 0x81 => {
                    pos = pos.saturating_add(4);
                }
                _ => {}
            }
        }

        // Check if base_reg matches an argument register or known alias
        let maybe_arg = reg_to_arg
            .get(&base_reg)
            .copied()
            .or_else(|| aliases.get(&base_reg).copied());
        if let Some(arg_idx) = maybe_arg {
            #[allow(clippy::collapsible_if)]
            if (0..65536).contains(&disp) {
                accesses.push(ArgAccess {
                    symbol: symbol.to_string(),
                    arg_index: arg_idx,
                    offset: disp,
                    width: default_width,
                    access: access_type,
                    provenance: "OBS_FROM_HARDWARE",
                });
            }
        }
    }

    accesses
}

/// Process report file and extract structure layout table.
pub fn process_report(report_content: &str) -> Vec<ArgAccess> {
    let mut prologues: BTreeMap<String, BTreeMap<usize, Vec<u8>>> = BTreeMap::new();
    let mut out_params: BTreeMap<String, usize> = BTreeMap::new();

    for line in report_content.lines() {
        if !line.starts_with("OBS|bytes|") {
            continue;
        }
        let parts: Vec<&str> = line.split('|').collect();
        if parts.len() < 7 {
            continue;
        }

        let Some(&symbol) = parts.get(3) else {
            continue;
        };
        let Some(&what) = parts.get(4) else { continue };
        let Some(&offset_str) = parts.get(5) else {
            continue;
        };
        let Some(&hex) = parts.get(6) else { continue };

        let offset = offset_str.parse::<usize>().unwrap_or(0);

        if what == "prologue" || what.ends_with("-prologue") {
            let bytes = hex_to_bytes(hex);
            prologues
                .entry(symbol.to_string())
                .or_default()
                .insert(offset, bytes);
        } else if what == "extent" && offset > 0 {
            out_params.insert(symbol.to_string(), offset);
        }
    }

    let mut results = BTreeSet::new();

    // 1. Disassemble prologues
    for (sym, chunks) in prologues {
        let mut full_code = Vec::new();
        for (_, chunk) in chunks {
            full_code.extend_from_slice(&chunk);
        }
        let accesses = scan_prologue(&sym, &full_code);
        for acc in accesses {
            results.insert(acc);
        }
    }

    // 2. Add out-param extent measurements
    for (sym, extent) in out_params {
        results.insert(ArgAccess {
            symbol: sym,
            arg_index: 0,
            offset: 0,
            width: extent,
            access: "out-param",
            provenance: "OBS_FROM_HARDWARE",
        });
    }

    results.into_iter().collect()
}

/// Run the decode-layout subcommand.
pub fn run(report_path: &Path, out_path: Option<&Path>) -> Result<(), String> {
    let content = fs::read_to_string(report_path)
        .map_err(|e| format!("cannot read report {}: {e}", report_path.display()))?;

    let accesses = process_report(&content);

    let mut output = String::from("symbol\targ_index\toffset\twidth\taccess\tprovenance\n");
    for a in accesses {
        let _ = writeln!(
            output,
            "{}\t{}\t{}\t{}\t{}\t{}",
            a.symbol, a.arg_index, a.offset, a.width, a.access, a.provenance
        );
    }

    if let Some(path) = out_path {
        fs::write(path, output.as_bytes())
            .map_err(|e| format!("cannot write layout to {}: {e}", path.display()))?;
        println!(
            "wrote {} layout rows to {}",
            output.lines().count().saturating_sub(1),
            path.display()
        );
    } else {
        let stdout = io::stdout();
        let mut handle = stdout.lock();
        handle
            .write_all(output.as_bytes())
            .map_err(|e| format!("cannot write to stdout: {e}"))?;
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn decodes_arg_read_write_and_addr() {
        // mov rax, [rdi + 0x10] -> 48 8b 47 10
        // mov [rdi + 0x04], esi -> 89 77 04
        // lea rdx, [rsi + 0x20] -> 48 8d 56 20
        let code = vec![
            0x48, 0x8b, 0x47, 0x10, 0x89, 0x77, 0x04, 0x48, 0x8d, 0x56, 0x20,
        ];
        let accesses = scan_prologue("test_func", &code);
        assert_eq!(accesses.len(), 3);

        assert_eq!(
            accesses.first(),
            Some(&ArgAccess {
                symbol: "test_func".to_string(),
                arg_index: 0,
                offset: 16,
                width: 8,
                access: "read",
                provenance: "OBS_FROM_HARDWARE",
            })
        );
        assert_eq!(
            accesses.get(1),
            Some(&ArgAccess {
                symbol: "test_func".to_string(),
                arg_index: 0,
                offset: 4,
                width: 4,
                access: "write",
                provenance: "OBS_FROM_HARDWARE",
            })
        );
        assert_eq!(
            accesses.get(2),
            Some(&ArgAccess {
                symbol: "test_func".to_string(),
                arg_index: 1,
                offset: 32,
                width: 8,
                access: "addr",
                provenance: "OBS_FROM_HARDWARE",
            })
        );
    }

    #[test]
    fn parses_out_param_extent_from_report() {
        let report = "OBS|bytes|001|test_out|extent|96|\n";
        let accesses = process_report(report);
        assert_eq!(accesses.len(), 1);
        assert_eq!(
            accesses.first(),
            Some(&ArgAccess {
                symbol: "test_out".to_string(),
                arg_index: 0,
                offset: 0,
                width: 96,
                access: "out-param",
                provenance: "OBS_FROM_HARDWARE",
            })
        );
    }
}
