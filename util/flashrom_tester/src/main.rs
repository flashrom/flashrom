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

mod logger;

use clap::{App, Arg};
use flashrom::{FlashChip, Flashrom, FlashromCmd, FlashromLib};
use flashrom_tester::{tester, tests};
use std::sync::atomic::AtomicBool;

pub mod built_info {
    include!(concat!(env!("OUT_DIR"), "/built.rs"));
}

fn main() {
    let matches = App::new("flashrom_tester")
        .long_version(&*format!(
            "{}-{}\n\
             Target: {}\n\
             Profile: {}\n\
             Features: {:?}\n\
             Build time: {}\n\
             Compiler: {}",
            built_info::PKG_VERSION,
            option_env!("VCSID").unwrap_or("<unknown>"),
            built_info::TARGET,
            built_info::PROFILE,
            built_info::FEATURES,
            built_info::BUILT_TIME_UTC,
            built_info::RUSTC_VERSION,
        ))
        .arg(
            Arg::with_name("libflashrom")
                .long("libflashrom")
                .takes_value(false)
                .help("Test the flashrom library instead of a binary"),
        )
        .arg(
            Arg::with_name("flashrom_binary")
                .long("flashrom_binary")
                .short("b")
                .takes_value(true)
                .required_unless("libflashrom")
                .conflicts_with("libflashrom")
                .help("Path to flashrom binary to test"),
        )
        .arg(
            Arg::with_name("ccd_target_type")
                .required(true)
                .possible_values(&["internal"]),
        )
        .arg(
            Arg::with_name("print-layout")
                .short("l")
                .long("print-layout")
                .help("Print the layout file's contents before running tests"),
        )
        .arg(
            Arg::with_name("log_debug")
                .short("d")
                .long("debug")
                .help("Write detailed logs, for debugging"),
        )
        .arg(
            Arg::with_name("output-format")
                .short("f")
                .long("output-format")
                .help("Set the test report format")
                .takes_value(true)
                .case_insensitive(true)
                .possible_values(&["pretty", "json"])
                .default_value("pretty"),
        )
        .arg(
            Arg::with_name("test_name")
                .multiple(true)
                .help("Names of individual tests to run (run all if unspecified)"),
        )
        .get_matches();

    logger::init(matches.is_present("log_debug"));
    debug!("Args parsed and logging initialized OK");

    debug!("Collecting crossystem info");
    let crossystem =
        flashrom_tester::utils::collect_crosssystem(&[]).expect("could not run crossystem");

    let ccd_type = FlashChip::from(
        matches
            .value_of("ccd_target_type")
            .expect("ccd_target_type should be required"),
    )
    .expect("ccd_target_type should admit only known types");

    let cmd: Box<dyn Flashrom> = if matches.is_present("libflashrom") {
        Box::new(FlashromLib::new(
            ccd_type,
            if matches.is_present("log_debug") {
                flashrom::FLASHROM_MSG_DEBUG
            } else {
                flashrom::FLASHROM_MSG_WARN
            },
        ))
    } else {
        Box::new(FlashromCmd {
            path: matches
                .value_of("flashrom_binary")
                .expect("flashrom_binary is required")
                .to_string(),
            fc: ccd_type,
        })
    };

    let print_layout = matches.is_present("print-layout");
    let output_format = matches
        .value_of("output-format")
        .expect("output-format should have a default value")
        .parse::<tester::OutputFormat>()
        .expect("output-format is not a parseable OutputFormat");
    let test_names = matches.values_of("test_name");

    if let Err(e) = tests::generic(
        cmd.as_ref(),
        ccd_type,
        print_layout,
        output_format,
        test_names,
        Some(handle_sigint()),
        crossystem,
    ) {
        eprintln!("Failed to run tests: {:?}", e);
        std::process::exit(1);
    }
}

/// Catch exactly one SIGINT, printing a message in response and setting a flag.
///
/// The returned value is false by default, becoming true after a SIGINT is
/// trapped.
///
/// Once a signal is trapped, the default behavior is restored (terminating
/// the process) for future signals.
fn handle_sigint() -> &'static AtomicBool {
    use libc::c_int;
    use std::sync::atomic::Ordering;

    unsafe {
        let _ = libc::signal(libc::SIGINT, sigint_handler as libc::sighandler_t);
    }
    static TERMINATE_FLAG: AtomicBool = AtomicBool::new(false);

    extern "C" fn sigint_handler(_: c_int) {
        const STDERR_FILENO: c_int = 2;
        static MESSAGE: &[u8] = b"
WARNING: terminating tests prematurely may leave Flash in an inconsistent state,
rendering your machine unbootable. Testing will end on completion of the current
test, or press ^C again to exit immediately (possibly bricking your machine).
";

        // Use raw write() because signal-safety is a very hard problem. Safe because this doesn't
        // modify any memory.
        let _ = unsafe {
            libc::write(
                STDERR_FILENO,
                MESSAGE.as_ptr() as *const libc::c_void,
                MESSAGE.len() as libc::size_t,
            )
        };
        unsafe {
            let _ = libc::signal(libc::SIGINT, libc::SIG_DFL);
        }
        TERMINATE_FLAG.store(true, Ordering::Release);
    }

    &TERMINATE_FLAG
}
