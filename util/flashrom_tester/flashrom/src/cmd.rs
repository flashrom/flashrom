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

use crate::{FlashChip, FlashromError};

use libflashrom::FlashromFlags;

use std::{
    ffi::{OsStr, OsString},
    path::Path,
    process::Command,
};

#[derive(Default)]
pub struct FlashromOpt<'a> {
    pub wp_opt: WPOpt,
    pub io_opt: Option<IOOpt<'a>>,

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

pub enum OperationArgs<'a> {
    /// The file is the whole chip.
    EntireChip(&'a Path),
    /// File is the size of the full chip, limited to a single named region.
    ///
    /// The required path is the file to use, and the optional path is a layout file
    /// specifying how to locate regions (if unspecified, flashrom will attempt
    /// to discover the layout itself).
    FullFileRegion(&'a str, &'a Path, Option<&'a Path>),
    /// File is the size of the single named region only.
    ///
    /// The required path is the file to use, and the optional path is a layout file
    /// specifying how to locate regions (if unspecified, flashrom will attempt
    /// to discover the layout itself).
    RegionFileRegion(&'a str, &'a Path, Option<&'a Path>), // The file contains only the region
}

pub enum IOOpt<'a> {
    Read(OperationArgs<'a>),   // -r <file>
    Write(OperationArgs<'a>),  // -w <file>
    Verify(OperationArgs<'a>), // -v <file>
    Erase,                     // -E
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
            flash_name: true,
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts, "name")?;
        match extract_flash_name(&stdout) {
            None => Err("Didn't find chip vendor/name in flashrom output".into()),
            Some((vendor, name)) => Ok((vendor.into(), name.into())),
        }
    }

    fn write_from_file_region(
        &self,
        path: &Path,
        region: &str,
        layout: &Path,
    ) -> Result<bool, FlashromError> {
        let opts = FlashromOpt {
            io_opt: Some(IOOpt::Write(OperationArgs::FullFileRegion(
                region,
                path,
                Some(layout),
            ))),
            ..Default::default()
        };

        self.dispatch(opts, "write_file_with_layout")?;
        Ok(true)
    }

    fn wp_range(&self, range: (i64, i64), en: bool) -> Result<bool, FlashromError> {
        let opts = FlashromOpt {
            wp_opt: WPOpt {
                enable: en,
                disable: !en,
                range: Some(range),
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
        let protection_mode = if en { "hardware" } else { "disable" };
        info!("See if chip write protect is {}abled", status);

        let opts = FlashromOpt {
            wp_opt: WPOpt {
                status: true,
                ..Default::default()
            },
            ..Default::default()
        };

        let (stdout, _) = self.dispatch(opts, "wp_status")?;
        let s = std::format!("Protection mode: {}", protection_mode);
        Ok(stdout.contains(&s))
    }

    fn wp_toggle(&self, en: bool) -> Result<bool, FlashromError> {
        let range = if en {
            let rom_sz: i64 = self.get_size()?;
            (0, rom_sz) // (start, len)
        } else {
            (0, 0)
        };
        self.wp_range(range, en)?;
        let status = if en { "en" } else { "dis" };
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
            io_opt: Some(IOOpt::Read(OperationArgs::EntireChip(path))),
            ..Default::default()
        };

        self.dispatch(opts, "read_into_file")?;
        Ok(())
    }

    fn read_region_into_file(&self, path: &Path, region: &str) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: Some(IOOpt::Read(OperationArgs::RegionFileRegion(
                region, path, None,
            ))),
            ..Default::default()
        };

        self.dispatch(opts, "read_region_into_file")?;
        Ok(())
    }

    fn write_from_file(&self, path: &Path) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: Some(IOOpt::Write(OperationArgs::EntireChip(path))),
            ..Default::default()
        };

        self.dispatch(opts, "write_from_file")?;
        Ok(())
    }

    fn verify_from_file(&self, path: &Path) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: Some(IOOpt::Verify(OperationArgs::EntireChip(path))),
            ..Default::default()
        };

        self.dispatch(opts, "verify_from_file")?;
        Ok(())
    }

    fn verify_region_from_file(&self, path: &Path, region: &str) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: Some(IOOpt::Verify(OperationArgs::RegionFileRegion(
                region, path, None,
            ))),
            ..Default::default()
        };

        self.dispatch(opts, "verify_region_from_file")?;
        Ok(())
    }

    fn erase(&self) -> Result<(), FlashromError> {
        let opts = FlashromOpt {
            io_opt: Some(IOOpt::Erase),
            ..Default::default()
        };

        self.dispatch(opts, "erase")?;
        Ok(())
    }

    fn can_control_hw_wp(&self) -> bool {
        self.fc.can_control_hw_wp()
    }

    fn set_flags(&self, flags: &FlashromFlags) -> () {
        // The flashrom CLI sets its own default flags,
        // and we currently have no need for custom flags,
        // so this set_flags function is intentionally a no-op.
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
    fn add_operation_args(opts: OperationArgs, params: &mut Vec<OsString>) {
        let (file, region, layout) = match opts {
            OperationArgs::EntireChip(file) => (Some(file), None, None),
            OperationArgs::FullFileRegion(region, file, layout) => {
                (Some(file), Some(region.to_string()), layout)
            }
            OperationArgs::RegionFileRegion(region, file, layout) => (
                None,
                Some(format!("{region}:{}", file.to_string_lossy())),
                layout,
            ),
        };
        if let Some(file) = file {
            params.push(file.into())
        }
        if let Some(region) = region {
            params.push("--include".into());
            params.push(region.into())
        }
        if let Some(layout) = layout {
            params.push("--layout".into());
            params.push(layout.into())
        }
    }
    if let Some(io) = opts.io_opt {
        match io {
            IOOpt::Read(args) => {
                params.push("-r".into());
                add_operation_args(args, &mut params);
            }
            IOOpt::Write(args) => {
                params.push("-w".into());
                add_operation_args(args, &mut params);
            }
            IOOpt::Verify(args) => {
                params.push("-v".into());
                add_operation_args(args, &mut params);
            }
            IOOpt::Erase => params.push("-E".into()),
        }
    }

    // misc_opt
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
                    io_opt: Some(opts),
                    ..Default::default()
                }),
                expected
            );
        }

        test_io_opt(
            IOOpt::Read(crate::cmd::OperationArgs::EntireChip(Path::new("foo.bin"))),
            &["-r", "foo.bin"],
        );
        test_io_opt(
            IOOpt::Write(crate::cmd::OperationArgs::EntireChip(Path::new("bar.bin"))),
            &["-w", "bar.bin"],
        );
        test_io_opt(
            IOOpt::Verify(crate::cmd::OperationArgs::EntireChip(Path::new("baz.bin"))),
            &["-v", "baz.bin"],
        );
        test_io_opt(IOOpt::Erase, &["-E"]);
        test_io_opt(
            IOOpt::Read(crate::cmd::OperationArgs::FullFileRegion(
                "RO",
                Path::new("foo.bin"),
                Some(Path::new("baz.bin")),
            )),
            &["-r", "foo.bin", "--include", "RO", "--layout", "baz.bin"],
        );

        test_io_opt(
            IOOpt::Read(crate::cmd::OperationArgs::RegionFileRegion(
                "foo",
                Path::new("bar.bin"),
                None,
            )),
            &["-r", "--include", "foo:bar.bin"],
        )
    }

    #[test]
    fn decode_misc() {
        //use Default::default;

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
