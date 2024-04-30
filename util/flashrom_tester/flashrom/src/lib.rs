//
// Copyright 2019, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Alternatively, this software may be distributed under the terms of the
// GNU General Public License ("GPL") version 2 as published by the Free
// Software Foundation.
//

#[macro_use]
extern crate log;

mod cmd;
mod flashromlib;

use std::{error, fmt, path::Path};

pub use cmd::FlashromCmd;
pub use flashromlib::FlashromLib;

pub use libflashrom::{
    flashrom_log_level, FlashromFlags, FLASHROM_MSG_DEBUG, FLASHROM_MSG_DEBUG2, FLASHROM_MSG_ERROR,
    FLASHROM_MSG_INFO, FLASHROM_MSG_SPEW, FLASHROM_MSG_WARN,
};

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum FlashChip {
    INTERNAL,
}

impl FlashChip {
    pub fn from(s: &str) -> Result<FlashChip, &str> {
        match s {
            "internal" => Ok(FlashChip::INTERNAL),
            _ => Err("cannot convert str to enum"),
        }
    }
    pub fn to(fc: FlashChip) -> &'static str {
        match fc {
            FlashChip::INTERNAL => "internal",
        }
    }

    /// Return the programmer string and optional programmer options
    pub fn to_split(fc: FlashChip) -> (&'static str, Option<&'static str>) {
        let programmer = FlashChip::to(fc);
        let mut bits = programmer.splitn(2, ':');
        (bits.next().unwrap(), bits.next())
    }

    /// Return whether the hardware write protect signal can be controlled.
    ///
    /// Servo and dediprog adapters are assumed to always have hardware write protect
    /// disabled.
    pub fn can_control_hw_wp(&self) -> bool {
        match self {
            FlashChip::INTERNAL => true,
        }
    }
}

#[derive(Debug, PartialEq, Eq)]
pub struct FlashromError {
    msg: String,
}

impl fmt::Display for FlashromError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.msg)
    }
}

impl error::Error for FlashromError {}

impl<T> From<T> for FlashromError
where
    T: Into<String>,
{
    fn from(msg: T) -> Self {
        FlashromError { msg: msg.into() }
    }
}

pub trait Flashrom {
    /// Returns the size of the flash in bytes.
    fn get_size(&self) -> Result<i64, FlashromError>;

    /// Returns the vendor name and the flash name.
    fn name(&self) -> Result<(String, String), FlashromError>;

    /// Set write protect status and range.
    fn wp_range(&self, range: (i64, i64), wp_enable: bool) -> Result<bool, FlashromError>;

    /// Read the write protect regions for the flash.
    fn wp_list(&self) -> Result<String, FlashromError>;

    /// Return true if the flash write protect status matches `en`.
    fn wp_status(&self, en: bool) -> Result<bool, FlashromError>;

    /// Set write protect status.
    /// If en=true sets wp_range to the whole chip (0,getsize()).
    /// If en=false sets wp_range to (0,0).
    /// This is due to the MTD driver, which requires wp enable to use a range
    /// length != 0 and wp disable to have the range 0,0.
    fn wp_toggle(&self, en: bool) -> Result<bool, FlashromError>;

    /// Read the whole flash to the file specified by `path`.
    fn read_into_file(&self, path: &Path) -> Result<(), FlashromError>;

    /// Read only a region of the flash into the file specified by `path`. Note
    /// the first byte written to the file is the first byte from the region.
    fn read_region_into_file(&self, path: &Path, region: &str) -> Result<(), FlashromError>;

    /// Write the whole flash to the file specified by `path`.
    fn write_from_file(&self, path: &Path) -> Result<(), FlashromError>;

    /// Write only a region of the flash.
    /// `path` is a file of the size of the whole flash.
    /// The `region` name corresponds to a region name in the `layout` file, not the flash.
    fn write_from_file_region(
        &self,
        path: &Path,
        region: &str,
        layout: &Path,
    ) -> Result<bool, FlashromError>;

    /// Verify the whole flash against the file specified by `path`.
    fn verify_from_file(&self, path: &Path) -> Result<(), FlashromError>;

    /// Verify only the region against the file specified by `path`.
    /// Note the first byte in the file is matched against the first byte of the region.
    fn verify_region_from_file(&self, path: &Path, region: &str) -> Result<(), FlashromError>;

    /// Erase the whole flash.
    fn erase(&self) -> Result<(), FlashromError>;

    /// Return true if the hardware write protect of this flash can be controlled.
    fn can_control_hw_wp(&self) -> bool;

    /// Set flags used by the flashrom cli.
    fn set_flags(&self, flags: &FlashromFlags) -> ();
}
