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

use flashrom_tester::types;
use std::io::Write;
use std::path::PathBuf;
use std::sync::Mutex;

struct Logger<W: Write + Send> {
    level: log::LevelFilter,
    target: LogTarget<W>,
    color: types::Color,
}

enum LogTarget<W>
where
    W: Write,
{
    Terminal,
    Write(Mutex<W>),
}

impl<W: Write + Send> log::Log for Logger<W> {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        metadata.level() <= self.level
    }

    fn log(&self, record: &log::Record) {
        fn log_internal<W: Write>(
            mut w: W,
            record: &log::Record,
            color: &types::Color,
        ) -> std::io::Result<()> {
            let now = chrono::Utc::now().to_rfc3339_opts(chrono::SecondsFormat::Micros, true);
            write!(w, "{}{} ", color.magenta, now)?;
            write!(w, "{}[ {} ]{} ", color.yellow, record.level(), color.reset)?;
            writeln!(w, "{}", record.args())
        }

        // Write errors deliberately ignored
        let _ = match self.target {
            LogTarget::Terminal => {
                let stdout = std::io::stdout();
                let mut lock = stdout.lock();
                log_internal(&mut lock, record, &self.color)
            }
            LogTarget::Write(ref mutex) => {
                let mut lock = mutex.lock().unwrap();
                log_internal(&mut *lock, record, &self.color)
            }
        };
    }

    fn flush(&self) {
        // Flush errors deliberately ignored
        let _ = match self.target {
            LogTarget::Terminal => std::io::stdout().flush(),
            LogTarget::Write(ref w) => w.lock().unwrap().flush(),
        };
    }
}

pub fn init(to_file: Option<PathBuf>, debug: bool) {
    let mut logger = Logger {
        level: log::LevelFilter::Info,
        target: LogTarget::Terminal,
        color: if atty::is(atty::Stream::Stdout) {
            types::COLOR
        } else {
            types::NOCOLOR
        },
    };

    if debug {
        logger.level = log::LevelFilter::Debug;
    }
    if let Some(path) = to_file {
        logger.target = LogTarget::Write(Mutex::new(
            std::fs::File::create(path).expect("Unable to open log file for writing"),
        ));
        logger.color = types::NOCOLOR;
    }

    log::set_max_level(logger.level);
    log::set_boxed_logger(Box::new(logger)).unwrap();
}

#[cfg(test)]
mod tests {
    use super::{LogTarget, Logger};
    use flashrom_tester::types;
    use log::{Level, LevelFilter, Log, Record};
    use std::sync::Mutex;

    fn run_records(records: &[Record]) -> String {
        let mut buf = Vec::<u8>::new();
        {
            let lock = Mutex::new(&mut buf);
            let logger = Logger {
                level: LevelFilter::Info,
                target: LogTarget::Write(lock),
                color: types::COLOR,
            };

            for record in records {
                if logger.enabled(record.metadata()) {
                    logger.log(record);
                }
            }
        }
        String::from_utf8(buf).unwrap()
    }

    /// Log messages have the expected format
    #[test]
    fn format() {
        let buf = run_records(&[Record::builder()
            .args(format_args!("Test message at INFO"))
            .level(Level::Info)
            .build()]);

        assert_eq!(&buf[..5], "\x1b[35m");
        // Time is difficult to test, assume it's formatted okay
        assert_eq!(
            &buf[24..],
            " \x1b[33m[ INFO ]\x1b[0m Test message at INFO\n"
        );
    }

    #[test]
    fn level_filter() {
        let buf = run_records(&[
            Record::builder()
                .args(format_args!("Test message at DEBUG"))
                .level(Level::Debug)
                .build(),
            Record::builder()
                .args(format_args!("Hello, world!"))
                .level(Level::Error)
                .build(),
        ]);

        // There is one line because the Debug record wasn't written.
        println!("{}", buf);
        assert_eq!(buf.lines().count(), 1);
    }
}
