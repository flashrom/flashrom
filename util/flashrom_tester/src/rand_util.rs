//
// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
// SPDX-FileCopyrightText: 2019, Google Inc. All rights reserved.
//

use std::fs::File;
use std::io::prelude::*;
use std::io::BufWriter;
use std::path::Path;

use rand::prelude::*;

pub fn gen_rand_testdata(path: &Path, size: usize) -> std::io::Result<()> {
    let mut buf = BufWriter::new(File::create(path)?);

    let mut a: Vec<u8> = vec![0; size];
    thread_rng().fill(a.as_mut_slice());

    buf.write_all(a.as_slice())?;

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn gen_rand_testdata() {
        use super::gen_rand_testdata;

        let path0 = Path::new("/tmp/idk_test00");
        let path1 = Path::new("/tmp/idk_test01");
        let sz = 1024;

        gen_rand_testdata(path0, sz).unwrap();
        gen_rand_testdata(path1, sz).unwrap();

        let mut buf0 = Vec::new();
        let mut buf1 = Vec::new();

        let mut f = File::open(path0).unwrap();
        let mut g = File::open(path1).unwrap();

        f.read_to_end(&mut buf0).unwrap();
        g.read_to_end(&mut buf1).unwrap();

        assert_ne!(buf0, buf1);
    }
}
