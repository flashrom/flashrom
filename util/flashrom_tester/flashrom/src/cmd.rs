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

use crate::{FlashChip, FlashromError, ROMWriteSpecifics};

use std::{
    ffi::{OsStr, OsString},
    path::Path,
    process::Command,
};

#[derive(Default)]
pub struct FlashromOpt<'a> {
    pub wp_opt: WPOpt,
    pub io_opt: IOOpt<'a>,

    pub layout: Option<&'a Path>, // -l <file>
    pub image: Option<&'a str>,   // -i <name>

    pub flash_name: bool, // --flash-name
    pub verbose: bool,    // -V
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
    pub read: Option<&'a Path>,              // -r <file>
    pub write: Option<&'a Path>,             // -w <file>
    pub verify: Option<&'a Path>,            // -v <file>
    pub erase: bool,                         // -E
    pub region: Option<(&'a str, &'a Path)>, // --image
}

#[derive(PartialEq, Eq, Debug)]
pub struct FlashromCmd {
    pub path: String,
    pub fc: FlashChip,
}

/// Attempt to determine the Flash size given stdout from `flashrom --flash-size`
fn flashrom_extract_size(stdout: &str) -> Result<i64, FlashromError> {
    // Search for the last line of output that contains only digits, assuming
    // that's the actual size. flashrom sadly tends to write additional messages
    // to stdout.
    match stdout
        .lines()
        .filter(|line| line.chars().all(|c| c.is_ascii_digit()))
        .last()
        .map(str::parse::<i64>)
    {
        None => Err("Found no purely-numeric lines in flashrom output".into()),
        Some(Err(e)) => {
            Err(format!("Failed to parse flashrom size output as integer: {}", e).into())
        }
        Some(Ok(sz)) => Ok(sz),
    }
}

impl FlashromCmd {
    fn dispatch(
        &self,
        fropt: FlashromOpt,
        debug_name: &str,
    ) -> Result<(String, String), FlashromError> {
        let params = flashrom_decode_opts(fropt);
        flashrom_dispatch(self.path.as_str(), &params, self.fc, debug_name)
    }
}

impl crate::Flashrom for FlashromCmd {
    fn get_size(&self) -> Result<i64, FlashromError> {
        let (stdout, _) =
            flashrom_dispatch(self.path.as_str(), &["--flash-size"], self.fc, "get_size")?;
        flashrom_extract_size(&stdout)
    }

    fn name(&self) -> Result<(String, String), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                ..Default::default()
            },

            flash_name: true,

            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts, "name")?;
        match extract_flash_name(&stdout) {
            None => Err("Didn't find chip vendor/name in flashrom output".into()),
            Some((vendor, name)) => Ok((vendor.into(), name.into())),
        }
    }

    fn write_file_with_layout(&self, rws: &ROMWriteSpecifics) -> Result<bool, FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                write: rws.write_file,
                ..Default::default()
            },

            layout: rws.layout_file,
            image: rws.name_file,

            ..Default::default()
        };

        self.dispatch(opts, "write_file_with_layout")?;
        Ok(true)
    }

    fn wp_range(&self, range: (i64, i64), wp_enable: bool) -> Result<bool, FlashromError> {
        let opts = FlashromOpt {
            wp_opt: WPOpt {
                range: Some(range),
                enable: wp_enable,
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "wp_range")?;
        Ok(true)
    }

    fn wp_list(&self) -> Result<String, FlashromError> {
        let opts = FlashromOpt {
            wp_opt: WPOpt {
                list: true,
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts, "wp_list")?;
        if stdout.is_empty() {
            return Err(
                "wp_list isn't supported on platforms using the Linux kernel SPI driver wp_list"
                    .into(),
            );
        }
        Ok(stdout)
    }

    fn wp_status(&self, en: bool) -> Result<bool, FlashromError> {
        let status = if en { "en" } else { "dis" };
        info!("See if chip write protect is {}abled", status);

        let opts = FlashromOpt {
            wp_opt: WPOpt {
                status: true,
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts, "wp_status")?;
        let s = std::format!("write protect is {}abled", status);
        Ok(stdout.contains(&s))
    }

    fn wp_toggle(&self, en: bool) -> Result<bool, FlashromError> {
        let status = if en { "en" } else { "dis" };

        // For MTD, --wp-range and --wp-enable must be used simultaneously.
        let range = if en {
            let rom_sz: i64 = self.get_size()?;
            Some((0, rom_sz)) // (start, len)
        } else {
            None
        };

        let opts = FlashromOpt {
            wp_opt: WPOpt {
                range,
                enable: en,
                disable: !en,
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "wp_toggle")?;

        match self.wp_status(true) {
            Ok(_ret) => {
                info!("Successfully {}abled write-protect", status);
                Ok(true)
            }
            Err(e) => Err(format!("Cannot {}able write-protect: {}", status, e).into()),
        }
    }

    fn read_into_file(&self, path: &Path) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                read: Some(path),
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "read_into_file")?;
        Ok(())
    }

    fn read_region_into_file(&self, path: &Path, region: &str) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                region: Some((region, path)),
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "read_region_into_file")?;
        Ok(())
    }

    fn write_from_file(&self, path: &Path) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                write: Some(path),
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "write_from_file")?;
        Ok(())
    }

    fn verify_from_file(&self, path: &Path) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                verify: Some(path),
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "verify_from_file")?;
        Ok(())
    }

    fn erase(&self) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                erase: true,
                ..Default::default()
            },
            ..Default::default()
        };

        self.dispatch(opts, "erase")?;
        Ok(())
    }

    fn can_control_hw_wp(&self) -> bool {
        self.fc.can_control_hw_wp()
    }
}

