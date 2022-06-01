extern crate cc;

fn main() {
    // pkg_config is needed only to pick up the include path for log.c to use.
    // libflashrom-sys tells cargo how to link to libflashrom.
    let flashrom = pkg_config::Config::new()
        .cargo_metadata(false)
        .probe("flashrom")
        .unwrap();
    let mut log_c = cc::Build::new();
    log_c.file("src/log.c");
    for p in flashrom.include_paths {
        log_c.include(p);
    }
    log_c.compile("log.o");
    println!("cargo:rerun-if-changed=src/log.c");
}
