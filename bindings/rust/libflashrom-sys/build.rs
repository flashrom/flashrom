fn main() {
    pkg_config::probe_library("flashrom").unwrap();
    let bindings = bindgen::Builder::default()
        .header("../../../include/libflashrom.h")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        // only generate the flashrom functions and used types.
        .allowlist_function("flashrom_.*")
        // newtype enums provide type checking without the UB potential of rust enums.
        .default_enum_style(bindgen::EnumVariation::NewType { is_bitfield: false })
        // We use constified enum for flashrom_log_level to allow '<' comparison.
        .constified_enum("flashrom_log_level")
        .prepend_enum_name(false)
        .derive_copy(false)
        .must_use_type("flashrom_wp_result")
        // Avoid some va_list related functionality that is not easy to use in rust.
        .blocklist_function("flashrom_set_log_callback")
        .blocklist_type("flashrom_log_callback")
        .blocklist_type("va_list")
        .blocklist_type("__builtin_va_list")
        .blocklist_type("__va_list_tag")
        .size_t_is_usize(true)
        .generate()
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
