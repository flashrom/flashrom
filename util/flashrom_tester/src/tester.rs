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

use super::rand_util;
use super::types;
use super::utils::{self, LayoutSizes};
use flashrom::FlashromError;
use flashrom::{FlashChip, Flashrom};
use libflashrom::FlashromFlags;
use serde_json::json;
use std::fs::File;
use std::io::Write;
use std::path::Path;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};

// type-signature comes from the return type of lib.rs workers.
type TestError = Box<dyn std::error::Error>;
pub type TestResult = Result<(), TestError>;

pub struct TestEnv<'a> {
    chip_type: FlashChip,
    /// Flashrom instantiation information.
    ///
    /// Where possible, prefer to use methods on the TestEnv rather than delegating
    /// to the raw flashrom functions.
    pub cmd: &'a dyn Flashrom,
    layout: LayoutSizes,

    pub wp: WriteProtectState<'a>,
    /// The path to a file containing the flash contents at test start.
    original_flash_contents: PathBuf,
    /// The path to a file containing flash-sized random data
    random_data: PathBuf,
    /// The path to a file containing layout data.
    pub layout_file: PathBuf,
}

impl<'a> TestEnv<'a> {
    pub fn create(
        chip_type: FlashChip,
        cmd: &'a dyn Flashrom,
        print_layout: bool,
    ) -> Result<Self, FlashromError> {
        let rom_sz = cmd.get_size()?;
        let out = TestEnv {
            chip_type,
            cmd,
            layout: utils::get_layout_sizes(rom_sz)?,
            wp: WriteProtectState::from_hardware(cmd, chip_type)?,
            original_flash_contents: "/tmp/flashrom_tester_golden.bin".into(),
            random_data: "/tmp/random_content.bin".into(),
            layout_file: create_layout_file(rom_sz, Path::new("/tmp/"), print_layout),
        };
        let flags = FlashromFlags::default();
        info!("Set flags: {}", flags);
        out.cmd.set_flags(&flags);

        info!("Stashing golden image for verification/recovery on completion");
        out.cmd.read_into_file(&out.original_flash_contents)?;
        out.cmd.verify_from_file(&out.original_flash_contents)?;

        info!("Generating random flash-sized data");
        rand_util::gen_rand_testdata(&out.random_data, rom_sz as usize)
            .map_err(|io_err| format!("I/O error writing random data file: {:#}", io_err))?;

        Ok(out)
    }

    pub fn run_test<T: TestCase>(&mut self, test: T) -> TestResult {
        let name = test.get_name();
        info!("Beginning test: {}", name);
        let out = test.run(self);
        info!("Completed test: {}; result {:?}", name, out);
        out
    }

    pub fn chip_type(&self) -> FlashChip {
        // This field is not public because it should be immutable to tests,
        // so this getter enforces that it is copied.
        self.chip_type
    }

    /// Return the path to a file that contains random data and is the same size
    /// as the flash chip.
    pub fn random_data_file(&self) -> &Path {
        &self.random_data
    }

    pub fn layout(&self) -> &LayoutSizes {
        &self.layout
    }

    /// Return true if the current Flash contents are the same as the golden image
    /// that was present at the start of testing.
    pub fn is_golden(&self) -> bool {
        self.cmd
            .verify_from_file(&self.original_flash_contents)
            .is_ok()
    }

    /// Do whatever is necessary to make the current Flash contents the same as they
    /// were at the start of testing.
    pub fn ensure_golden(&mut self) -> Result<(), FlashromError> {
        self.wp.set_hw(false)?.set_sw(false)?;
        self.cmd.write_from_file(&self.original_flash_contents)?;
        Ok(())
    }

    /// Attempt to erase the flash.
    pub fn erase(&self) -> Result<(), FlashromError> {
        self.cmd.erase()?;
        Ok(())
    }

    /// Verify that the current Flash contents are the same as the file at the given
    /// path.
    ///
    /// Returns Err if they are not the same.
    pub fn verify(&self, contents_path: &Path) -> Result<(), FlashromError> {
        self.cmd.verify_from_file(contents_path)?;
        Ok(())
    }
}