fn flashrom_decode_opts(opts: FlashromOpt) -> Vec<OsString> {
    let mut params = Vec::<OsString>::new();

    // ------------ WARNING !!! ------------
    // each param must NOT contain spaces!
    // -------------------------------------

    // wp_opt
    if opts.wp_opt.range.is_some() {
        let (x0, x1) = opts.wp_opt.range.unwrap();
        params.push("--wp-range".into());
        params.push(hex_range_string(x0, x1).into());
    }
    if opts.wp_opt.status {
        params.push("--wp-status".into());
    } else if opts.wp_opt.list {
        params.push("--wp-list".into());
    } else if opts.wp_opt.enable {
        params.push("--wp-enable".into());
    } else if opts.wp_opt.disable {
        params.push("--wp-disable".into());
    }

    // io_opt
    if let Some((region, path)) = opts.io_opt.region {
        params.push("--image".into());
        let mut p = OsString::new();
        p.push(region);
        p.push(":");
        p.push(path);
        params.push(p);
        params.push("-r".into());
    } else if opts.io_opt.read.is_some() {
        params.push("-r".into());
        params.push(opts.io_opt.read.unwrap().into());
    } else if opts.io_opt.write.is_some() {
        params.push("-w".into());
        params.push(opts.io_opt.write.unwrap().into());
    } else if opts.io_opt.verify.is_some() {
        params.push("-v".into());
        params.push(opts.io_opt.verify.unwrap().into());
    } else if opts.io_opt.erase {
        params.push("-E".into());
    }

    // misc_opt
    if opts.layout.is_some() {
        params.push("-l".into());
        params.push(opts.layout.unwrap().into());
    }
    if opts.image.is_some() {
        params.push("-i".into());
        params.push(opts.image.unwrap().into());
    }

    if opts.flash_name {
        params.push("--flash-name".into());
    }
    if opts.verbose {
        params.push("-V".into());
    }

    params
}

fn flashrom_dispatch<S: AsRef<OsStr>>(
    path: &str,
    params: &[S],
    fc: FlashChip,
    debug_name: &str,
) -> Result<(String, String), FlashromError> {
    // from man page:
    //  ' -p, --programmer <name>[:parameter[,parameter[,parameter]]] '
    let mut args: Vec<&OsStr> = vec![OsStr::new("-p"), OsStr::new(FlashChip::to(fc))];
    args.extend(params.iter().map(S::as_ref));

    info!("flashrom_dispatch() running: {} {:?}", path, args);

    let output = match Command::new(path).args(&args).output() {
        Ok(x) => x,
        Err(e) => return Err(format!("Failed to run flashrom: {}", e).into()),
    };

    let stdout = String::from_utf8_lossy(output.stdout.as_slice());
    let stderr = String::from_utf8_lossy(output.stderr.as_slice());
    debug!("{}()'stdout: {}.", debug_name, stdout);
    debug!("{}()'stderr: {}.", debug_name, stderr);

    if !output.status.success() {
        // There is two cases on failure;
        //  i. ) A bad exit code,
        //  ii.) A SIG killed us.
        match output.status.code() {
            Some(code) => {
                return Err(format!("{}\nExited with error code: {}", stderr, code).into());
            }
            None => return Err("Process terminated by a signal".into()),
        }
    }

    Ok((stdout.into(), stderr.into()))
}

