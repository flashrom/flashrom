#!/usr/bin/env bash
# Package the Meson CLI build made by .github/workflows/build-binaries.yml.
set -euo pipefail

: "${PLATFORM:?Set PLATFORM to the build matrix platform}"
version=$(cat VERSION)
if [[ ${RELEASE_BUILD:-false} != true ]]; then
    version="${version}+git.$(git rev-parse --short=12 HEAD)"
fi
name="flashrom-${version}-${PLATFORM}"
stage="package/${name}"
mkdir -p "$stage" dist
cp COPYING.rst README.rst "$stage/"
git rev-parse HEAD > "$stage/COMMIT"
meson introspect --buildoptions build > "$stage/build-options.json"

case "$PLATFORM" in
    linux-*)
        # VERSION includes a leading v; prereleases must sort before releases.
        deb_version=${version#v}
        deb_version=${deb_version//-/\~}
        mkdir -p "$stage/usr/sbin" "$stage/usr/share/doc/flashrom" "$stage/DEBIAN" "$stage/debian"
        install -m 755 build/flashrom "$stage/usr/sbin/flashrom"
        mv "$stage/"*.rst "$stage/COMMIT" "$stage/build-options.json" "$stage/usr/share/doc/flashrom/"
        # dpkg-shlibdeps requires a source control file even when using -O.
        cat > "$stage/debian/control" <<'EOF'
Source: flashrom
Section: electronics
Priority: optional
Maintainer: flashrom developers <flashrom@flashrom.org>

Package: flashrom
Architecture: any
Description: Identify, read, write, verify and erase flash chips
EOF
        dependencies=$(cd "$stage" && dpkg-shlibdeps -O -eusr/sbin/flashrom)
        dependencies=${dependencies#shlibs:Depends=}
        cat > "$stage/DEBIAN/control" <<EOF
Package: flashrom
Version: ${deb_version}
Architecture: $(dpkg --print-architecture)
Section: electronics
Priority: optional
Maintainer: flashrom developers <flashrom@flashrom.org>
Depends: ${dependencies}
Homepage: https://www.flashrom.org/
Description: Identify, read, write, verify and erase flash chips
EOF
        # This file is only an input to dpkg-shlibdeps, not package content.
        rm "$stage/debian/control"
        rmdir "$stage/debian"
        "$stage/usr/sbin/flashrom" --version
        dpkg-deb --root-owner-group --build "$stage" "dist/${name}.deb"
        ;;
    macos-*)
        install -m 755 build/flashrom "$stage/flashrom"
        dylibbundler -b -cd -x "$stage/flashrom" -d "$stage/lib" -p '@executable_path/lib/'
        # Keep dependency notices and package provenance alongside the libraries.
        for formula in libusb libftdi openssl@3; do
            prefix=$(brew --prefix "$formula")
            mkdir -p "$stage/licenses/$formula"
            find "$prefix/" -maxdepth 1 -type f \
                \( -iname '*license*' -o -iname '*copying*' -o -iname '*notice*' \) \
                -exec cp '{}' "$stage/licenses/$formula/" \;
            brew info --json=v2 "$formula" > "$stage/licenses/$formula/homebrew.json"
        done
        # Reject dependencies on Homebrew or the build directory after bundling.
        for binary in "$stage/flashrom" "$stage/lib/"*.dylib; do
            # dylibbundler can rewrite several Homebrew rpaths to the same
            # path. Recent dyld versions reject these duplicate LC_RPATHs.
            rpaths=$(otool -l "$binary" | awk '
                $1 == "cmd" && $2 == "LC_RPATH" {
                    getline; getline
                    if ($2 == "@executable_path/lib/") n++
                }
                END { print n+0 }')
            while (( rpaths > 1 )); do
                install_name_tool -delete_rpath '@executable_path/lib/' "$binary"
                rpaths=$((rpaths - 1))
            done
            dependencies=$(otool -L "$binary")
            if printf '%s\n' "$dependencies" | tail -n +2 | grep -E '/(opt/homebrew|usr/local|Users)/'; then
                echo "Unbundled dependency in $binary" >&2
                exit 1
            fi
            # Bundling changes Mach-O load commands and invalidates signatures.
            # Ad-hoc signing permits execution but is not Developer ID signing
            # or notarization for Gatekeeper-approved distribution.
            codesign --force --sign - "$binary"
            codesign --verify --strict "$binary"
        done
        "$stage/flashrom" --version
        tar -czf "dist/${name}.tar.gz" -C package "$name"
        ;;
    windows-*)
        cp build/flashrom.exe "$stage/"
        # Follow PE imports recursively, copying only toolchain DLLs. Windows
        # system DLLs stay on the target OS; an unknown import is an error.
        queue=("$stage/flashrom.exe")
        index=0
        system_dir="$(cygpath -u "${SYSTEMROOT:-C:/Windows}")/System32"
        while (( index < ${#queue[@]} )); do
            binary=${queue[$index]}
            index=$((index + 1))
            imports=$(objdump -p "$binary")
            while read -r dll; do
                [[ -f "$stage/$dll" ]] && continue
                if [[ -f "$MINGW_PREFIX/bin/$dll" ]]; then
                    cp "$MINGW_PREFIX/bin/$dll" "$stage/"
                    queue+=("$stage/$dll")
                elif [[ -f "$system_dir/$dll" || "$dll" == api-ms-win-* || "$dll" == ext-ms-win-* ]]; then
                    continue
                else
                    echo "Unresolved DLL: $dll (imported by $binary)" >&2
                    exit 1
                fi
            done < <(printf '%s\n' "$imports" | tr -d '\r' | sed -n 's/^[[:space:]]*DLL Name: //p')
        done
        cp -R "$MINGW_PREFIX/share/licenses" "$stage/licenses"
        pacman -Q > "$stage/msys2-packages.txt"
        # Prevent installed MSYS2 DLLs from masking incomplete packaging.
        PATH="$system_dir" "$stage/flashrom.exe" --version
        (cd package && zip -qr "../dist/${name}.zip" "$name")
        ;;
    *)
        echo "Unknown platform: $PLATFORM" >&2
        exit 1
        ;;
esac
