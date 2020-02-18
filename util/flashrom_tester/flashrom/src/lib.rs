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

pub use cmd::{dut_ctrl_toggle_wp, FlashromCmd};

#[derive(Copy, Clone, PartialEq, Debug)]
pub enum FlashChip {
    EC,
    HOST,
    SERVO,
    DEDIPROG,
}

impl FlashChip {
    pub fn from(s: &str) -> Result<FlashChip, &str> {
        let r = match s {
            "ec" => Ok(FlashChip::EC),
            "host" => Ok(FlashChip::HOST),
            "servo" => Ok(FlashChip::SERVO),
            "dediprog" => Ok(FlashChip::DEDIPROG),
            _ => Err("cannot convert str to enum"),
        };
        return r;
    }
    pub fn to(fc: FlashChip) -> &'static str {
        let r = match fc {
            FlashChip::EC => "ec",
            FlashChip::HOST => "host",
            FlashChip::SERVO => "ft2231_spi:type=servo-v2",
            FlashChip::DEDIPROG => "dediprog",
        };
        return r;
    }

    /// Return whether the hardware write protect signal can be controlled.
    ///
    /// Servo and dediprog adapters are assumed to always have hardware write protect
    /// disabled.
    pub fn can_control_hw_wp(&self) -> bool {
        match self {
            FlashChip::HOST | FlashChip::EC => true,
            FlashChip::SERVO | FlashChip::DEDIPROG => false,
        }
    }
}

pub type FlashromError = String;

#[derive(Default)]
pub struct FlashromOpt<'a> {
    pub wp_opt: WPOpt,
    pub io_opt: IOOpt<'a>,

    pub layout: Option<&'a str>, // -l <file>
    pub image: Option<&'a str>,  // -i <name>

    pub flash_name: bool,  // --flash-name
    pub ignore_fmap: bool, // --ignore-fmap
    pub verbose: bool,     // -V
}

#[derive(Default)]
pub struct WPOpt {
    pub range: Option<(i64, i64)>, // --wp-range x0 x1
    pub status: bool,              // --wp-status
    pub list: bool,                // --wp-list
    pub enable: bool,              // --wp-enable
    pub disable: bool,             // --wp-disable
}

#[derive(Default)]
pub struct IOOpt<'a> {
    pub read: Option<&'a str>,   // -r <file>
    pub write: Option<&'a str>,  // -w <file>
    pub verify: Option<&'a str>, // -v <file>
    pub erase: bool,             // -E
}

pub trait Flashrom {
    fn get_size(&self) -> Result<i64, FlashromError>;
    fn dispatch(&self, fropt: FlashromOpt) -> Result<(Vec<u8>, Vec<u8>), FlashromError>;
}

pub fn name(cmd: &cmd::FlashromCmd) -> Result<(String, String), FlashromError> {
    let opts = FlashromOpt {
        io_opt: IOOpt {
            ..Default::default()
        },

        flash_name: true,

        ..Default::default()
    };

    let (stdout, stderr) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    let eoutput = String::from_utf8_lossy(stderr.as_slice());
    debug!("name()'stdout: {:#?}.", output);
    debug!("name()'stderr: {:#?}.", eoutput);

    match extract_flash_name(&output) {
        None => Err("Didn't find chip vendor/name in flashrom output".into()),
        Some((vendor, name)) => Ok((vendor.into(), name.into())),
    }
}

/// Get a flash vendor and name from the first matching line of flashrom output.
///
/// The target line looks like 'vendor="foo" name="bar"', as output by flashrom --flash-name.
/// This is usually the last line of output.
fn extract_flash_name(stdout: &str) -> Option<(&str, &str)> {
    for line in stdout.lines() {
        if !line.starts_with("vendor=\"") {
            continue;
        }

        let tail = line.trim_start_matches("vendor=\"");
        let mut split = tail.splitn(2, "\" name=\"");
        let vendor = split.next();
        let name = split.next().map(|s| s.trim_end_matches('"'));

        match (vendor, name) {
            (Some(v), Some(n)) => return Some((v, n)),
            _ => continue,
        }
    }
    None
}

pub struct ROMWriteSpecifics<'a> {
    pub layout_file: Option<&'a str>,
    pub write_file: Option<&'a str>,
    pub name_file: Option<&'a str>,
}

