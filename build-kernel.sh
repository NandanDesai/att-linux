#!/bin/bash
set -e

export CC="ccache gcc"
export KCACHE=ccache

make defconfig

scripts/config --enable CONFIG_DEBUG_INFO=y
scripts/config --enable CONFIG_DEBUG_INFO_DWARF4  # Optional: better DWARF support for modern GDB
scripts/config --enable CONFIG_DEBUG_INFO_REDUCED=n
scripts/config --enable CONFIG_COMPAT=y
scripts/config --enable CONFIG_BINFMT_ELF=y
scripts/config --enable CONFIG_BINFMT_SCRIPT=y
scripts/config --enable CONFIG_BINFMT_MISC=y
scripts/config --enable CONFIG_KALLSYMS=y

# Rebuild with full debug info and no stripping
make -j$(nproc) vmlinux bzImage
