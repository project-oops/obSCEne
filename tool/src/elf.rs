//! Reading ELF64, including the vendor extensions a console loader expects.
//!
//! Deliberately small: this reads what the tool needs and refuses everything else,
//! rather than becoming a general ELF library. A parser that accepts more than it
//! understands is a parser that reports success on a file it silently misread.
//!
//! # Nothing here trusts a length
//!
//! Every read is bounds-checked against the file. These files come from a linker most
//! of the time and from a console the rest, and a malformed one should produce an error
//! naming the field rather than a panic naming a line number.

use std::fmt;

/// FreeBSD. The target kernel is FreeBSD-derived and its loader checks for this.
pub const ELFOSABI_FREEBSD: u8 = 0x09;

/// `e_type` offset.
pub const E_TYPE_OFFSET: usize = 0x10;

/// The vendor's shared-**library** type: what a `.prx` is.
///
/// A loader that distinguishes the two runs a library's initialisers and then looks
/// elsewhere for a process to start, so a module marked this way loads, relocates, runs
/// `DT_INIT` and is never entered. Accepted everywhere, executed only by loaders that do not
/// check.
pub const ET_SCE_DYNAMIC: u16 = 0xFE18;

/// A loadable segment.
pub const PT_LOAD: u32 = 1;
/// The ordinary dynamic segment.
pub const PT_DYNAMIC: u32 = 2;
/// The vendor segment carrying the tables a loader actually reads.
pub const PT_SCE_DYNLIBDATA: u32 = 0x6100_0000;

/// Marks the end of a dynamic table.
pub const DT_NULL: u64 = 0;

/// Size of one symbol table entry.
pub const SYM_SIZE: usize = 24;

/// A parsed ELF64 file, borrowing its bytes.
pub struct Elf<'a> {
    /// The whole file.
    ///
    /// Public because a finished module has no section headers to reach its tables
    /// through - the only route in is a program header's offset into these bytes.
    pub bytes: &'a [u8],
    /// Program headers, in file order.
    pub program_headers: Vec<ProgramHeader>,
    /// Section headers, in file order. Empty in a loaded module, which has none.
    pub sections: Vec<SectionHeader>,
    /// Entry point, unadjusted.
    pub entry: u64,
    /// `e_type` as found.
    pub e_type: u16,
}

/// One program header.
///
/// Every field the format defines is kept, including those nothing reads yet. A header
/// struct that omits fields because no caller wants them is one that becomes wrong the
/// moment a caller does, and re-adding a field means re-deriving every offset around it.
#[allow(
    dead_code,
    reason = "a faithful record of the format, not a needs-driven subset"
)]
#[derive(Debug, Clone, Copy)]
pub struct ProgramHeader {
    /// Segment type.
    pub p_type: u32,
    /// Permission flags.
    pub flags: u32,
    /// Offset in the file.
    pub offset: u64,
    /// Virtual address once loaded.
    pub vaddr: u64,
    /// Bytes present in the file.
    pub filesz: u64,
    /// Bytes occupied in memory, which may exceed `filesz`.
    pub memsz: u64,
    /// Required alignment.
    pub align: u64,
}

/// One section header. Complete for the same reason as [`ProgramHeader`].
#[allow(
    dead_code,
    reason = "a faithful record of the format, not a needs-driven subset"
)]
#[derive(Debug, Clone)]
pub struct SectionHeader {
    /// Section name, resolved from the section string table.
    pub name: String,
    /// Section type.
    pub sh_type: u32,
    /// Offset in the file.
    pub offset: u64,
    /// Size in bytes.
    pub size: u64,
    /// Section this one links to, meaning varies by type.
    pub link: u32,
    /// Size of one entry, for sections that hold a table.
    pub entsize: u64,
    /// Virtual address once loaded.
    pub addr: u64,
}

/// One dynamic table entry.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DynamicEntry {
    /// The tag.
    pub tag: u64,
    /// Its value: an offset, a size, a count, or a constant, depending on the tag.
    pub value: u64,
}