pub fn write_file_with_layout(
    cmd: &cmd::FlashromCmd,
    rws: &ROMWriteSpecifics,
) -> Result<bool, FlashromError> {
    let opts = FlashromOpt {
        io_opt: IOOpt {
            write: rws.write_file,
            ..Default::default()
        },

        layout: rws.layout_file,
        image: rws.name_file,

        ignore_fmap: true,

        ..Default::default()
    };

    let (stdout, stderr) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    let eoutput = String::from_utf8_lossy(stderr.as_slice());
    debug!("write_file_with_layout()'stdout:\n{}.", output);
    debug!("write_file_with_layout()'stderr:\n{}.", eoutput);
    Ok(true)
}

pub fn wp_range(
    cmd: &cmd::FlashromCmd,
    range: (i64, i64),
    wp_enable: bool,
) -> Result<bool, FlashromError> {
    let opts = FlashromOpt {
        wp_opt: WPOpt {
            range: Some(range),
            enable: wp_enable,
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, stderr) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    let eoutput = String::from_utf8_lossy(stderr.as_slice());
    debug!("wp_range()'stdout:\n{}.", output);
    debug!("wp_range()'stderr:\n{}.", eoutput);
    Ok(true)
}

pub fn wp_list(cmd: &cmd::FlashromCmd) -> Result<String, FlashromError> {
    let opts = FlashromOpt {
        wp_opt: WPOpt {
            list: true,
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, _) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    if output.len() == 0 {
        return Err(
            "wp_list isn't supported on platforms using the Linux kernel SPI driver wp_list".into(),
        );
    }
    Ok(output.to_string())
}

pub fn wp_status(cmd: &cmd::FlashromCmd, en: bool) -> Result<bool, FlashromError> {
    let status = if en { "en" } else { "dis" };
    info!("See if chip write protect is {}abled", status);

    let opts = FlashromOpt {
        wp_opt: WPOpt {
            status: true,
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, _) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());

    debug!("wp_status():\n{}", output);

    let s = std::format!("write protect is {}abled", status);
    Ok(output.contains(&s))
}

pub fn wp_toggle(cmd: &cmd::FlashromCmd, en: bool) -> Result<bool, FlashromError> {
    let status = if en { "en" } else { "dis" };

    // For MTD, --wp-range and --wp-enable must be used simultaneously.
    let range = if en {
        let rom_sz: i64 = cmd.get_size()?;
        Some((0, rom_sz)) // (start, len)
    } else {
        None
    };

    let opts = FlashromOpt {
        wp_opt: WPOpt {
            range: range,
            enable: en,
            disable: !en,
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, stderr) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    let eoutput = String::from_utf8_lossy(stderr.as_slice());

    debug!("wp_toggle()'stdout:\n{}.", output);
    debug!("wp_toggle()'stderr:\n{}.", eoutput);

    match wp_status(&cmd, true) {
        Ok(_ret) => {
            info!("Successfully {}abled write-protect", status);
            Ok(true)
        }
        Err(e) => Err(format!("Cannot {}able write-protect: {}", status, e)),
    }
}

pub fn read(cmd: &cmd::FlashromCmd, path: &str) -> Result<(), FlashromError> {
    let opts = FlashromOpt {
        io_opt: IOOpt {
            read: Some(path),
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, _) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    debug!("read():\n{}", output);
    Ok(())
}

pub fn write(cmd: &cmd::FlashromCmd, path: &str) -> Result<(), FlashromError> {
    let opts = FlashromOpt {
        io_opt: IOOpt {
            write: Some(path),
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, _) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    debug!("write():\n{}", output);
    Ok(())
}

pub fn verify(cmd: &cmd::FlashromCmd, path: &str) -> Result<(), FlashromError> {
    let opts = FlashromOpt {
        io_opt: IOOpt {
            verify: Some(path),
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, _) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    debug!("verify():\n{}", output);
    Ok(())
}

pub fn erase(cmd: &cmd::FlashromCmd) -> Result<(), FlashromError> {
    let opts = FlashromOpt {
        io_opt: IOOpt {
            erase: true,
            ..Default::default()
        },
        ..Default::default()
    };

    let (stdout, _) = cmd.dispatch(opts)?;
    let output = String::from_utf8_lossy(stdout.as_slice());
    debug!("erase():\n{}", output);
    Ok(())
}

#[cfg(test)]
mod tests {
    #[test]
    fn extract_flash_name() {
        use super::extract_flash_name;

        assert_eq!(
            extract_flash_name(
                "coreboot table found at 0x7cc13000\n\
                 Found chipset \"Intel Braswell\". Enabling flash write... OK.\n\
                 vendor=\"Winbond\" name=\"W25Q64DW\"\n"
            ),
            Some(("Winbond", "W25Q64DW"))
        );

        assert_eq!(
            extract_flash_name(
                "vendor name is TEST\n\
                 Something failed!"
            ),
            None
        )
    }
}
