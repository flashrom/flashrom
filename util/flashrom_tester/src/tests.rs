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

use super::cros_sysinfo;
use super::tester::{self, OutputFormat, TestCase, TestEnv, TestResult};
use super::utils::{self, LayoutNames};
use flashrom::{FlashChip, Flashrom};
use std::collections::{HashMap, HashSet};
use std::convert::TryInto;
use std::fs::{self, File};
use std::io::BufRead;
use std::sync::atomic::AtomicBool;

const ELOG_FILE: &str = "/tmp/elog.file";
const FW_MAIN_B_PATH: &str = "/tmp/FW_MAIN_B.bin";

/// Iterate over tests, yielding only those tests with names matching filter_names.
///
/// If filter_names is None, all tests will be run. None is distinct from Some(∅);
//  Some(∅) runs no tests.
///
/// Name comparisons are performed in lower-case: values in filter_names must be
/// converted to lowercase specifically.
///
/// When an entry in filter_names matches a test, it is removed from that set.
/// This allows the caller to determine if any entries in the original set failed
/// to match any test, which may be user error.
fn filter_tests<'n, 't: 'n, T: TestCase>(
    tests: &'t [T],
    filter_names: &'n mut Option<HashSet<String>>,
) -> impl 'n + Iterator<Item = &'t T> {
    tests.iter().filter(move |test| match filter_names {
        // Accept all tests if no names are given
        None => true,
        Some(ref mut filter_names) => {
            // Pop a match to the test name from the filter set, retaining the test
            // if there was a match.
            filter_names.remove(&test.get_name().to_lowercase())
        }
    })
}

/// Run tests.
///
/// Only returns an Error if there was an internal error; test failures are Ok.
///
/// test_names is the case-insensitive names of tests to run; if None, then all
/// tests are run. Provided names that don't match any known test will be logged
/// as a warning.
#[allow(clippy::or_fun_call)] // This is used for to_string here and we don't care.
pub fn generic<'a, TN: Iterator<Item = &'a str>>(
    cmd: &dyn Flashrom,
    fc: FlashChip,
    print_layout: bool,
    output_format: OutputFormat,
    test_names: Option<TN>,
    terminate_flag: Option<&AtomicBool>,
    crossystem: String,
) -> Result<(), Box<dyn std::error::Error>> {
    utils::ac_power_warning();

    info!("Record crossystem information.\n{}", crossystem);

    // Register tests to run:
    let tests: &[&dyn TestCase] = &[
        &("Get_device_name", get_device_name_test),
        &("Coreboot_ELOG_sanity", elog_sanity_test),
        &("Host_is_ChromeOS", host_is_chrome_test),
        &("WP_Region_List", wp_region_list_test),
        &("Erase_and_Write", erase_write_test),
        &("Fail_to_verify", verify_fail_test),
        &("HWWP_Locks_SWWP", hwwp_locks_swwp_test),
        &("Lock_top_quad", partial_lock_test(LayoutNames::TopQuad)),
        &(
            "Lock_bottom_quad",
            partial_lock_test(LayoutNames::BottomQuad),
        ),
        &(
            "Lock_bottom_half",
            partial_lock_test(LayoutNames::BottomHalf),
        ),
        &("Lock_top_half", partial_lock_test(LayoutNames::TopHalf)),
    ];

    // Limit the tests to only those requested, unless none are requested
    // in which case all tests are included.
    let mut filter_names: Option<HashSet<String>> =
        test_names.map(|names| names.map(|s| s.to_lowercase()).collect());
    let tests = filter_tests(tests, &mut filter_names);

    let chip_name = cmd
        .name()
        .map(|x| format!("vendor=\"{}\" name=\"{}\"", x.0, x.1))
        .unwrap_or("<Unknown chip>".into());

    // ------------------------.
    // Run all the tests and collate the findings:
    let results = tester::run_all_tests(fc, cmd, tests, terminate_flag, print_layout);

    // Any leftover filtered names were specified to be run but don't exist
    for leftover in filter_names.iter().flatten() {
        warn!("No test matches filter name \"{}\"", leftover);
    }

    let os_release = sys_info::os_release().unwrap_or("<Unknown OS>".to_string());
    let cros_release = cros_sysinfo::release_description()
        .unwrap_or("<Unknown or not a ChromeOS release>".to_string());
    let system_info = cros_sysinfo::system_info().unwrap_or("<Unknown System>".to_string());
    let bios_info = cros_sysinfo::bios_info().unwrap_or("<Unknown BIOS>".to_string());

    let meta_data = tester::ReportMetaData {
        chip_name,
        os_release,
        cros_release,
        system_info,
        bios_info,
    };
    tester::collate_all_test_runs(&results, meta_data, output_format);
    Ok(())
}