impl<'a> Elf<'a> {
    /// Parses the headers of an ELF64 little-endian file.
    pub fn parse(bytes: &'a [u8]) -> Result<Self, ElfError> {
        if bytes.get(..4) != Some(b"\x7fELF") {
            return Err(ElfError::NotElf);
        }
        if bytes.get(4) != Some(&2) {
            return Err(ElfError::Not64Bit);
        }
        if bytes.get(5) != Some(&1) {
            return Err(ElfError::NotLittleEndian);
        }

        let e_type = read_u16(bytes, E_TYPE_OFFSET)?;
        let entry = read_u64(bytes, 0x18)?;
        let e_phoff = read_u64(bytes, 0x20)?;
        let e_shoff = read_u64(bytes, 0x28)?;
        let e_phentsize = read_u16(bytes, 0x36)?;
        let e_phnum = read_u16(bytes, 0x38)?;
        let e_shentsize = read_u16(bytes, 0x3A)?;
        let e_shnum = read_u16(bytes, 0x3C)?;
        let e_shstrndx = read_u16(bytes, 0x3E)?;

        let mut program_headers = Vec::with_capacity(e_phnum as usize);
        for index in 0..u64::from(e_phnum) {
            let at = usize_at(e_phoff, index, u64::from(e_phentsize))?;
            program_headers.push(ProgramHeader {
                p_type: read_u32(bytes, at)?,
                flags: read_u32(bytes, at.saturating_add(4))?,
                offset: read_u64(bytes, at.saturating_add(8))?,
                vaddr: read_u64(bytes, at.saturating_add(16))?,
                filesz: read_u64(bytes, at.saturating_add(32))?,
                memsz: read_u64(bytes, at.saturating_add(40))?,
                align: read_u64(bytes, at.saturating_add(48))?,
            });
        }

        // A module loaded from a console has no section headers. That is not an error
        // and the tool must work either way, so an absent table yields an empty list
        // rather than a failure.
        let mut sections = Vec::new();
        if e_shoff != 0 && e_shnum != 0 {
            let raw = read_sections(bytes, e_shoff, e_shentsize, e_shnum)?;
            let names = section_name_table(bytes, &raw, e_shstrndx);
            sections = raw
                .into_iter()
                .map(|(name_offset, mut header)| {
                    header.name = names
                        .and_then(|table| c_string(table, name_offset as usize))
                        .unwrap_or_default();
                    header
                })
                .collect();
        }

        Ok(Self {
            bytes,
            program_headers,
            sections,
            entry,
            e_type,
        })
    }

    /// The first program header of a given type.
    #[must_use]
    pub fn segment(&self, p_type: u32) -> Option<&ProgramHeader> {
        self.program_headers.iter().find(|p| p.p_type == p_type)
    }

    /// A section by name.
    #[must_use]
    pub fn section(&self, name: &str) -> Option<&SectionHeader> {
        self.sections.iter().find(|s| s.name == name)
    }

    /// The bytes a section covers.
    #[must_use]
    pub fn section_data(&self, section: &SectionHeader) -> Option<&'a [u8]> {
        let start = usize::try_from(section.offset).ok()?;
        let len = usize::try_from(section.size).ok()?;
        self.bytes.get(start..start.checked_add(len)?)
    }

    /// The bytes a segment covers in the file.
    #[must_use]
    pub fn segment_data(&self, header: &ProgramHeader) -> Option<&'a [u8]> {
        let start = usize::try_from(header.offset).ok()?;
        let len = usize::try_from(header.filesz).ok()?;
        self.bytes.get(start..start.checked_add(len)?)
    }

    /// Every entry in the dynamic table, stopping at `DT_NULL`.
    pub fn dynamic(&self) -> Result<Vec<DynamicEntry>, ElfError> {
        let Some(header) = self.segment(PT_DYNAMIC) else {
            return Ok(Vec::new());
        };
        let data = self
            .segment_data(header)
            .ok_or(ElfError::SegmentOutOfBounds { p_type: PT_DYNAMIC })?;
        let mut out = Vec::new();
        let mut at = 0_usize;
        while at.saturating_add(16) <= data.len() {
            let tag = read_u64(data, at)?;
            let value = read_u64(data, at.saturating_add(8))?;
            at = at.saturating_add(16);
            if tag == DT_NULL {
                break;
            }
            out.push(DynamicEntry { tag, value });
        }
        Ok(out)
    }

    /// Undefined dynamic symbols, which are exactly the imports.
    ///
    /// Read from section headers, so this works on a linker's output and not on a
    /// module recovered from memory. That is the case it is needed for.
    pub fn undefined_symbols(&self) -> Result<Vec<String>, ElfError> {
        let Some(symtab) = self.section(".dynsym") else {
            return Ok(Vec::new());
        };
        let strtab = self
            .sections
            .get(symtab.link as usize)
            .ok_or(ElfError::MissingStringTable)?;
        let symbols = self
            .section_data(symtab)
            .ok_or(ElfError::MissingStringTable)?;
        let names = self
            .section_data(strtab)
            .ok_or(ElfError::MissingStringTable)?;

        let mut out = Vec::new();
        let mut at = 0_usize;
        while at.saturating_add(SYM_SIZE) <= symbols.len() {
            let st_name = read_u32(symbols, at)?;
            let st_shndx = read_u16(symbols, at.saturating_add(6))?;
            at = at.saturating_add(SYM_SIZE);
            // Undefined means the loader supplies it: an import.
            if st_shndx != 0 || st_name == 0 {
                continue;
            }
            if let Some(name) = c_string(names, st_name as usize)
                && !name.is_empty()
            {
                out.push(name);
            }
        }
        out.sort_unstable();
        out.dedup();
        Ok(out)
    }
}

