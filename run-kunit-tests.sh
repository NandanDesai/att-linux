#!/bin/bash

qemu-system-x86_64 \
  -nodefaults \
  -m 1024 \
  -kernel .kunit/arch/x86/boot/bzImage \
  -append 'kunit.run=att-generic-list kunit.enable=1 console=ttyS0 kunit_shutdown=reboot' \
  -no-reboot \
  -nographic \
  -serial stdio 