impl<'a> Drop for TestEnv<'a> {
    fn drop(&mut self) {
        info!("Verifying flash remains unmodified");
        if !self.is_golden() {
            warn!("ROM seems to be in a different state at finish; restoring original");
            if let Err(e) = self.ensure_golden() {
                error!("Failed to write back golden image: {:?}", e);
            }
        }
    }
}

struct WriteProtect {
    hw: bool,
    sw: bool,
}

/// RAII handle for setting write protect in either hardware or software.
///
/// Given an instance, the state of either write protect can be modified by calling
/// `set`. When it goes out of scope, the write protects will be returned
/// to the state they had then it was created.
pub struct WriteProtectState<'a> {
    current: WriteProtect,
    initial: WriteProtect,
    cmd: &'a dyn Flashrom,
    fc: FlashChip,
}

impl<'a> WriteProtectState<'a> {
    /// Initialize a state from the current state of the hardware.
    pub fn from_hardware(cmd: &'a dyn Flashrom, fc: FlashChip) -> Result<Self, FlashromError> {
        let hw = Self::get_hw(cmd)?;
        let sw = Self::get_sw(cmd)?;
        info!("Initial write protect state: HW={} SW={}", hw, sw);

        Ok(WriteProtectState {
            current: WriteProtect { hw, sw },
            initial: WriteProtect { hw, sw },
            cmd,
            fc,
        })
    }

    /// Get the actual hardware write protect state.
    fn get_hw(cmd: &dyn Flashrom) -> Result<bool, String> {
        if cmd.can_control_hw_wp() {
            super::utils::get_hardware_wp()
        } else {
            Ok(false)
        }
    }

    /// Get the actual software write protect state.
    fn get_sw(cmd: &dyn Flashrom) -> Result<bool, FlashromError> {
        let b = cmd.wp_status(true)?;
        Ok(b)
    }

    /// Return true if the current programmer supports setting the hardware
    /// write protect.
    ///
    /// If false, calls to set_hw() will do nothing.
    pub fn can_control_hw_wp(&self) -> bool {
        self.cmd.can_control_hw_wp()
    }

    /// Set the software write protect and check that the state is as expected.
    pub fn set_sw(&mut self, enable: bool) -> Result<&mut Self, String> {
        info!("set_sw request={}, current={}", enable, self.current.sw);
        if self.current.sw != enable {
            self.cmd
                .wp_toggle(/* en= */ enable)
                .map_err(|e| e.to_string())?;
        }
        if Self::get_sw(self.cmd).map_err(|e| e.to_string())? != enable {
            Err(format!(
                "Software write protect did not change state to {} when requested",
                enable
            ))
        } else {
            self.current.sw = enable;
            Ok(self)
        }
    }

    // Set software write protect with a custom range
    pub fn set_range(&mut self, range: (i64, i64), enable: bool) -> Result<&mut Self, String> {
        info!("set_range request={}, current={}", enable, self.current.sw);
        self.cmd
            .wp_range(range, enable)
            .map_err(|e| e.to_string())?;
        let actual_state = Self::get_sw(self.cmd).map_err(|e| e.to_string())?;
        if actual_state != enable {
            Err(format!(
                "set_range request={}, real={}",
                enable, actual_state
            ))
        } else {
            self.current.sw = enable;
            Ok(self)
        }
    }

    /// Set the hardware write protect if supported and check that the state is as expected.
    pub fn set_hw(&mut self, enable: bool) -> Result<&mut Self, String> {
        info!("set_hw request={}, current={}", enable, self.current.hw);
        if self.can_control_hw_wp() {
            if self.current.hw != enable {
                super::utils::toggle_hw_wp(/* dis= */ !enable)?;
            }
            // toggle_hw_wp does check this, but we might not have called toggle_hw_wp so check again.
            if Self::get_hw(self.cmd)? != enable {
                return Err(format!(
                    "Hardware write protect did not change state to {} when requested",
                    enable
                ));
            }
        } else {
            info!(
                "Ignoring attempt to set hardware WP with {:?} programmer",
                self.fc
            );
        }
        self.current.hw = enable;
        Ok(self)
    }

