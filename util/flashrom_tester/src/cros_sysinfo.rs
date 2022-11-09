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
