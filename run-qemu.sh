#!/bin/bash

TPMDIR="$HOME/mytpm" && mkdir -p "$TPMDIR" && swtpm socket --tpm2 --ctrl type=unixio,path="$TPMDIR/swtpm-sock" --log level=20 --tpmstate dir="$TPMDIR" --daemon

qemu-system-x86_64 \
  -kernel arch/x86/boot/bzImage \
  -initrd initramfs.cpio.gz \
  -append "console=ttyS0 nokaslr" \
  -chardev socket,id=chrtpm,path=$TPMDIR/swtpm-sock \
  -tpmdev emulator,id=tpm0,chardev=chrtpm \
  -device tpm-tis,tpmdev=tpm0 \
  -nographic \
  -m 1024 \
  -s -S  # Start with GDB stub