    /// Reset the hardware to what it was when this state was created, reporting errors.
    ///
    /// This behaves exactly like allowing a state to go out of scope, but it can return
    /// errors from that process rather than panicking.
    pub fn close(mut self) -> Result<(), String> {
        let out = self.drop_internal();
        // We just ran drop, don't do it again
        std::mem::forget(self);
        out
    }

    /// Sets both write protects to the state they had when this state was created.
    fn drop_internal(&mut self) -> Result<(), String> {
        // Toggle both protects back to their initial states.
        // Software first because we can't change it once hardware is enabled.
        if self.set_sw(self.initial.sw).is_err() {
            self.set_hw(false)?;
            self.set_sw(self.initial.sw)?;
        }
        self.set_hw(self.initial.hw)?;

        Ok(())
    }
}

impl<'a> Drop for WriteProtectState<'a> {
    fn drop(&mut self) {
        self.drop_internal()
            .expect("Error while dropping WriteProtectState")
    }
}

pub trait TestCase {
    fn get_name(&self) -> &str;
    fn expected_result(&self) -> TestConclusion;
    fn run(&self, env: &mut TestEnv) -> TestResult;
}

impl<S: AsRef<str>, F: Fn(&mut TestEnv) -> TestResult> TestCase for (S, F) {
    fn get_name(&self) -> &str {
        self.0.as_ref()
    }

    fn expected_result(&self) -> TestConclusion {
        TestConclusion::Pass
    }

    fn run(&self, env: &mut TestEnv) -> TestResult {
        (self.1)(env)
    }
}

impl<T: TestCase + ?Sized> TestCase for &T {
    fn get_name(&self) -> &str {
        (*self).get_name()
    }

    fn expected_result(&self) -> TestConclusion {
        (*self).expected_result()
    }

    fn run(&self, env: &mut TestEnv) -> TestResult {
        (*self).run(env)
    }
}

#[allow(dead_code)]
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum TestConclusion {
    Pass,
    Fail,
    UnexpectedPass,
    UnexpectedFail,
}

pub struct ReportMetaData {
    pub chip_name: String,
    pub os_release: String,
    pub cros_release: String,
    pub system_info: String,
    pub bios_info: String,
}

fn decode_test_result(res: TestResult, con: TestConclusion) -> (TestConclusion, Option<TestError>) {
    use TestConclusion::*;

    match (res, con) {
        (Ok(_), Fail) => (UnexpectedPass, None),
        (Err(e), Pass) => (UnexpectedFail, Some(e)),
        _ => (Pass, None),
    }
}

fn create_layout_file(rom_sz: i64, tmp_dir: &Path, print_layout: bool) -> PathBuf {
    info!("Calculate ROM partition sizes & Create the layout file.");
    let layout_sizes = utils::get_layout_sizes(rom_sz).expect("Could not partition rom");

    let layout_file = tmp_dir.join("layout.file");
    let mut f = File::create(&layout_file).expect("Could not create layout file");
    let mut buf: Vec<u8> = vec![];
    utils::construct_layout_file(&mut buf, &layout_sizes).expect("Could not construct layout file");

    f.write_all(&buf).expect("Writing layout file failed");
    if print_layout {
        info!(
            "Dumping layout file as requested:\n{}",
            String::from_utf8_lossy(&buf)
        );
    }
    layout_file
}

