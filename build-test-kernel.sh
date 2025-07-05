#!/bin/bash
set -euo pipefail

BUILD_DIR=".kunit"
ARCH="x86_64"
JOBS="$(nproc)"

export CC="ccache gcc"
export KCACHE="ccache"

# Create a fresh defconfig under $BUILD_DIR
make O=$BUILD_DIR defconfig

# Enable KUnit and your module’s test-tristate
scripts/config --file .kunit/.config --enable KUNIT
scripts/config --file .kunit/.config --enable KUNIT_SANITY
scripts/config --file .kunit/.config --enable KUNIT_REPORT
scripts/config --file .kunit/.config --enable ATT_KUNIT_TEST

# Fold in defaults and resolve dependencies
make O=$BUILD_DIR olddefconfig

# Build your QEMU bzImage (and modules) with ATT_KUNIT_TEST=m
make O=$BUILD_DIR -j"$JOBS" bzImage