pub fn dut_ctrl_toggle_wp(en: bool) -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
    let args = if en {
        ["fw_wp_en:off", "fw_wp:on"]
    } else {
        ["fw_wp_en:on", "fw_wp:off"]
    };
    dut_ctrl(&args)
}

fn dut_ctrl(args: &[&str]) -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
    let output = match Command::new("dut-control").args(args).output() {
        Ok(x) => x,
        Err(e) => return Err(format!("Failed to run dut-control: {}", e).into()),
    };
    if !output.status.success() {
        // There is two cases on failure;
        //  i. ) A bad exit code,
        //  ii.) A SIG killed us.
        match output.status.code() {
            Some(code) => {
                return Err(format!("Exited with error code: {}", code).into());
            }
            None => return Err("Process terminated by a signal".into()),
        }
    }

    Ok((output.stdout, output.stderr))
}

fn hex_range_string(s: i64, l: i64) -> String {
    format!("{:#08X},{:#08X}", s, l)
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

#[cfg(test)]
mod tests {
    use std::path::Path;

    use super::flashrom_decode_opts;
    use super::{FlashromOpt, IOOpt, WPOpt};

    #[test]
    fn decode_wp_opt() {
        fn test_wp_opt(wpo: WPOpt, expected: &[&str]) {
            assert_eq!(
                flashrom_decode_opts(FlashromOpt {
                    wp_opt: wpo,
                    ..Default::default()
                }),
                expected
            );
        }

        test_wp_opt(Default::default(), &[]);
        test_wp_opt(
            WPOpt {
                range: Some((0, 1234)),
                status: true,
                ..Default::default()
            },
            &["--wp-range", "0x000000,0x0004D2", "--wp-status"],
        );
        test_wp_opt(
            WPOpt {
                list: true,
                ..Default::default()
            },
            &["--wp-list"],
        );
        test_wp_opt(
            WPOpt {
                enable: true,
                ..Default::default()
            },
            &["--wp-enable"],
        );
        test_wp_opt(
            WPOpt {
                disable: true,
                ..Default::default()
            },
            &["--wp-disable"],
        );
    }

    #[test]
    fn decode_io_opt() {
        fn test_io_opt(opts: IOOpt, expected: &[&str]) {
            assert_eq!(
                flashrom_decode_opts(FlashromOpt {
                    io_opt: opts,
                    ..Default::default()
                }),
                expected
            );
        }

        test_io_opt(
            IOOpt {
                read: Some(Path::new("foo.bin")),
                ..Default::default()
            },
            &["-r", "foo.bin"],
        );
        test_io_opt(
            IOOpt {
                write: Some(Path::new("bar.bin")),
                ..Default::default()
            },
            &["-w", "bar.bin"],
        );
        test_io_opt(
            IOOpt {
                verify: Some(Path::new("/tmp/baz.bin")),
                ..Default::default()
            },
            &["-v", "/tmp/baz.bin"],
        );
        test_io_opt(
            IOOpt {
                erase: true,
                ..Default::default()
            },
            &["-E"],
        );
    }

    #[test]
    fn decode_misc() {
        //use Default::default;
        assert_eq!(
            flashrom_decode_opts(FlashromOpt {
                layout: Some(Path::new("TestLayout")),
                ..Default::default()
            }),
            &["-l", "TestLayout"]
        );

        assert_eq!(
            flashrom_decode_opts(FlashromOpt {
                image: Some("TestImage"),
                ..Default::default()
            }),
            &["-i", "TestImage"]
        );

        assert_eq!(
            flashrom_decode_opts(FlashromOpt {
                flash_name: true,
                verbose: true,
                ..Default::default()
            }),
            &["--flash-name", "-V"]
        );
    }

    #[test]
    fn flashrom_extract_size() {
        use super::flashrom_extract_size;

        assert_eq!(
            flashrom_extract_size(
                "coreboot table found at 0x7cc13000.\n\
                 Found chipset \"Intel Braswell\". Enabling flash write... OK.\n\
                 8388608\n"
            ),
            Ok(8388608)
        );

        assert_eq!(
            flashrom_extract_size("There was a catastrophic error."),
            Err("Found no purely-numeric lines in flashrom output".into())
        );
    }

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