pub fn run_all_tests<T, TS>(
    chip: FlashChip,
    cmd: &dyn Flashrom,
    ts: TS,
    terminate_flag: Option<&AtomicBool>,
    print_layout: bool,
) -> Vec<(String, (TestConclusion, Option<TestError>))>
where
    T: TestCase + Copy,
    TS: IntoIterator<Item = T>,
{
    let mut env =
        TestEnv::create(chip, cmd, print_layout).expect("Failed to set up test environment");

    let mut results = Vec::new();
    for t in ts {
        if terminate_flag
            .map(|b| b.load(Ordering::Acquire))
            .unwrap_or(false)
        {
            break;
        }

        let result = decode_test_result(env.run_test(t), t.expected_result());
        results.push((t.get_name().into(), result));
    }
    results
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum OutputFormat {
    Pretty,
    Json,
}

impl std::str::FromStr for OutputFormat {
    type Err = ();

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        use OutputFormat::*;

        if s.eq_ignore_ascii_case("pretty") {
            Ok(Pretty)
        } else if s.eq_ignore_ascii_case("json") {
            Ok(Json)
        } else {
            Err(())
        }
    }
}

pub fn collate_all_test_runs(
    truns: &[(String, (TestConclusion, Option<TestError>))],
    meta_data: ReportMetaData,
    format: OutputFormat,
) {
    match format {
        OutputFormat::Pretty => {
            let color = if atty::is(atty::Stream::Stdout) {
                types::COLOR
            } else {
                types::NOCOLOR
            };
            println!();
            println!("  =============================");
            println!("  =====  AVL qual RESULTS  ====");
            println!("  =============================");
            println!();
            println!("  %---------------------------%");
            println!("   os release: {}", meta_data.os_release);
            println!("   cros release: {}", meta_data.cros_release);
            println!("   chip name: {}", meta_data.chip_name);
            println!("   system info: \n{}", meta_data.system_info);
            println!("   bios info: \n{}", meta_data.bios_info);
            println!("  %---------------------------%");
            println!();

            for trun in truns.iter() {
                let (name, (result, error)) = trun;
                if *result != TestConclusion::Pass {
                    println!(
                        " {} {}",
                        style!(format!(" <+> {} test:", name), color.bold, color),
                        style_dbg!(result, color.red, color)
                    );
                    match error {
                        None => {}
                        Some(e) => info!(" - {} failure details:\n{}", name, e.to_string()),
                    };
                } else {
                    println!(
                        " {} {}",
                        style!(format!(" <+> {} test:", name), color.bold, color),
                        style_dbg!(result, color.green, color)
                    );
                }
            }
            println!();
        }
        OutputFormat::Json => {
            use serde_json::{Map, Value};

            let mut all_pass = true;
            let mut tests = Map::<String, Value>::new();
            for (name, (result, error)) in truns {
                let passed = *result == TestConclusion::Pass;
                all_pass &= passed;

                let error = match error {
                    Some(e) => Value::String(format!("{:#?}", e)),
                    None => Value::Null,
                };

                assert!(
                    !tests.contains_key(name),
                    "Found multiple tests named {:?}",
                    name
                );
                tests.insert(
                    name.into(),
                    json!({
                        "pass": passed,
                        "error": error,
                    }),
                );
            }

            let json = json!({
                "pass": all_pass,
                "metadata": {
                    "os_release": meta_data.os_release,
                    "chip_name": meta_data.chip_name,
                    "system_info": meta_data.system_info,
                    "bios_info": meta_data.bios_info,
                },
                "tests": tests,
            });
            println!("{:#}", json);
        }
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn decode_test_result() {
        use super::decode_test_result;
        use super::TestConclusion::*;

        let (result, err) = decode_test_result(Ok(()), Pass);
        assert_eq!(result, Pass);
        assert!(err.is_none());

        let (result, err) = decode_test_result(Ok(()), Fail);
        assert_eq!(result, UnexpectedPass);
        assert!(err.is_none());

        let (result, err) = decode_test_result(Err("broken".into()), Pass);
        assert_eq!(result, UnexpectedFail);
        assert!(err.is_some());

        let (result, err) = decode_test_result(Err("broken".into()), Fail);
        assert_eq!(result, Pass);
        assert!(err.is_none());
    }

    #[test]
    fn output_format_round_trip() {
        use super::OutputFormat::{self, *};

        assert_eq!(format!("{:?}", Pretty).parse::<OutputFormat>(), Ok(Pretty));
        assert_eq!(format!("{:?}", Json).parse::<OutputFormat>(), Ok(Json));
    }
}
