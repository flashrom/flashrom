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

pub struct Color {
    pub bold: &'static str,
    pub reset: &'static str,
    pub magenta: &'static str,
    pub yellow: &'static str,
    pub green: &'static str,
    pub red: &'static str,
}

pub const COLOR: Color = Color {
    bold: "\x1b[1m",
    reset: "\x1b[0m",
    magenta: "\x1b[35m",
    yellow: "\x1b[33m",
    green: "\x1b[92m",
    red: "\x1b[31m",
};

pub const NOCOLOR: Color = Color {
    bold: "",
    reset: "",
    magenta: "",
    yellow: "",
    green: "",
    red: "",
};

macro_rules! style_dbg {
    ($s: expr, $c: expr, $col: expr) => {
        format!("{}{:?}{}", $c, $s, $col.reset)
    };
}
macro_rules! style {
    ($s: expr, $c: expr, $col: expr) => {
        format!("{}{}{}", $c, $s, $col.reset)
    };
}
