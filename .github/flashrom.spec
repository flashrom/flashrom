Name:		flashrom
#Update this version string and create a matching v%{version} tag to cut a release 
Version:	1.4.1
Release:	0%{?dist}
Summary:	Simple program for reading/writing flash chips content
License:	GPLv2
URL:		https://flashrom.org

Source0:	https://github.com/metal-toolbox/%{name}/archive/refs/tags/v%{version}.tar.gz

BuildRequires:	gcc
BuildRequires:	pciutils-devel
BuildRequires:	libusb-devel
# Used for new programmers (libusb0 will eventually be removed)
BuildRequires:	libusbx-devel
BuildRequires:	systemd
BuildRequires:	systemd-devel
BuildRequires:	zlib-devel
%ifarch %{ix86} x86_64 aarch64
BuildRequires:	dmidecode
Requires:	dmidecode
%endif
Requires:	udev

%description
flashrom is a utility for identifying, reading, writing, verifying and erasing
flash chips. It is designed to flash BIOS/EFI/coreboot/firmware/optionROM
images on mainboards, network/graphics/storage controller cards, and various
other programmer devices.

%prep
%autosetup -p1 -n %{name}-%{version}

%build
CFLAGS=-g %{__make} PREFIX=/usr CONFIG_INTERNAL=yes %{?_smp_mflags}

%install
%{__make} PREFIX=/usr DESTDIR=$RPM_BUILD_ROOT install

%files
%license COPYING
%{_sbindir}/%{name}
/usr/share/bash-completion/completions/flashrom.bash

%changelog
