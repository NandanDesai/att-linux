#!/bin/bash

qemu-system-x86_64 \
  -kernel arch/x86/boot/bzImage \
  -initrd initramfs.cpio.gz \
  -append "console=ttyS0 nokaslr" \
  -nographic \
  -m 1024 \
  -s -S  # Start with GDB stub
