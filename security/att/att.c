// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/att.h>


static __init int att_lsm_init(void)
{
    pr_info("att_lsm: Hello from the ATT LSM!\n");
    return 0;
}

DEFINE_LSM(att) = {
    .name = "att",
    .init = att_lsm_init,
};
