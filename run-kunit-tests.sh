#!/bin/bash

# Comma-separated list of KUnit tests to run
KUNIT_TESTS="generic_list_add_test,generic_list_delete_test,generic_list_get_copy_test"

qemu-system-x86_64 \
  -nodefaults \
  -m 1024 \
  -kernel .kunit/arch/x86/boot/bzImage \
  -append "kunit.run=${KUNIT_TESTS} kunit.enable=1 console=ttyS0 kunit_shutdown=reboot" \
  -no-reboot \
  -nographic \
  -serial stdio
