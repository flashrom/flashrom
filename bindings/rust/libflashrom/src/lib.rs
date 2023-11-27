/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 The Chromium OS Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

//! # libflashrom
//!
//! The `libflashrom` library is a rust FFI binding to the flashrom library.
//! libflashrom can be used to read write and modify some settings of flash chips.
//! The library closely follows the libflashrom C API, but exports a `safe` interface
//! including automatic resource management and forced error checking.
//!
//! libflashrom does not support threading, all usage of this library must occur on one thread.
//!
//! Most of the library functionality is defined on the [`Chip`] type.
//!
//! Example:
//!
//! ```
//! use libflashrom::*;
//! let mut chip = Chip::new(
//!     Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap(),
//!     Some("W25Q128.V")
//! ).unwrap();
//! let mut buf = chip.image_read(None).unwrap();
//! buf[0] = 0xFE;
//! chip.image_write(&mut buf, None).unwrap();
//! ```

use once_cell::sync::Lazy;
use regex::Regex;
use std::error;
use std::ffi::c_void;
use std::ffi::CStr;
use std::ffi::CString;
use std::fmt;
use std::io::Write;
use std::ptr::null;
use std::ptr::null_mut;
use std::ptr::NonNull;
use std::sync::Mutex;
use std::sync::Once;

pub use libflashrom_sys::{
    flashrom_log_level, FLASHROM_MSG_DEBUG, FLASHROM_MSG_DEBUG2, FLASHROM_MSG_ERROR,
    FLASHROM_MSG_INFO, FLASHROM_MSG_SPEW, FLASHROM_MSG_WARN,
};

pub use libflashrom_sys::flashrom_wp_mode;

// libflashrom uses (start, len) or inclusive [start, end] for ranges.
// This type exists to rust RangeBounds types to a convenient internal format.
#[derive(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
struct RangeInternal {
    start: usize,
    len: usize,
}

/// The type returned for write protect and layout queries
pub type Range = std::ops::Range<usize>;

impl<T> From<T> for RangeInternal
where
    T: std::ops::RangeBounds<usize>,
{
    fn from(range: T) -> Self {
        let start = match range.start_bound() {
            std::ops::Bound::Included(start) => *start,
            std::ops::Bound::Excluded(start) => *start + 1,
            std::ops::Bound::Unbounded => 0,
        };
        RangeInternal {
            start,
            len: match range.end_bound() {
                std::ops::Bound::Included(end) => *end - start + 1,
                std::ops::Bound::Excluded(end) => *end - start,
                std::ops::Bound::Unbounded => usize::MAX - start,
            },
        }
    }
}

impl RangeInternal {
    // inclusive end for libflashrom
    fn end(&self) -> usize {
        self.start + self.len - 1
    }
}

// log_c is set to be the callback at [`Programmer`] init. It deals with va_list and calls log_rust.
// log_rust calls a user defined function, or by default log_eprint.
// log_eprint just writes to stderr.
extern "C" {
    fn set_log_callback();
    // Modifying and reading current_level is not thread safe, but neither is
    // the libflashrom implementation, so we shouldnt be using threads anyway.
    static mut current_level: libflashrom_sys::flashrom_log_level;
}

/// Callers can use this function to log to the [`Logger`] they have set via [`set_log_level`]
///
/// However from rust it is likely easier to call the [`Logger`] directly.
#[no_mangle]
pub extern "C" fn log_rust(
    level: libflashrom_sys::flashrom_log_level,
    format: &std::os::raw::c_char,
) {
    // Because this function is called from C, it must not panic.
    // SAFETY: log_c always provides a non null ptr to a null terminated string
    // msg does not outlive format.
    let msg = unsafe { CStr::from_ptr(format) }.to_string_lossy();
    // Locking can fail if a thread panics while holding the lock.
    match LOG_FN.lock() {
        Ok(g) => (*g)(level, msg.as_ref()),
        Err(_) => eprintln!("ERROR: libflashrom log failure to lock function"),
    };
}

fn log_eprint(_level: libflashrom_sys::flashrom_log_level, msg: &str) {
    // Because this function is called from C, it must not panic.
    // Ignore the error.
    let _ = std::io::stderr().write_all(msg.as_bytes());
}

// Can't directly atexit(flashrom_shutdown) because it is unsafe
extern "C" fn shutdown_wrapper() {
    unsafe {
        libflashrom_sys::flashrom_shutdown();
    }
}

/// A callback to log flashrom messages. This must not panic.
pub type Logger = fn(libflashrom_sys::flashrom_log_level, &str);

static LOG_FN: Lazy<Mutex<Logger>> = Lazy::new(|| Mutex::new(log_eprint));

