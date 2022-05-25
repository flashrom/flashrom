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

use std::process::Command;

#[derive(Default)]
pub struct FlashromOpt<'a> {
    pub wp_opt: WPOpt,
    pub io_opt: IOOpt<'a>,

    pub layout: Option<&'a str>, // -l <file>
    pub image: Option<&'a str>,  // -i <name>

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
    pub read: Option<&'a str>,   // -r <file>
    pub write: Option<&'a str>,  // -w <file>
    pub verify: Option<&'a str>, // -v <file>
    pub erase: bool,             // -E
}

#[derive(PartialEq, Debug)]
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
        None => return Err("Found no purely-numeric lines in flashrom output".into()),
        Some(Err(e)) => {
            return Err(format!(
                "Failed to parse flashrom size output as integer: {}",
                e
            ))
        }
        Some(Ok(sz)) => Ok(sz),
    }
}

impl FlashromCmd {
    fn dispatch(&self, fropt: FlashromOpt) -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
        let params = flashrom_decode_opts(fropt);
        flashrom_dispatch(self.path.as_str(), &params, self.fc)
    }
}

impl crate::Flashrom for FlashromCmd {
    fn get_size(&self) -> Result<i64, FlashromError> {
        let (stdout, _) = flashrom_dispatch(self.path.as_str(), &["--flash-size"], self.fc)?;
        let sz = String::from_utf8_lossy(&stdout);

        flashrom_extract_size(&sz)
    }

    fn name(&self) -> Result<(String, String), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                ..Default::default()
            },

            flash_name: true,

            ..Default::default()
        };

        let (stdout, stderr) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        let eoutput = String::from_utf8_lossy(stderr.as_slice());
        debug!("name()'stdout: {:#?}.", output);
        debug!("name()'stderr: {:#?}.", eoutput);

        match extract_flash_name(&output) {
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

        let (stdout, stderr) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        let eoutput = String::from_utf8_lossy(stderr.as_slice());
        debug!("write_file_with_layout()'stdout:\n{}.", output);
        debug!("write_file_with_layout()'stderr:\n{}.", eoutput);
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

        let (stdout, stderr) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        let eoutput = String::from_utf8_lossy(stderr.as_slice());
        debug!("wp_range()'stdout:\n{}.", output);
        debug!("wp_range()'stderr:\n{}.", eoutput);
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

        let (stdout, _) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        if output.len() == 0 {
            return Err(
                "wp_list isn't supported on platforms using the Linux kernel SPI driver wp_list"
                    .into(),
            );
        }
        Ok(output.to_string())
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

        let (stdout, _) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());

        debug!("wp_status():\n{}", output);

        let s = std::format!("write protect is {}abled", status);
        Ok(output.contains(&s))
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
                range: range,
                enable: en,
                disable: !en,
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, stderr) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        let eoutput = String::from_utf8_lossy(stderr.as_slice());

        debug!("wp_toggle()'stdout:\n{}.", output);
        debug!("wp_toggle()'stderr:\n{}.", eoutput);

        match self.wp_status(true) {
            Ok(_ret) => {
                info!("Successfully {}abled write-protect", status);
                Ok(true)
            }
            Err(e) => Err(format!("Cannot {}able write-protect: {}", status, e)),
        }
    }

    fn read(&self, path: &str) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                read: Some(path),
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        debug!("read():\n{}", output);
        Ok(())
    }

    fn write(&self, path: &str) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                write: Some(path),
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        debug!("write():\n{}", output);
        Ok(())
    }

    fn verify(&self, path: &str) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: IOOpt {
                verify: Some(path),
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        debug!("verify():\n{}", output);
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

        let (stdout, _) = self.dispatch(opts)?;
        let output = String::from_utf8_lossy(stdout.as_slice());
        debug!("erase():\n{}", output);
        Ok(())
    }

    fn can_control_hw_wp(&self) -> bool {
        self.fc.can_control_hw_wp()
    }
}

