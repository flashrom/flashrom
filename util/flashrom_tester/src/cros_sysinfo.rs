//
// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
// SPDX-FileCopyrightText: 2019, Google Inc. All rights reserved.
//

use std::ffi::OsStr;
use std::fs;
use std::io::Result as IoResult;
use std::process::{Command, Stdio};

use super::utils;

fn dmidecode_dispatch<S: AsRef<OsStr>>(args: &[S]) -> IoResult<String> {
    let output = Command::new("/usr/sbin/dmidecode")
        .args(args)
        .stdin(Stdio::null())
        .output()?;

    if !output.status.success() {
        return Err(utils::translate_command_error(&output));
    }
    Ok(String::from_utf8_lossy(&output.stdout).into_owned())
}

pub fn system_info() -> IoResult<String> {
    dmidecode_dispatch(&["-q", "-t1"])
}

pub fn bios_info() -> IoResult<String> {
    dmidecode_dispatch(&["-q", "-t0"])
}

pub fn release_description() -> IoResult<String> {
    for l in fs::read_to_string("/etc/lsb-release")?.lines() {
        if l.starts_with("CHROMEOS_RELEASE_DESCRIPTION") {
            return Ok(l.to_string());
        }
    }
    Err(std::io::ErrorKind::NotFound.into())
}
