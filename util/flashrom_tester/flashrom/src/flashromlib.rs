// Copyright 2022, Google Inc.
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

use libflashrom::{Chip, FlashromFlag, FlashromFlags, Programmer};

use std::{cell::RefCell, convert::TryFrom, fs, path::Path};

use crate::{FlashChip, FlashromError};

#[derive(Debug)]
pub struct FlashromLib {
    // RefCell required here to keep Flashrom trait immutable.
    // Cant make Flashrom methods mut because WriteProtectState
    // and TestEnv both keep a reference.
    pub flashrom: RefCell<Chip>,
    pub fc: FlashChip,
}

impl FlashromLib {
    pub fn new(fc: FlashChip, log_level: libflashrom::flashrom_log_level) -> FlashromLib {
        libflashrom::set_log_level(Some(log_level));
        let (programmer, options) = FlashChip::to_split(fc);
        let flashrom = Chip::new(Programmer::new(programmer, options).unwrap(), None).unwrap();
        FlashromLib {
            flashrom: RefCell::new(flashrom),
            fc,
        }
    }
}

impl crate::Flashrom for FlashromLib {
    fn get_size(&self) -> Result<i64, FlashromError> {
        Ok(self.flashrom.borrow().get_size() as i64)
    }

    fn name(&self) -> Result<(String, String), FlashromError> {
        Ok(("not".to_string(), "implemented".to_string()))
    }

    fn wp_range(&self, range: (i64, i64), wp_enable: bool) -> Result<bool, FlashromError> {
        let mut cfg = libflashrom::WriteProtectCfg::new()?;
        let start = usize::try_from(range.0).unwrap();
        let len = usize::try_from(range.1).unwrap();
        cfg.set_range::<std::ops::Range<usize>>(start..(start + len));
        cfg.set_mode(if wp_enable {
            libflashrom::flashrom_wp_mode::FLASHROM_WP_MODE_HARDWARE
        } else {
            libflashrom::flashrom_wp_mode::FLASHROM_WP_MODE_DISABLED
        });
        self.flashrom.borrow_mut().set_wp(&cfg)?;
        Ok(true)
    }

    fn wp_list(&self) -> Result<String, FlashromError> {
        let ranges = self.flashrom.borrow_mut().get_wp_ranges()?;
        Ok(format!("{:?}", ranges))
    }

    fn wp_status(&self, en: bool) -> Result<bool, FlashromError> {
        let ret = self
            .flashrom
            .borrow_mut()
            .get_wp()
            .map_err(|e| format!("{:?}", e))?
            .get_mode();
        if en {
            Ok(ret != libflashrom::flashrom_wp_mode::FLASHROM_WP_MODE_DISABLED)
        } else {
            Ok(ret == libflashrom::flashrom_wp_mode::FLASHROM_WP_MODE_DISABLED)
        }
    }

    fn wp_toggle(&self, en: bool) -> Result<bool, FlashromError> {
        let range = if en { (0, self.get_size()?) } else { (0, 0) };
        self.wp_range(range, en)
    }

    fn read_into_file(&self, path: &Path) -> Result<(), FlashromError> {
        let buf = self.flashrom.borrow_mut().image_read(None)?;
        fs::write(path, buf).map_err(|error| error.to_string())?;
        Ok(())
    }

    fn read_region_into_file(&self, path: &Path, region: &str) -> Result<(), FlashromError> {
        let mut layout = self.flashrom.borrow_mut().layout_read_fmap_from_rom()?;
        layout.include_region(region)?;
        let range = layout.get_region_range(region)?;
        let buf = self.flashrom.borrow_mut().image_read(None)?;
        fs::write(path, &buf[range]).map_err(|error| error.to_string())?;
        Ok(())
    }

    fn write_from_file(&self, path: &Path) -> Result<(), FlashromError> {
        let mut buf = fs::read(path).map_err(|error| error.to_string())?;
        self.flashrom.borrow_mut().image_write(&mut buf, None)?;
        Ok(())
    }

    fn write_from_file_region(
        &self,
        path: &Path,
        region: &str,
        layout: &Path,
    ) -> Result<bool, FlashromError> {
        let buf = fs::read(layout).map_err(|error| error.to_string())?;
        let buf = String::from_utf8(buf).unwrap();
        let mut layout: libflashrom::Layout = buf
            .parse()
            .map_err(|e: Box<dyn std::error::Error>| e.to_string())?;
        layout.include_region(region)?;
        let mut buf = fs::read(path).map_err(|error| error.to_string())?;
        self.flashrom
            .borrow_mut()
            .image_write(&mut buf, Some(layout))?;
        Ok(true)
    }

    fn verify_from_file(&self, path: &Path) -> Result<(), FlashromError> {
        let buf = fs::read(path).map_err(|error| error.to_string())?;
        self.flashrom.borrow_mut().image_verify(&buf, None)?;
        Ok(())
    }

    fn verify_region_from_file(&self, path: &Path, region: &str) -> Result<(), FlashromError> {
        let mut layout = self.flashrom.borrow_mut().layout_read_fmap_from_rom()?;
        layout.include_region(region)?;
        let range = layout.get_region_range(region)?;
        let region_data = fs::read(path).map_err(|error| error.to_string())?;
        if region_data.len() != range.len() {
            return Err(format!(
                "verify region range ({}) does not match provided file size ({})",
                range.len(),
                region_data.len()
            )
            .into());
        }
        let mut buf = vec![0; self.get_size()? as usize];
        buf[range].copy_from_slice(&region_data);
        self.flashrom
            .borrow_mut()
            .image_verify(&buf, Some(layout))?;
        Ok(())
    }

    fn erase(&self) -> Result<(), FlashromError> {
        self.flashrom.borrow_mut().erase()?;
        Ok(())
    }

    fn can_control_hw_wp(&self) -> bool {
        self.fc.can_control_hw_wp()
    }

    fn set_flags(&self, flags: &FlashromFlags) -> () {
        self.flashrom
            .borrow_mut()
            .flag_set(FlashromFlag::FlashromFlagForce, flags.force);
        self.flashrom
            .borrow_mut()
            .flag_set(FlashromFlag::FlashromFlagForceBoardmismatch, flags.force_boardmismatch);
        self.flashrom
            .borrow_mut()
            .flag_set(FlashromFlag::FlashromFlagVerifyAfterWrite, flags.verify_after_write);
        self.flashrom
            .borrow_mut()
            .flag_set(FlashromFlag::FlashromFlagVerifyWholeChip, flags.verify_whole_chip);
        self.flashrom
            .borrow_mut()
            .flag_set(FlashromFlag::FlashromFlagSkipUnreadableRegions, flags.skip_unreadable_regions);
        self.flashrom
            .borrow_mut()
            .flag_set(FlashromFlag::FlashromFlagSkipUnwritableRegions, flags.skip_unwritable_regions);
    }
}
