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