/// Reads the raw section table, pairing each header with its unresolved name offset.
fn read_sections(
    bytes: &[u8],
    e_shoff: u64,
    e_shentsize: u16,
    e_shnum: u16,
) -> Result<Vec<(u32, SectionHeader)>, ElfError> {
    let mut out = Vec::with_capacity(e_shnum as usize);
    for index in 0..u64::from(e_shnum) {
        let at = usize_at(e_shoff, index, u64::from(e_shentsize))?;
        out.push((
            read_u32(bytes, at)?,
            SectionHeader {
                name: String::new(),
                sh_type: read_u32(bytes, at.saturating_add(4))?,
                addr: read_u64(bytes, at.saturating_add(16))?,
                offset: read_u64(bytes, at.saturating_add(24))?,
                size: read_u64(bytes, at.saturating_add(32))?,
                link: read_u32(bytes, at.saturating_add(40))?,
                entsize: read_u64(bytes, at.saturating_add(56))?,
            },
        ));
    }
    Ok(out)
}

/// The section holding section names, if the index is usable.
fn section_name_table<'a>(
    bytes: &'a [u8],
    raw: &[(u32, SectionHeader)],
    e_shstrndx: u16,
) -> Option<&'a [u8]> {
    let (_, header) = raw.get(e_shstrndx as usize)?;
    let start = usize::try_from(header.offset).ok()?;
    let len = usize::try_from(header.size).ok()?;
    bytes.get(start..start.checked_add(len)?)
}

/// A NUL-terminated string at an offset into a table.
fn c_string(table: &[u8], at: usize) -> Option<String> {
    let rest = table.get(at..)?;
    let end = rest.iter().position(|b| *b == 0).unwrap_or(rest.len());
    let text = rest.get(..end)?;
    Some(String::from_utf8_lossy(text).into_owned())
}

/// `base + index * stride`, as a `usize`, refusing anything that does not fit.
fn usize_at(base: u64, index: u64, stride: u64) -> Result<usize, ElfError> {
    let offset = index
        .checked_mul(stride)
        .and_then(|scaled| base.checked_add(scaled))
        .ok_or(ElfError::OffsetOverflow)?;
    usize::try_from(offset).map_err(|_| ElfError::OffsetOverflow)
}

fn read_u16(bytes: &[u8], at: usize) -> Result<u16, ElfError> {
    let slice = bytes
        .get(at..at.saturating_add(2))
        .ok_or(ElfError::Truncated { at })?;
    let mut buf = [0_u8; 2];
    buf.copy_from_slice(slice);
    Ok(u16::from_le_bytes(buf))
}

fn read_u32(bytes: &[u8], at: usize) -> Result<u32, ElfError> {
    let slice = bytes
        .get(at..at.saturating_add(4))
        .ok_or(ElfError::Truncated { at })?;
    let mut buf = [0_u8; 4];
    buf.copy_from_slice(slice);
    Ok(u32::from_le_bytes(buf))
}

fn read_u64(bytes: &[u8], at: usize) -> Result<u64, ElfError> {
    let slice = bytes
        .get(at..at.saturating_add(8))
        .ok_or(ElfError::Truncated { at })?;
    let mut buf = [0_u8; 8];
    buf.copy_from_slice(slice);
    Ok(u64::from_le_bytes(buf))
}

/// Why a file could not be read.
#[derive(Debug, PartialEq, Eq)]
pub enum ElfError {
    /// No ELF magic.
    NotElf,
    /// Not a 64-bit file.
    Not64Bit,
    /// Not little-endian.
    NotLittleEndian,
    /// A read ran past the end of the file.
    Truncated {
        /// Where the read started.
        at: usize,
    },
    /// A header's offset and size do not fit in this address space.
    OffsetOverflow,
    /// A segment's declared extent is not inside the file.
    SegmentOutOfBounds {
        /// The segment type.
        p_type: u32,
    },
    /// A symbol table exists without the string table it needs.
    MissingStringTable,
}

impl fmt::Display for ElfError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::NotElf => write!(f, "not an ELF file"),
            Self::Not64Bit => write!(f, "not a 64-bit ELF"),
            Self::NotLittleEndian => write!(f, "not little-endian"),
            Self::Truncated { at } => write!(f, "file ends inside a header at offset {at:#x}"),
            Self::OffsetOverflow => write!(f, "a header offset does not fit in this address space"),
            Self::SegmentOutOfBounds { p_type } => {
                write!(f, "segment {p_type:#x} extends past the end of the file")
            }
            Self::MissingStringTable => write!(f, "a symbol table has no usable string table"),
        }
    }
}

