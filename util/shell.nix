with import <nixpkgs> {};

stdenv.mkDerivation {
	name = "flashrom";

	buildInputs = [
		cmocka
		libftdi1
#		libjaylink	# Will be added in NixOS 21.11
		libusb1
		meson
		ninja
		pciutils
		pkg-config
	];
}
