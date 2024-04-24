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

use std::io::prelude::*;
use std::process::Command;

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum LayoutNames {
    TopQuad,
    TopHalf,
    BottomHalf,
    BottomQuad,
}

impl LayoutNames {
    // Return a section that does not overlap
    pub fn get_non_overlapping_section(&self) -> LayoutNames {
        match self {
            LayoutNames::TopQuad => LayoutNames::BottomQuad,
            LayoutNames::TopHalf => LayoutNames::BottomHalf,
            LayoutNames::BottomHalf => LayoutNames::TopHalf,
            LayoutNames::BottomQuad => LayoutNames::TopQuad,
        }
    }
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct LayoutSizes {
    half_sz: i64,
    quad_sz: i64,
    rom_top: i64,
    bottom_half_top: i64,
    bottom_quad_top: i64,
    top_quad_bottom: i64,
}

pub fn get_layout_sizes(rom_sz: i64) -> Result<LayoutSizes, String> {
    if rom_sz <= 0 {
        return Err("invalid rom size provided".into());
    }
    if rom_sz & (rom_sz - 1) != 0 {
        return Err("invalid rom size, not a power of 2".into());
    }
    Ok(LayoutSizes {
        half_sz: rom_sz / 2,
        quad_sz: rom_sz / 4,
        rom_top: rom_sz - 1,
        bottom_half_top: (rom_sz / 2) - 1,
        bottom_quad_top: (rom_sz / 4) - 1,
        top_quad_bottom: (rom_sz / 4) * 3,
    })
}

pub fn layout_section(ls: &LayoutSizes, ln: LayoutNames) -> (&'static str, i64, i64) {
    match ln {
        LayoutNames::TopQuad => ("TOP_QUAD", ls.top_quad_bottom, ls.quad_sz),
        LayoutNames::TopHalf => ("TOP_HALF", ls.half_sz, ls.half_sz),
        LayoutNames::BottomHalf => ("BOTTOM_HALF", 0, ls.half_sz),
        LayoutNames::BottomQuad => ("BOTTOM_QUAD", 0, ls.quad_sz),
    }
}

pub fn construct_layout_file<F: Write>(mut target: F, ls: &LayoutSizes) -> std::io::Result<()> {
    writeln!(target, "000000:{:x} BOTTOM_QUAD", ls.bottom_quad_top)?;
    writeln!(target, "000000:{:x} BOTTOM_HALF", ls.bottom_half_top)?;
    writeln!(target, "{:x}:{:x} TOP_HALF", ls.half_sz, ls.rom_top)?;
    writeln!(target, "{:x}:{:x} TOP_QUAD", ls.top_quad_bottom, ls.rom_top)
}

pub fn toggle_hw_wp(dis: bool) -> Result<(), String> {
    // The easist way to toggle the hardware write-protect is
    // to {dis}connect the battery (and/or {open,close} the WP screw).
    let s = if dis { "dis" } else { "" };
    let screw_state = if dis { "open" } else { "close" };
    // Print a failure message, but not on the first try.
    let mut fail_msg = None;
    while dis == get_hardware_wp()? {
        if let Some(msg) = fail_msg {
            eprintln!("{msg}");
        }
        fail_msg = Some(format!("Hardware write protect is still {}!", !dis));
        // The following message is read by the tast test. Do not modify.
        info!("Prompt for hardware WP {}able", s);
        eprintln!(" > {}connect the battery (and/or {} the WP screw)", s, screw_state);
        pause();
    }
    Ok(())
}

pub fn ac_power_warning() {
    info!("*****************************");
    info!("AC power *must be* connected!");
    info!("*****************************");
    pause();
}

fn pause() {
    // The following message is read by the tast test. Do not modify.
    println!("Press enter to continue...");
    // Rust stdout is always LineBuffered at time of writing.
    // But this is not guaranteed, so flush anyway.
    std::io::stdout().flush().unwrap();
    // This reads one line, there is no guarantee the line came
    // after the above prompt. But it is good enough.
    if std::io::stdin().read_line(&mut String::new()).unwrap() == 0 {
        panic!("stdin closed");
    }
}

pub fn get_hardware_wp() -> std::result::Result<bool, String> {
    let wp_s_val = collect_crosssystem(&["wpsw_cur"])?.parse::<u32>();
    match wp_s_val {
        Ok(v) => {
            if v == 1 {
                Ok(true)
            } else if v == 0 {
                Ok(false)
            } else {
                Err("Unknown write protect value".into())
            }
        }
        Err(_) => Err("Cannot parse write protect value".into()),
    }
}

pub fn collect_crosssystem(args: &[&str]) -> Result<String, String> {
    let cmd = match Command::new("crossystem").args(args).output() {
        Ok(x) => x,
        Err(e) => return Err(format!("Failed to run crossystem: {}", e)),
    };

    if !cmd.status.success() {
        return Err(translate_command_error(&cmd).to_string());
    };

    Ok(String::from_utf8_lossy(&cmd.stdout).into_owned())
}

pub fn translate_command_error(output: &std::process::Output) -> std::io::Error {
    use std::io::{Error, ErrorKind};
    // There is two cases on failure;
    //  i. ) A bad exit code,
    //  ii.) A SIG killed us.
    match output.status.code() {
        Some(code) => {
            let e = format!(
                "{}\nExited with error code: {}",
                String::from_utf8_lossy(&output.stderr),
                code
            );
            Error::new(ErrorKind::Other, e)
        }
        None => Error::new(
            ErrorKind::Other,
            "Process terminated by a signal".to_string(),
        ),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn construct_layout_file() {
        use super::{construct_layout_file, get_layout_sizes};

        let mut buf = Vec::new();
        construct_layout_file(
            &mut buf,
            &get_layout_sizes(0x10000).expect("64k is a valid chip size"),
        )
        .expect("no I/O errors expected");

        assert_eq!(
            &buf[..],
            &b"000000:3fff BOTTOM_QUAD\n\
               000000:7fff BOTTOM_HALF\n\
               8000:ffff TOP_HALF\n\
               c000:ffff TOP_QUAD\n"[..]
        );
    }

    #[test]
    fn get_layout_sizes() {
        use super::get_layout_sizes;

        assert_eq!(
            get_layout_sizes(-128).err(),
            Some("invalid rom size provided".into())
        );

        assert_eq!(
            get_layout_sizes(3 << 20).err(),
            Some("invalid rom size, not a power of 2".into())
        );

        assert_eq!(
            get_layout_sizes(64 << 10).unwrap(),
            LayoutSizes {
                half_sz: 0x8000,
                quad_sz: 0x4000,
                rom_top: 0xFFFF,
                bottom_half_top: 0x7FFF,
                bottom_quad_top: 0x3FFF,
                top_quad_bottom: 0xC000,
            }
        );
    }
}