/// Query the programmer and chip name.
/// Success means we got something back, which is good enough.
fn get_device_name_test(env: &mut TestEnv) -> TestResult {
    env.cmd.name()?;
    Ok(())
}

/// List the write-protectable regions of flash.
/// NOTE: This is not strictly a 'test' as it is allowed to fail on some platforms.
///       However, we will warn when it does fail.
fn wp_region_list_test(env: &mut TestEnv) -> TestResult {
    match env.cmd.wp_list() {
        Ok(list_str) => info!("\n{}", list_str),
        Err(e) => warn!("{}", e),
    };
    Ok(())
}

/// Verify that enabling hardware and software write protect prevents chip erase.
fn erase_write_test(env: &mut TestEnv) -> TestResult {
    if !env.is_golden() {
        info!("Memory has been modified; reflashing to ensure erasure can be detected");
        env.ensure_golden()?;
    }

    // With write protect enabled erase should fail.
    env.wp.set_sw(true)?.set_hw(true)?;
    if env.erase().is_ok() {
        info!("Flashrom returned Ok but this may be incorrect; verifying");
        if !env.is_golden() {
            return Err("Hardware write protect asserted however can still erase!".into());
        }
        info!("Erase claimed to succeed but verify is Ok; assume erase failed");
    }

    // With write protect disabled erase should succeed.
    env.wp.set_hw(false)?.set_sw(false)?;
    env.erase()?;
    if env.is_golden() {
        return Err("Successful erase didn't modify memory".into());
    }

    Ok(())
}

/// Verify that enabling hardware write protect prevents disabling software write protect.
fn hwwp_locks_swwp_test(env: &mut TestEnv) -> TestResult {
    if !env.wp.can_control_hw_wp() {
        return Err("Lock test requires ability to control hardware write protect".into());
    }

    env.wp.set_hw(false)?.set_sw(true)?;
    // Toggling software WP off should work when hardware WP is off.
    // Then enable software WP again for the next test.
    env.wp.set_sw(false)?.set_sw(true)?;

    // Toggling software WP off should not work when hardware WP is on.
    env.wp.set_hw(true)?;
    if env.wp.set_sw(false).is_ok() {
        return Err("Software WP was reset despite hardware WP being enabled".into());
    }
    Ok(())
}

/// Check that the elog contains *something*, as an indication that Coreboot
/// is actually able to write to the Flash. This only makes sense for chips
/// running Coreboot, which we assume is just host.
fn elog_sanity_test(env: &mut TestEnv) -> TestResult {
    if env.chip_type() != FlashChip::INTERNAL {
        info!("Skipping ELOG sanity check for non-internal chip");
        return Ok(());
    }
    // flash should be back in the golden state
    env.ensure_golden()?;

    const ELOG_RW_REGION_NAME: &str = "RW_ELOG";
    env.cmd
        .read_region_into_file(ELOG_FILE.as_ref(), ELOG_RW_REGION_NAME)?;

    // Just checking for the magic numer
    // TODO: improve this test to read the events
    if fs::metadata(ELOG_FILE)?.len() < 4 {
        return Err("ELOG contained no data".into());
    }
    let data = fs::read(ELOG_FILE)?;
    if u32::from_le_bytes(data[0..4].try_into()?) != 0x474f4c45 {
        return Err("ELOG had bad magic number".into());
    }
    Ok(())
}

/// Check that we are running ChromiumOS.
fn host_is_chrome_test(_env: &mut TestEnv) -> TestResult {
    let release_info = if let Ok(f) = File::open("/etc/os-release") {
        let buf = std::io::BufReader::new(f);
        parse_os_release(buf.lines().flatten())
    } else {
        info!("Unable to read /etc/os-release to probe system information");
        HashMap::new()
    };

    match release_info.get("ID") {
        Some(id) if id == "chromeos" || id == "chromiumos" => Ok(()),
        oid => {
            let id = match oid {
                Some(s) => s,
                None => "UNKNOWN",
            };
            Err(format!(
                "Test host os-release \"{}\" should be but is not chromeos",
                id
            )
            .into())
        }
    }
}

