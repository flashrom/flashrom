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
use flashrom::FlashChip;
use flashrom_tester::{tester, tests};
use std::path::PathBuf;

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
        .arg(Arg::with_name("flashrom_binary").required(true))
        .arg(
            Arg::with_name("ccd_target_type")
                .required(true)
                .possible_values(&["host", "ec", "servo"]),
        )
        .arg(
            Arg::with_name("print-layout")
                .short("l")
                .long("print-layout")
                .help("Print the layout file's contents before running tests"),
        )
        .arg(
            Arg::with_name("log-file")
                .short("o")
                .long("log-file")
                .takes_value(true)
                .help("Write logs to a file rather than stdout"),
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

    logger::init(
        matches.value_of_os("log-file").map(PathBuf::from),
        matches.is_present("log_debug"),
    );
    debug!("Args parsed and logging initialized OK");

    let flashrom_path = matches
        .value_of("flashrom_binary")
        .expect("flashrom_binary should be required");
    let ccd_type = FlashChip::from(
        matches
            .value_of("ccd_target_type")
            .expect("ccd_target_type should be required"),
    )
    .expect("ccd_target_type should admit only known types");

    let print_layout = matches.is_present("print-layout");
    let output_format = matches
        .value_of("output-format")
        .expect("output-format should have a default value")
        .parse::<tester::OutputFormat>()
        .expect("output-format is not a parseable OutputFormat");
    let test_names = matches.values_of("test_name");

    if let Err(e) = tests::generic(
        flashrom_path,
        ccd_type,
        print_layout,
        output_format,
        test_names,
    ) {
        eprintln!("Failed to run tests: {:?}", e);
        std::process::exit(1);
    }
}