/// Set the maximum log message level that will be passed to [`Logger`]
///
/// log_rust and therefore the provided [`Logger`] will only be called for messages
/// greater or equal to the provided priority.
///
/// ```
/// use libflashrom::set_log_level;
/// use libflashrom::FLASHROM_MSG_SPEW;
/// // Disable logging.
/// set_log_level(None);
/// // Log all messages  at priority FLASHROM_MSG_SPEW and above.
/// set_log_level(Some(FLASHROM_MSG_SPEW));
/// ```
pub fn set_log_level(level: Option<libflashrom_sys::flashrom_log_level>) {
    // SAFETY: current_level is only read by log_c, in this thread.
    match level {
        Some(level) => unsafe { current_level = level + 1 },
        None => unsafe { current_level = 0 },
    };
}

/// Set a [`Logger`] logging callback function
///
/// Provided function must not panic, as it is called from C.
pub fn set_log_function(logger: Logger) {
    *LOG_FN.lock().unwrap() = logger;
}

/// A type holding the error code returned by libflashrom and the function that returned the error.
///
/// The error codes returned from each function differ in meaning, see libflashrom.h
#[derive(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct ErrorCode {
    function: &'static str,
    code: i32,
}

impl fmt::Display for ErrorCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "libflashrom: {} returned {}", self.function, self.code)
    }
}

impl error::Error for ErrorCode {}

impl From<ErrorCode> for String {
    fn from(e: ErrorCode) -> Self {
        format!("{}", e)
    }
}

/// Errors from initialising libflashrom or a [`Programmer`]
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum InitError {
    DuplicateInit,
    FlashromInit(ErrorCode),
    InvalidName(std::ffi::NulError),
    ProgrammerInit(ErrorCode),
}

impl fmt::Display for InitError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl error::Error for InitError {}

impl From<InitError> for String {
    fn from(e: InitError) -> Self {
        format!("{:?}", e)
    }
}

impl From<std::ffi::NulError> for InitError {
    fn from(err: std::ffi::NulError) -> InitError {
        InitError::InvalidName(err)
    }
}

/// Errors from probing a [`Chip`]
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum ChipInitError {
    InvalidName(std::ffi::NulError),
    NoChipError,
    MultipleChipsError,
    ProbeError,
}

impl fmt::Display for ChipInitError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl error::Error for ChipInitError {}

impl From<ChipInitError> for String {
    fn from(e: ChipInitError) -> Self {
        format!("{:?}", e)
    }
}

