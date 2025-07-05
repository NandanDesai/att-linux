#!/bin/bash
set -euo pipefail

export CC="ccache gcc"
export KCACHE="ccache"

BUILD_DIR=".debugbuild"


# Start from a defconfig
make O=$BUILD_DIR defconfig

# Toggle desired config symbols
scripts/config --file $BUILD_DIR/.config --enable DEBUG_INFO
scripts/config --file $BUILD_DIR/.config --enable DEBUG_INFO_DWARF4    # better DWARF for GDB
scripts/config --file $BUILD_DIR/.config --disable DEBUG_INFO_REDUCED  # strip less
scripts/config --file $BUILD_DIR/.config --enable COMPAT
scripts/config --file $BUILD_DIR/.config --enable BINFMT_ELF
scripts/config --file $BUILD_DIR/.config --enable BINFMT_SCRIPT
scripts/config --file $BUILD_DIR/.config --enable BINFMT_MISC
scripts/config --file $BUILD_DIR/.config --enable KALLSYMS

# Refresh .config with defaults for any new symbols
make O=$BUILD_DIR olddefconfig

# 6) Build
make O=$BUILD_DIR -j"$(nproc)" vmlinux bzImage
