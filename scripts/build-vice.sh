#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
vice_root="$root/src/vice"
vice_src="$vice_root/vice"

if [ ! -d "$vice_src" ]; then
    echo "VICE submodule is missing; run: git submodule update --init src/vice" >&2
    exit 1
fi

cd "$vice_root"
for patch_file in "$root"/src/patches/*.diff; do
    patch -N -p1 < "$patch_file" || true
done

cd "$vice_src"
if [ ! -x ./configure ]; then
    ./autogen.sh
fi

./configure --enable-embedded EMUTYPE=xpet --with-sdl2
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