impl From<std::ffi::NulError> for ChipInitError {
    fn from(err: std::ffi::NulError) -> ChipInitError {
        ChipInitError::InvalidName(err)
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum RegionError {
    ErrorCode(ErrorCode),
    InvalidName(std::ffi::NulError),
}

impl fmt::Display for RegionError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl error::Error for RegionError {}

impl From<RegionError> for String {
    fn from(e: RegionError) -> Self {
        format!("{:?}", e)
    }
}

impl From<std::ffi::NulError> for RegionError {
    fn from(err: std::ffi::NulError) -> RegionError {
        RegionError::InvalidName(err)
    }
}

#[derive(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct ParseLayoutError(String);

impl fmt::Display for ParseLayoutError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl error::Error for ParseLayoutError {}

impl From<ParseLayoutError> for String {
    fn from(e: ParseLayoutError) -> Self {
        format!("{:?}", e)
    }
}

/// A translation of the flashrom_wp_result type
///
/// WpOK is omitted, as it is not an error.
/// WpErrUnknown is used for an unknown error type.
/// Keep this list in sync with libflashrom.h
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub enum WPError {
    WpErrChipUnsupported,
    WpErrOther,
    WpErrReadFailed,
    WpErrWriteFailed,
    WpErrVerifyFailed,
    WpErrRangeUnsupported,
    WpErrModeUnsupported,
    WpErrRangeListUnavailable,
    WpErrUnsupportedState,
    WpErrUnknown(libflashrom_sys::flashrom_wp_result),
}

impl From<libflashrom_sys::flashrom_wp_result> for WPError {
    fn from(e: libflashrom_sys::flashrom_wp_result) -> Self {
        assert!(e != libflashrom_sys::flashrom_wp_result::FLASHROM_WP_OK);
        match e {
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_CHIP_UNSUPPORTED => {
                WPError::WpErrChipUnsupported
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_OTHER => WPError::WpErrOther,
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_READ_FAILED => {
                WPError::WpErrReadFailed
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_WRITE_FAILED => {
                WPError::WpErrWriteFailed
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_VERIFY_FAILED => {
                WPError::WpErrVerifyFailed
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_RANGE_UNSUPPORTED => {
                WPError::WpErrRangeUnsupported
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_MODE_UNSUPPORTED => {
                WPError::WpErrModeUnsupported
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_RANGE_LIST_UNAVAILABLE => {
                WPError::WpErrRangeListUnavailable
            }
            libflashrom_sys::flashrom_wp_result::FLASHROM_WP_ERR_UNSUPPORTED_STATE => {
                WPError::WpErrUnsupportedState
            }
            _ => WPError::WpErrUnknown(e), // this could also be a panic
        }
    }
}

impl fmt::Display for WPError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl error::Error for WPError {}

impl From<WPError> for String {
    fn from(e: WPError) -> Self {
        format!("{:?}", e)
    }
}

/// Return a rust sanitised string derived from flashrom_version_info.
pub fn flashrom_version_info() -> Option<String> {
    let p = unsafe { libflashrom_sys::flashrom_version_info() };
    if p.is_null() {
        None
    } else {
        // SAFETY: flashrom_version_info returns a global `const char flashrom_version[]`
        // derived from `-DFLASHROM_VERSION`, this is not guaranteed to be
        // null terminated, but is in a normal build.
        Some(unsafe { CStr::from_ptr(p) }.to_string_lossy().into_owned())
    }
}

/// Structure for an initialised flashrom_programmer
// flashrom_programmer_init returns a pointer accepted by flashrom_flash_probe
// but this is not implemented at time of writing. When implemented the pointer
// can be stored here.
#[derive(Debug)]
pub struct Programmer {}

/// Structure for an initialised flashrom chip, or flashrom_flashctx
// As returned by flashrom_flash_probe
// The layout is owned here as the chip only stores a pointer when a layout is set.
#[derive(Debug)]
pub struct Chip {
    ctx: NonNull<libflashrom_sys::flashrom_flashctx>,
    _programmer: Programmer,
    layout: Option<Layout>,
}

impl Programmer {
    /// Initialise libflashrom and a programmer
    ///
    /// See libflashrom.h flashrom_programmer_init for argument documentation.
    ///
    /// Panics:
    ///
    /// If this libflashrom implementation returns a programmer pointer.
    pub fn new(
        programmer_name: &str,
        programmer_options: Option<&str>,
    ) -> Result<Programmer, InitError> {
        static ONCE: Once = Once::new();
        if ONCE.is_completed() {
            // Flashrom does not currently support concurrent programmers
            // Flashrom also does not currently support initialising a second programmer after a first has been initialised.
            // This is used to warn the user if they try to initialise a second programmer.
            return Err(InitError::DuplicateInit);
        }
        ONCE.call_once(|| {});

        static INIT_RES: Lazy<Result<(), InitError>> = Lazy::new(|| {
            unsafe { set_log_callback() };
            // always perform_selfcheck
            let res = unsafe { libflashrom_sys::flashrom_init(1) };
            if res == 0 {
                let res = unsafe { libc::atexit(shutdown_wrapper) };
                if res == 0 {
                    Ok(())
                } else {
                    unsafe { libflashrom_sys::flashrom_shutdown() };
                    Err(InitError::FlashromInit(ErrorCode {
                        function: "atexit",
                        code: res,
                    }))
                }
            } else {
                Err(InitError::FlashromInit(ErrorCode {
                    function: "flashrom_init",
                    code: res,
                }))
            }
        });
        (*INIT_RES).clone()?;

        let mut programmer: *mut libflashrom_sys::flashrom_programmer = null_mut();
        let programmer_name = CString::new(programmer_name)?;
        let programmer_options = match programmer_options {
            Some(programmer_options) => Some(CString::new(programmer_options)?),
            None => None,
        };
        let res = unsafe {
            libflashrom_sys::flashrom_programmer_init(
                &mut programmer,
                programmer_name.as_ptr(),
                programmer_options.as_ref().map_or(null(), |x| x.as_ptr()),
            )
        };
        if res != 0 {
            Err(InitError::ProgrammerInit(ErrorCode {
                function: "flashrom_programmer_init",
                code: res,
            }))
        } else if !programmer.is_null() {
            panic!("flashrom_programmer_init returning a programmer pointer is not supported")
        } else {
            Ok(Programmer {})
        }
    }
}

impl Drop for Programmer {
    fn drop(&mut self) {
        unsafe {
            libflashrom_sys::flashrom_programmer_shutdown(null_mut());
        }
    }
}

/// A translation of the flashrom_flag type
///
/// Keep this list in sync with libflashrom.h
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub enum FlashromFlag {
    FlashromFlagForce,
    FlashromFlagForceBoardmismatch,
    FlashromFlagVerifyAfterWrite,
    FlashromFlagVerifyWholeChip,
    FlashromFlagSkipUnreadableRegions,
    FlashromFlagSkipUnwritableRegions,
}

impl From<FlashromFlag> for libflashrom_sys::flashrom_flag {
    fn from(e: FlashromFlag) -> Self {
        match e {
            FlashromFlag::FlashromFlagForce => libflashrom_sys::flashrom_flag::FLASHROM_FLAG_FORCE,
            FlashromFlag::FlashromFlagForceBoardmismatch => {
                libflashrom_sys::flashrom_flag::FLASHROM_FLAG_FORCE_BOARDMISMATCH
            }
            FlashromFlag::FlashromFlagVerifyAfterWrite => {
                libflashrom_sys::flashrom_flag::FLASHROM_FLAG_VERIFY_AFTER_WRITE
            }
            FlashromFlag::FlashromFlagVerifyWholeChip => {
                libflashrom_sys::flashrom_flag::FLASHROM_FLAG_VERIFY_WHOLE_CHIP
            }
            FlashromFlag::FlashromFlagSkipUnreadableRegions => {
                libflashrom_sys::flashrom_flag::FLASHROM_FLAG_SKIP_UNREADABLE_REGIONS
            }
            FlashromFlag::FlashromFlagSkipUnwritableRegions => {
                libflashrom_sys::flashrom_flag::FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS
            }
            e => panic!("Unexpected FlashromFlag: {:?}", e),
        }
    }
}

/// Various flags of the flashrom context
///
/// Keep the struct in sync with the flags in flash.h
#[derive(Debug)]
pub struct FlashromFlags {
    pub force: bool,
    pub force_boardmismatch: bool,
    pub verify_after_write: bool,
    pub verify_whole_chip: bool,
    pub skip_unreadable_regions: bool,
    pub skip_unwritable_regions: bool,
}

/// Keep the default values in sync with cli_classic.c
impl Default for FlashromFlags {
    fn default() -> Self {
        Self {
            force: false,
            force_boardmismatch: false,
            verify_after_write: true,
            verify_whole_chip: true,
            // These flags are introduced to address issues related to CSME locking parts. Setting
            // them to false as default values
            skip_unreadable_regions: false,
            skip_unwritable_regions: false,
        }
    }
}

impl fmt::Display for FlashromFlags {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "FlashromFlags {{ force: {}, force_boardmismatch: {}, verify_after_write: {}, verify_whole_chip: {}, skip_unreadable_regions: {}, skip_unwritable_regions: {} }}",
            self.force,
            self.force_boardmismatch,
            self.verify_after_write,
            self.verify_whole_chip,
            self.skip_unreadable_regions,
            self.skip_unwritable_regions,
        )
    }
}

impl Chip {
    /// Probe for a chip
    ///
    /// See libflashrom.h flashrom_flash_probe for argument documentation.
    pub fn new(programmer: Programmer, chip_name: Option<&str>) -> Result<Chip, ChipInitError> {
        let mut flash_ctx: *mut libflashrom_sys::flashrom_flashctx = null_mut();
        let chip_name = match chip_name {
            Some(chip_name) => Some(CString::new(chip_name)?),
            None => None,
        };
        match unsafe {
            libflashrom_sys::flashrom_flash_probe(
                &mut flash_ctx,
                null(),
                chip_name.as_ref().map_or(null(), |x| x.as_ptr()),
            )
        } {
            0 => Ok(Chip {
                ctx: NonNull::new(flash_ctx).expect("flashrom_flash_probe returned null"),
                _programmer: programmer,
                layout: None,
            }),
            3 => Err(ChipInitError::MultipleChipsError),
            2 => Err(ChipInitError::NoChipError),
            _ => Err(ChipInitError::ProbeError),
        }
    }

    pub fn get_size(&self) -> usize {
        unsafe { libflashrom_sys::flashrom_flash_getsize(self.ctx.as_ref()) }
    }

    /// Read the write protect config of this [`Chip`]
    pub fn get_wp(&mut self) -> std::result::Result<WriteProtectCfg, WPError> {
        let mut cfg = WriteProtectCfg::new()?;
        let res =
            unsafe { libflashrom_sys::flashrom_wp_read_cfg(cfg.wp.as_mut(), self.ctx.as_mut()) };
        if res != libflashrom_sys::flashrom_wp_result::FLASHROM_WP_OK {
            return Err(res.into());
        }
        Ok(cfg)
    }

    /// Set the write protect config of this [`Chip`]
    pub fn set_wp(&mut self, wp: &WriteProtectCfg) -> std::result::Result<(), WPError> {
        let res =
            unsafe { libflashrom_sys::flashrom_wp_write_cfg(self.ctx.as_mut(), wp.wp.as_ref()) };
        if res != libflashrom_sys::flashrom_wp_result::FLASHROM_WP_OK {
            return Err(res.into());
        }
        Ok(())
    }

    /// Read the write protect ranges of this [`Chip`]
    ///
    /// # Panics
    ///
    /// Panics if flashrom_wp_get_available_ranges returns FLASHROM_WP_OK and a NULL pointer.
    pub fn get_wp_ranges(&mut self) -> std::result::Result<Vec<Range>, WPError> {
        let mut ranges: *mut libflashrom_sys::flashrom_wp_ranges = null_mut();
        let res = unsafe {
            libflashrom_sys::flashrom_wp_get_available_ranges(&mut ranges, self.ctx.as_mut())
        };
        if res != libflashrom_sys::flashrom_wp_result::FLASHROM_WP_OK {
            return Err(res.into());
        }
        let ranges = WriteProtectRanges {
            ranges: NonNull::new(ranges).expect("flashrom_wp_get_available_ranges returned null"),
        };

        let count =
            unsafe { libflashrom_sys::flashrom_wp_ranges_get_count(ranges.ranges.as_ref()) };
        let mut ret = Vec::with_capacity(count);
        for index in 0..count {
            let mut start = 0;
            let mut len = 0;
            let res = unsafe {
                libflashrom_sys::flashrom_wp_ranges_get_range(
                    &mut start,
                    &mut len,
                    ranges.ranges.as_ref(),
                    // TODO: fix after https://review.coreboot.org/c/flashrom/+/64996
                    index
                        .try_into()
                        .expect("flashrom_wp_ranges_get_count does not fit in a u32"),
                )
            };
            if res != libflashrom_sys::flashrom_wp_result::FLASHROM_WP_OK {
                return Err(res.into());
            }
            ret.push(start..(start + len))
        }
        Ok(ret)
    }

    /// Returns the layout read from the fmap of this [`Chip`]
    ///
    /// # Panics
    ///
    /// Panics if flashrom_layout_read_fmap_from_rom returns FLASHROM_WP_OK and a NULL pointer.
    pub fn layout_read_fmap_from_rom(&mut self) -> std::result::Result<Layout, ErrorCode> {
        let mut layout: *mut libflashrom_sys::flashrom_layout = null_mut();
        let err = unsafe {
            libflashrom_sys::flashrom_layout_read_fmap_from_rom(
                &mut layout,
                self.ctx.as_mut(),
                0,
                self.get_size(),
            )
        };
        if err != 0 {
            return Err(ErrorCode {
                function: "flashrom_layout_read_fmap_from_rom",
                code: err,
            });
        }
        Ok(Layout {
            layout: NonNull::new(layout).expect("flashrom_layout_read_fmap_from_rom returned null"),
        })
    }

    /// Sets the layout of this [`Chip`]
    ///
    /// [`Chip`] takes ownership of Layout to ensure it is not released before the [`Chip`].
    fn set_layout(&mut self, layout: Layout) {
        unsafe { libflashrom_sys::flashrom_layout_set(self.ctx.as_mut(), layout.layout.as_ref()) };
        self.layout = Some(layout)
    }

    fn unset_layout(&mut self) -> Option<Layout> {
        unsafe { libflashrom_sys::flashrom_layout_set(self.ctx.as_mut(), null()) };
        self.layout.take()
    }

    /// Read the whole [`Chip`], or a portion specified in a Layout
    pub fn image_read(
        &mut self,
        layout: Option<Layout>,
    ) -> std::result::Result<Vec<u8>, ErrorCode> {
        if let Some(layout) = layout {
            self.set_layout(layout);
        }
        let len = self.get_size();
        let mut buf = vec![0; len];
        let res = unsafe {
            libflashrom_sys::flashrom_image_read(
                self.ctx.as_mut(),
                buf.as_mut_ptr() as *mut c_void,
                len,
            )
        };
        self.unset_layout();

        if res == 0 {
            Ok(buf)
        } else {
            Err(ErrorCode {
                function: "flashrom_image_read",
                code: res,
            })
        }
    }

    /// Write the whole [`Chip`], or a portion specified in a Layout
    pub fn image_write(
        &mut self,
        buf: &mut [u8],
        layout: Option<Layout>,
    ) -> std::result::Result<(), ErrorCode> {
        if let Some(layout) = layout {
            self.set_layout(layout);
        }
        let res = unsafe {
            libflashrom_sys::flashrom_image_write(
                self.ctx.as_mut(),
                buf.as_mut_ptr() as *mut c_void,
                buf.len(),
                null(),
            )
        };
        self.unset_layout();

        if res == 0 {
            Ok(())
        } else {
            Err(ErrorCode {
                function: "flashrom_image_write",
                code: res,
            })
        }
    }

    /// Verify the whole [`Chip`], or a portion specified in a Layout
    pub fn image_verify(
        &mut self,
        buf: &[u8],
        layout: Option<Layout>,
    ) -> std::result::Result<(), ErrorCode> {
        if let Some(layout) = layout {
            self.set_layout(layout);
        }
        let res = unsafe {
            libflashrom_sys::flashrom_image_verify(
                self.ctx.as_mut(),
                buf.as_ptr() as *const c_void,
                buf.len(),
            )
        };
        self.unset_layout();

        if res == 0 {
            Ok(())
        } else {
            Err(ErrorCode {
                function: "flashrom_image_verify",
                code: res,
            })
        }
    }

    /// Erase the whole [`Chip`]
    pub fn erase(&mut self) -> std::result::Result<(), ErrorCode> {
        let res = unsafe { libflashrom_sys::flashrom_flash_erase(self.ctx.as_mut()) };
        if res == 0 {
            Ok(())
        } else {
            Err(ErrorCode {
                function: "flashrom_flash_erase",
                code: res,
            })
        }
    }

    /// Set a flag in the given flash context
    pub fn flag_set(&mut self, flag: FlashromFlag, value: bool) -> () {
        unsafe { libflashrom_sys::flashrom_flag_set(self.ctx.as_mut(), flag.into(), value) }
    }
}

impl Drop for Chip {
    fn drop(&mut self) {
        unsafe {
            libflashrom_sys::flashrom_flash_release(self.ctx.as_mut());
        }
    }
}

/// Structure for an initialised flashrom_wp_cfg
#[derive(Debug)]
pub struct WriteProtectCfg {
    wp: NonNull<libflashrom_sys::flashrom_wp_cfg>,
}

impl WriteProtectCfg {
    /// Create an empty [`WriteProtectCfg`]
    ///
    /// # Panics
    ///
    /// Panics if flashrom_wp_cfg_new returns FLASHROM_WP_OK and a NULL pointer.
    pub fn new() -> std::result::Result<WriteProtectCfg, WPError> {
        let mut cfg: *mut libflashrom_sys::flashrom_wp_cfg = null_mut();
        let res = unsafe { libflashrom_sys::flashrom_wp_cfg_new(&mut cfg) };
        if res != libflashrom_sys::flashrom_wp_result::FLASHROM_WP_OK {
            return Err(res.into());
        }
        Ok(WriteProtectCfg {
            wp: NonNull::new(cfg).expect("flashrom_wp_cfg_new returned null"),
        })
    }

    pub fn get_mode(&self) -> libflashrom_sys::flashrom_wp_mode {
        unsafe { libflashrom_sys::flashrom_wp_get_mode(self.wp.as_ref()) }
    }

    pub fn set_mode(&mut self, mode: libflashrom_sys::flashrom_wp_mode) {
        unsafe { libflashrom_sys::flashrom_wp_set_mode(self.wp.as_mut(), mode) }
    }

    pub fn get_range(&self) -> Range {
        let mut start = 0;
        let mut len = 0;
        unsafe { libflashrom_sys::flashrom_wp_get_range(&mut start, &mut len, self.wp.as_ref()) };
        start..(start + len)
    }

    pub fn set_range<T>(&mut self, range: T)
    where
        T: std::ops::RangeBounds<usize>,
    {
        let range: RangeInternal = range.into();
        unsafe { libflashrom_sys::flashrom_wp_set_range(self.wp.as_mut(), range.start, range.len) }
    }
}

impl Drop for WriteProtectCfg {
    fn drop(&mut self) {
        unsafe {
            libflashrom_sys::flashrom_wp_cfg_release(self.wp.as_mut());
        }
    }
}

impl fmt::Display for WriteProtectCfg {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "range:{:?} mode:{:?}", self.get_range(), self.get_mode())
    }
}

#[derive(Debug)]
struct WriteProtectRanges {
    ranges: NonNull<libflashrom_sys::flashrom_wp_ranges>,
}

impl WriteProtectRanges {}

impl Drop for WriteProtectRanges {
    fn drop(&mut self) {
        unsafe {
            libflashrom_sys::flashrom_wp_ranges_release(self.ranges.as_mut());
        }
    }
}

/// Structure for an initialised flashrom_layout
#[derive(Debug)]
pub struct Layout {
    layout: NonNull<libflashrom_sys::flashrom_layout>,
}

impl Layout {
    /// Create an empty [`Layout`]
    ///
    /// # Panics
    ///
    /// Panics if flashrom_layout_new returns 0 and a NULL pointer.
    pub fn new() -> std::result::Result<Layout, ErrorCode> {
        let mut layout: *mut libflashrom_sys::flashrom_layout = null_mut();
        let err = unsafe { libflashrom_sys::flashrom_layout_new(&mut layout) };
        if err != 0 {
            Err(ErrorCode {
                function: "flashrom_layout_new",
                code: err,
            })
        } else {
            Ok(Layout {
                layout: NonNull::new(layout).expect("flashrom_layout_new returned null"),
            })
        }
    }

    /// Add a region to the [`Layout`]
    ///
    /// Not the region will not be 'included', include_region must be called to include the region.
    ///
    /// # Errors
    ///
    /// This function will return an error if the region is not a valid CString,
    /// or if libflashrom returns an error.
    pub fn add_region<T>(&mut self, region: &str, range: T) -> std::result::Result<(), RegionError>
    where
        T: std::ops::RangeBounds<usize>,
    {
        let range: RangeInternal = range.into();
        let err = {
            let region = CString::new(region)?;
            unsafe {
                libflashrom_sys::flashrom_layout_add_region(
                    self.layout.as_mut(),
                    range.start,
                    range.end(),
                    region.as_ptr(),
                )
            }
        };

        if err != 0 {
            Err(RegionError::ErrorCode(ErrorCode {
                function: "flashrom_layout_add_region",
                code: err,
            }))
        } else {
            Ok(())
        }
    }

    /// Include a region
    ///
    /// # Errors
    ///
    /// This function will return an error if the region is not a valid CString,
    /// or if libflashrom returns an error.
    pub fn include_region(&mut self, region: &str) -> std::result::Result<(), RegionError> {
        let err = {
            let region = CString::new(region)?;
            unsafe {
                libflashrom_sys::flashrom_layout_include_region(
                    self.layout.as_mut(),
                    region.as_ptr(),
                )
            }
        };
        if err != 0 {
            Err(RegionError::ErrorCode(ErrorCode {
                function: "flashrom_layout_include_region",
                code: err,
            }))
        } else {
            Ok(())
        }
    }

    /// Exclude a region
    ///
    /// # Errors
    ///
    /// This function will return an error if the region is not a valid CString,
    /// or if libflashrom returns an error.
    pub fn exclude_region(&mut self, region: &str) -> std::result::Result<(), RegionError> {
        let err = {
            let region = CString::new(region)?;
            unsafe {
                libflashrom_sys::flashrom_layout_exclude_region(
                    self.layout.as_mut(),
                    region.as_ptr(),
                )
            }
        };
        if err != 0 {
            Err(RegionError::ErrorCode(ErrorCode {
                function: "flashrom_layout_exclude_region",
                code: err,
            }))
        } else {
            Ok(())
        }
    }

    /// Get the [`Range`] for the given region
    ///
    /// # Errors
    ///
    /// This function will return an error if the region is not a valid CString,
    /// or if libflashrom returns an error.
    pub fn get_region_range(&mut self, region: &str) -> std::result::Result<Range, RegionError> {
        let mut start: std::os::raw::c_uint = 0;
        let mut len: std::os::raw::c_uint = 0;
        let err = {
            let region = CString::new(region)?;
            unsafe {
                libflashrom_sys::flashrom_layout_get_region_range(
                    self.layout.as_mut(),
                    region.as_ptr(),
                    &mut start,
                    &mut len,
                )
            }
        };
        if err != 0 {
            Err(RegionError::ErrorCode(ErrorCode {
                function: "flashrom_layout_get_region_range",
                code: err,
            }))
        } else {
            // should be safe to assume sizeof(size_t) >= sizeof(unsigned int)
            // TODO: fix after https://review.coreboot.org/c/flashrom/+/65944
            Ok(start.try_into().unwrap()..(start + len).try_into().unwrap())
        }
    }
}

// TODO this will be replaced with an API implementation: https://review.coreboot.org/c/flashrom/+/65999
impl std::str::FromStr for Layout {
    type Err = Box<dyn error::Error>;

    /// This will attempt to parse the [`Layout`] file format into a [`Layout`]
    ///
    /// The format is documented in the flashrom man page.
    fn from_str(s: &str) -> std::result::Result<Self, Self::Err> {
        let mut ret = Layout::new()?;

        // format is hex:hex name
        // flashrom layout.c seems to allow any characters in the name string
        // I am restricting here to non whitespace
        let re = Regex::new(r"^([0-9A-Za-z]+):([0-9A-Za-z]+)\s+(\S+)$").unwrap();

        // we dont use captures_iter else we would ignore incorrect lines
        for line in s.lines() {
            let (start, end, name) = match re.captures(line) {
                Some(caps) => (
                    caps.get(1).unwrap(),
                    caps.get(2).unwrap(),
                    caps.get(3).unwrap(),
                ),
                None => Err(ParseLayoutError(format!("failed to parse: {:?}", line)))?,
            };
            let start = usize::from_str_radix(start.as_str(), 16)?;
            let end = usize::from_str_radix(end.as_str(), 16)?;
            ret.add_region(name.as_str(), start..=end)?;
        }

        Ok(ret)
    }
}

impl Drop for Layout {
    fn drop(&mut self) {
        unsafe {
            libflashrom_sys::flashrom_layout_release(self.layout.as_mut());
        }
    }
}

#[cfg(test)]
mod tests {
    use gag::BufferRedirect;
    use std::io::Read;

    use super::flashrom_version_info;
    use super::set_log_level;
    use super::Chip;
    use super::ChipInitError;
    use super::InitError;
    use crate::set_log_function;
    use crate::Layout;
    use crate::Programmer;
    use crate::WriteProtectCfg;

    // flashrom contains global state, which prevents correct initialisation of
    // a second programmer or probing of a second chip. Run all unit tests in
    // forked subprocesses to avoid this issue.
    use rusty_fork::rusty_fork_test;
    rusty_fork_test! {

        #[test]
        fn version() {
            // There is no version requirement yet, but for example:
            // assert!(flashrom_version_info().contains("v1.2"))
            assert!(!flashrom_version_info().unwrap().is_empty())
        }

        #[test]
        fn only_one_programmer() {
            {
                let _1 = Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap();
                // Only one programmer can be initialised at a time.
                assert_eq!(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap_err(), InitError::DuplicateInit)
            }
            // Only one programmer can ever be initialised
            assert_eq!(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap_err(), InitError::DuplicateInit)
        }

        #[test]
        fn programmer_bad_cstring_name() {
            assert!(matches!(Programmer::new("dummy\0", None).unwrap_err(), InitError::InvalidName(_)))
        }

        #[test]
        fn chip_none() {
            // Not specifying a chip will select one if there is one.
            Chip::new(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap(), None).unwrap();
        }

        #[test]
        fn chip_some() {
            // Specifying a valid chip.
            Chip::new(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap(), Some("W25Q128.V")).unwrap();
        }

        #[test]
        fn chip_nochip() {
            // Choosing a non existent chip fails.
            assert_eq!(
                Chip::new(Programmer::new("dummy", None).unwrap(), Some("W25Q128.V")).unwrap_err(),
                ChipInitError::NoChipError
            );
        }

        #[test]
        fn logging_stderr() {
            let mut buf = BufferRedirect::stderr().unwrap();
            let mut fc = Chip::new(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap(), Some("W25Q128.V")).unwrap();

            set_log_level(Some(libflashrom_sys::FLASHROM_MSG_INFO));
            fc.image_read(None).unwrap();
            let mut stderr = String::new();
            if buf.read_to_string(&mut stderr).unwrap() == 0 {
                panic!("stderr empty when it should have some messages");
            }

            set_log_level(None);
            fc.image_read(None).unwrap();
            if buf.read_to_string(&mut stderr).unwrap() != 0 {
                panic!("stderr not empty when it should be silent");
            }
        }

        #[test]
        fn logging_custom() {
            // Check that a custom logging callback works
            static mut BUF: String = String::new();
            fn logger(
                _: libflashrom_sys::flashrom_log_level,
                format: &str,
            ) {
                unsafe {BUF.push_str(format)}
            }
            set_log_function(logger);
            set_log_level(Some(libflashrom_sys::FLASHROM_MSG_SPEW));
            Chip::new(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap(), Some("W25Q128.V")).unwrap();
            assert_ne!(unsafe{BUF.len()}, 0);
        }

        #[test]
        fn flashchip() {
            // basic tests of the flashchip methods
            let mut fc = Chip::new(Programmer::new("dummy", Some("emulate=W25Q128FV")).unwrap(), Some("W25Q128.V")).unwrap();
            fc.get_size();

            let mut wp = fc.get_wp().unwrap();
            wp.set_mode(libflashrom_sys::flashrom_wp_mode::FLASHROM_WP_MODE_DISABLED);
            fc.set_wp(&wp).unwrap();
            fc.get_wp_ranges().unwrap();

            fn test_layout() -> Layout {
                let mut layout = Layout::new().unwrap();
                layout.add_region("xyz", 100..200).unwrap();
                layout.include_region("xyz").unwrap();
                layout.add_region("abc", 100..200).unwrap();
                layout
            }

            fc.image_read(None).unwrap();
            fc.image_read(Some(test_layout())).unwrap();

            let mut buf = vec![0; fc.get_size()];
            fc.image_write(&mut buf, None).unwrap();
            fc.image_write(&mut buf, Some(test_layout())).unwrap();

            fc.image_verify(&buf, None).unwrap();
            fc.image_verify(&buf, Some(test_layout())).unwrap();

            fc.erase().unwrap();
        }

        #[test]
        fn write_protect() {
            let mut wp = WriteProtectCfg::new().unwrap();
            wp.set_mode(libflashrom_sys::flashrom_wp_mode::FLASHROM_WP_MODE_DISABLED);
            wp.set_range(100..200);
            assert_eq!(wp.get_mode(), libflashrom_sys::flashrom_wp_mode::FLASHROM_WP_MODE_DISABLED);
            assert_eq!(wp.get_range(), 100..200);
        }
    }
}
