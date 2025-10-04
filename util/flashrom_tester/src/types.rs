//
// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
// SPDX-FileCopyrightText: 2019, Google Inc. All rights reserved.
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
