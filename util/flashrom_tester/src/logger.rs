//
// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
// SPDX-FileCopyrightText: 2019, Google Inc. All rights reserved.
//

use flashrom_tester::types;
use std::io::Write;

struct Logger {
    level: log::LevelFilter,
    color: types::Color,
}

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        metadata.level() <= self.level
    }

    fn log(&self, record: &log::Record) {
        // Write errors deliberately ignored
        let stdout = std::io::stdout();
        let mut lock = stdout.lock();
        let now = chrono::Utc::now().to_rfc3339_opts(chrono::SecondsFormat::Micros, true);
        let _ = write!(lock, "{}{} ", self.color.magenta, now);
        let _ = write!(
            lock,
            "{}[ {} ]{} ",
            self.color.yellow,
            record.level(),
            self.color.reset
        );
        let _ = writeln!(lock, "{}", record.args());
    }

    fn flush(&self) {
        // Flush errors deliberately ignored
        let _ = std::io::stdout().flush();
    }
}

pub fn init(debug: bool) {
    let mut logger = Logger {
        level: log::LevelFilter::Info,
        color: if atty::is(atty::Stream::Stdout) {
            types::COLOR
        } else {
            types::NOCOLOR
        },
    };

    if debug {
        logger.level = log::LevelFilter::Debug;
    }
    log::set_max_level(logger.level);
    log::set_boxed_logger(Box::new(logger)).unwrap();
}

#[cfg(test)]
mod tests {
    use std::io::Read;

    use super::Logger;
    use flashrom_tester::types;
    use log::{Level, LevelFilter, Log, Record};

    fn run_records(records: &[Record]) -> String {
        let buf = gag::BufferRedirect::stdout().unwrap();
        {
            let logger = Logger {
                level: LevelFilter::Info,
                color: types::COLOR,
            };

            for record in records {
                if logger.enabled(record.metadata()) {
                    logger.log(record);
                }
            }
        }
        let mut ret = String::new();
        buf.into_inner().read_to_string(&mut ret).unwrap();
        ret
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
        // Split on the UTC timezone char
        assert_eq!(
            buf.split_once("Z ").unwrap().1,
            "\x1b[33m[ INFO ]\x1b[0m Test message at INFO\n"
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