impl std::error::Error for ElfError {}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    clippy::arithmetic_side_effects,
    clippy::cast_possible_truncation,
    reason = "test fixtures build known-size buffers; a panic here is the failure               signal, which is the opposite of what these lints guard in the tool"
)]
mod tests {
    use super::{Elf, ElfError, PT_DYNAMIC, PT_LOAD};
    use selfish_elf::identity::ET_DYN;

    /// A minimal well-formed ELF64. Generated, never extracted.
    fn minimal(phdrs: &[(u32, u64, u64)]) -> Vec<u8> {
        const EHDR: usize = 64;
        const PHDR: usize = 56;
        let phoff = EHDR;
        let mut out = vec![0_u8; EHDR + PHDR * phdrs.len()];
        out[0..4].copy_from_slice(b"\x7fELF");
        out[4] = 2;
        out[5] = 1;
        out[0x10..0x12].copy_from_slice(&ET_DYN.to_le_bytes());
        out[0x18..0x20].copy_from_slice(&0x1234_u64.to_le_bytes());
        out[0x20..0x28].copy_from_slice(&(phoff as u64).to_le_bytes());
        out[0x36..0x38].copy_from_slice(&(PHDR as u16).to_le_bytes());
        out[0x38..0x3A].copy_from_slice(&(phdrs.len() as u16).to_le_bytes());
        for (index, (p_type, offset, filesz)) in phdrs.iter().enumerate() {
            let at = phoff + index * PHDR;
            out[at..at + 4].copy_from_slice(&p_type.to_le_bytes());
            out[at + 8..at + 16].copy_from_slice(&offset.to_le_bytes());
            out[at + 32..at + 40].copy_from_slice(&filesz.to_le_bytes());
        }
        out
    }

    #[test]
    fn a_minimal_file_parses_its_headers() {
        let bytes = minimal(&[(PT_LOAD, 0, 64)]);
        let elf = Elf::parse(&bytes).expect("parse");
        assert_eq!(elf.e_type, ET_DYN);
        assert_eq!(elf.entry, 0x1234);
        assert_eq!(elf.program_headers.len(), 1);
        assert_eq!(elf.segment(PT_LOAD).expect("load").p_type, PT_LOAD);
        assert!(elf.segment(PT_DYNAMIC).is_none());
    }

    #[test]
    fn a_file_without_section_headers_is_read_rather_than_refused() {
        // A module recovered from a console has none, and refusing would make the tool
        // useless on exactly the files it most needs to read.
        let bytes = minimal(&[(PT_LOAD, 0, 64)]);
        let elf = Elf::parse(&bytes).expect("parse");
        assert!(elf.sections.is_empty());
        assert_eq!(
            elf.undefined_symbols().expect("symbols"),
            Vec::<String>::new()
        );
    }

    #[test]
    fn a_truncated_header_names_the_offset_rather_than_panicking() {
        let mut bytes = minimal(&[(PT_LOAD, 0, 64)]);
        bytes.truncate(70);
        assert!(matches!(
            Elf::parse(&bytes),
            Err(ElfError::Truncated { .. })
        ));
    }

    #[test]
    fn non_elf_input_is_rejected_on_its_first_four_bytes() {
        // `matches!` rather than `assert_eq!`: the success type borrows the input and
        // deriving Debug on it to satisfy a test would be the test dictating the API.
        assert!(matches!(
            Elf::parse(b"not an elf at all"),
            Err(ElfError::NotElf)
        ));
        assert!(matches!(Elf::parse(&[]), Err(ElfError::NotElf)));
    }

    #[test]
    fn a_dynamic_segment_outside_the_file_is_an_error_not_a_panic() {
        let bytes = minimal(&[(PT_DYNAMIC, 0xFFFF, 0x100)]);
        let elf = Elf::parse(&bytes).expect("headers still parse");
        assert!(matches!(
            elf.dynamic(),
            Err(ElfError::SegmentOutOfBounds { .. })
        ));
    }

    #[test]
    fn a_dynamic_table_stops_at_its_terminator() {
        let mut bytes = minimal(&[(PT_DYNAMIC, 128, 48)]);
        bytes.resize(128, 0);
        // Two real entries, then DT_NULL, then a value that must not be read.
        for (tag, value) in [(0x6100_0035_u64, 0x40_u64), (0, 0), (0x99, 0x99)] {
            bytes.extend_from_slice(&tag.to_le_bytes());
            bytes.extend_from_slice(&value.to_le_bytes());
        }
        let elf = Elf::parse(&bytes).expect("parse");
        let entries = elf.dynamic().expect("dynamic");
        assert_eq!(entries.len(), 1, "reading past DT_NULL invents entries");
        assert_eq!(entries[0].tag, 0x6100_0035);
        assert_eq!(entries[0].value, 0x40);
    }
}
