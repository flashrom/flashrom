with import <nixpkgs> {};

stdenv.mkDerivation {
	name = "flashrom";

	buildInputs = [
		cmocka
		libftdi1
		libjaylink
		libusb1
		meson
		ninja
		pciutils
		pkg-config
	];
}
