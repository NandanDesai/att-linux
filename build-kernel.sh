#!/bin/bash
set -euo pipefail

export CC="ccache gcc"
export KCACHE="ccache"

# Start from a defconfig
make defconfig

# Toggle desired config symbols
scripts/config --enable DEBUG_INFO
scripts/config --enable DEBUG_INFO_DWARF4    # better DWARF for GDB
scripts/config --disable DEBUG_INFO_REDUCED  # strip less
scripts/config --enable COMPAT
scripts/config --enable BINFMT_ELF
scripts/config --enable BINFMT_SCRIPT
scripts/config --enable BINFMT_MISC
scripts/config --enable KALLSYMS

# Refresh .config with defaults for any new symbols
make olddefconfig

# 6) Build
make -j"$(nproc)" vmlinux bzImage