fn flashrom_decode_opts(opts: FlashromOpt) -> Vec<String> {
    let mut params = Vec::<String>::new();

    // ------------ WARNING !!! ------------
    // each param must NOT contain spaces!
    // -------------------------------------

    // wp_opt
    if opts.wp_opt.range.is_some() {
        let (x0, x1) = opts.wp_opt.range.unwrap();
        params.push("--wp-range".to_string());
        params.push(hex_range_string(x0, x1));
    }
    if opts.wp_opt.status {
        params.push("--wp-status".to_string());
    } else if opts.wp_opt.list {
        params.push("--wp-list".to_string());
    } else if opts.wp_opt.enable {
        params.push("--wp-enable".to_string());
    } else if opts.wp_opt.disable {
        params.push("--wp-disable".to_string());
    }

    // io_opt
    if opts.io_opt.read.is_some() {
        params.push("-r".to_string());
        params.push(opts.io_opt.read.unwrap().to_string());
    } else if opts.io_opt.write.is_some() {
        params.push("-w".to_string());
        params.push(opts.io_opt.write.unwrap().to_string());
    } else if opts.io_opt.verify.is_some() {
        params.push("-v".to_string());
        params.push(opts.io_opt.verify.unwrap().to_string());
    } else if opts.io_opt.erase {
        params.push("-E".to_string());
    }

    // misc_opt
    if opts.layout.is_some() {
        params.push("-l".to_string());
        params.push(opts.layout.unwrap().to_string());
    }
    if opts.image.is_some() {
        params.push("-i".to_string());
        params.push(opts.image.unwrap().to_string());
    }

    if opts.flash_name {
        params.push("--flash-name".to_string());
    }
    if opts.verbose {
        params.push("-V".to_string());
    }

    params
}

fn flashrom_dispatch<S: AsRef<str>>(
    path: &str,
    params: &[S],
    fc: FlashChip,
) -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
    // from man page:
    //  ' -p, --programmer <name>[:parameter[,parameter[,parameter]]] '
    let mut args: Vec<&str> = vec!["-p", FlashChip::to(fc)];
    args.extend(params.iter().map(S::as_ref));

    info!("flashrom_dispatch() running: {} {:?}", path, args);

    let output = match Command::new(path).args(&args).output() {
        Ok(x) => x,
        Err(e) => return Err(format!("Failed to run flashrom: {}", e)),
    };
    if !output.status.success() {
        // There is two cases on failure;
        //  i. ) A bad exit code,
        //  ii.) A SIG killed us.
        match output.status.code() {
            Some(code) => {
                return Err(format!(
                    "{}\nExited with error code: {}",
                    String::from_utf8_lossy(&output.stderr),
                    code
                ));
            }
            None => return Err("Process terminated by a signal".into()),
        }
    }

    Ok((output.stdout, output.stderr))
}

pub fn dut_ctrl_toggle_wp(en: bool) -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
    let args = if en {
        ["fw_wp_en:off", "fw_wp:on"]
    } else {
        ["fw_wp_en:on", "fw_wp:off"]
    };
    dut_ctrl(&args)
}

pub fn dut_ctrl_servo_type() -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
    let args = ["servo_type"];
    dut_ctrl(&args)
}

fn dut_ctrl(args: &[&str]) -> Result<(Vec<u8>, Vec<u8>), FlashromError> {
    let output = match Command::new("dut-control").args(args).output() {
        Ok(x) => x,
        Err(e) => return Err(format!("Failed to run dut-control: {}", e)),
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
    format!("{:#08X},{:#08X}", s, l).to_string()
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
                read: Some("foo.bin"),
                ..Default::default()
            },
            &["-r", "foo.bin"],
        );
        test_io_opt(
            IOOpt {
                write: Some("bar.bin"),
                ..Default::default()
            },
            &["-w", "bar.bin"],
        );
        test_io_opt(
            IOOpt {
                verify: Some("/tmp/baz.bin"),
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
                layout: Some("TestLayout"),
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
