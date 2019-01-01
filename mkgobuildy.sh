#!/bin/bash
set -e

DEFAULT_ARGS="-Dwith-libressl-mode=shim -Dwith-new-libcxx-abi=true"

if [[ ! -d buildroot/build32 ]]; then
    ASFLAGS="$ASFLAGS --32" CFLAGS="$CFLAGS -m32" CXXFLAGS="$CXXFLAGS -m32" LDFLAGS="$LDFLAGS -m32" \
    meson buildroot/build32 $DEFAULT_ARGS \
        --prefix=/usr \
        --sysconfdir=/etc \
        --datadir=/usr/share \
        --libdir=/usr/lib32 \
        -Dwith-shim=none \
        --buildtype debugoptimized
fi

if [[ ! -d buildroot/build64 ]]; then
    meson buildroot/build64 $DEFAULT_ARGS \
        --prefix=/usr \
        --sysconfdir=/etc \
        --datadir=/usr/share \
        --libdir=/usr/lib64 \
        -Dwith-shim=replacement \
        -Dwith-steam-binary=/usr/lib64/steam/steam \
        -Dwith-frontend=true \
        --buildtype debugoptimized
fi

ninja -C buildroot/build32
ninja -C buildroot/build64

sudo ninja -C buildroot/build32 install
sudo ninja -C buildroot/build64 install


