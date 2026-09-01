//! The export table of a platform module, as a citable data file.
//!
//! # What this is for
//!
//! A payload that escapes into a platform library has to find its functions, and it cannot
//! walk the export table at run time: libkernel is callable but not *readable* from a sandbox
//! (`0xa0020328` on every read - D209). So the addresses have to come from somewhere else