/// Verify that software write protect for a range protects only the requested range.
fn partial_lock_test(section: LayoutNames) -> impl Fn(&mut TestEnv) -> TestResult {
    move |env: &mut TestEnv| {
        // Need a clean image for verification
        env.ensure_golden()?;

        let (wp_section_name, start, len) = utils::layout_section(env.layout(), section);
        // Disable hardware WP so we can modify the protected range.
        env.wp.set_hw(false)?;
        // Then enable software WP so the range is enforced and enable hardware
        // WP so that flashrom does not disable software WP during the
        // operation.
        env.wp.set_range((start, len), true)?;
        env.wp.set_hw(true)?;

        // Check that we cannot write to the protected region.
        if env
            .cmd
            .write_from_file_region(env.random_data_file(), wp_section_name, &env.layout_file)
            .is_ok()
        {
            return Err(
                "Section should be locked, should not have been overwritable with random data"
                    .into(),
            );
        }
        if !env.is_golden() {
            return Err("Section didn't lock, has been overwritten with random data!".into());
        }

        // Check that we can write to the non protected region.
        let (non_wp_section_name, _, _) =
            utils::layout_section(env.layout(), section.get_non_overlapping_section());
        env.cmd.write_from_file_region(
            env.random_data_file(),
            non_wp_section_name,
            &env.layout_file,
        )?;

        Ok(())
    }
}

/// Check that flashrom 'verify' will fail if the provided data does not match the chip data.
fn verify_fail_test(env: &mut TestEnv) -> TestResult {
    env.ensure_golden()?;
    // Verify that verify is Ok when the data matches. We verify only a region
    // and not the whole chip because coprocessors or firmware may have written
    // some data in other regions.
    env.cmd
        .read_region_into_file(FW_MAIN_B_PATH.as_ref(), "FW_MAIN_B")?;
    env.cmd
        .verify_region_from_file(FW_MAIN_B_PATH.as_ref(), "FW_MAIN_B")?;

    // Verify that verify is false when the data does not match
    match env.verify(env.random_data_file()) {
        Ok(_) => Err("Verification says flash is full of random data".into()),
        Err(_) => Ok(()),
    }
}

/// Ad-hoc parsing of os-release(5); mostly according to the spec,
/// but ignores quotes and escaping.
fn parse_os_release<I: IntoIterator<Item = String>>(lines: I) -> HashMap<String, String> {
    fn parse_line(line: String) -> Option<(String, String)> {
        if line.is_empty() || line.starts_with('#') {
            return None;
        }

        let delimiter = match line.find('=') {
            Some(idx) => idx,
            None => {
                warn!("os-release entry seems malformed: {:?}", line);
                return None;
            }
        };
        Some((
            line[..delimiter].to_owned(),
            line[delimiter + 1..].to_owned(),
        ))
    }

    lines.into_iter().filter_map(parse_line).collect()
}

#[test]
fn test_parse_os_release() {
    let lines = [
        "BUILD_ID=12516.0.0",
        "# this line is a comment followed by an empty line",
        "",
        "ID_LIKE=chromiumos",
        "ID=chromeos",
        "VERSION=79",
        "EMPTY_VALUE=",
    ];
    let map = parse_os_release(lines.iter().map(|&s| s.to_owned()));

    fn get<'a, 'b>(m: &'a HashMap<String, String>, k: &'b str) -> Option<&'a str> {
        m.get(k).map(|s| s.as_ref())
    }

    assert_eq!(get(&map, "ID"), Some("chromeos"));
    assert_eq!(get(&map, "BUILD_ID"), Some("12516.0.0"));
    assert_eq!(get(&map, "EMPTY_VALUE"), Some(""));
    assert_eq!(get(&map, ""), None);
}

#[test]
fn test_name_filter() {
    let test_one = ("Test One", |_: &mut TestEnv| Ok(()));
    let test_two = ("Test Two", |_: &mut TestEnv| Ok(()));
    let tests: &[&dyn TestCase] = &[&test_one, &test_two];

    let mut names = None;
    // All tests pass through
    assert_eq!(filter_tests(tests, &mut names).count(), 2);

    names = Some(["test two"].iter().map(|s| s.to_string()).collect());
    // Filtered out test one
    assert_eq!(filter_tests(tests, &mut names).count(), 1);

    names = Some(["test three"].iter().map(|s| s.to_string()).collect());
    // No tests emitted
    assert_eq!(filter_tests(tests, &mut names).count(), 0);
    // Name got left behind because no test matched it
    assert_eq!(names.unwrap().len(), 1);
}
