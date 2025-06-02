/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ATT_H
#define _LINUX_ATT_H

#include <linux/types.h>

/*
 * Example exported function.
 */
static inline void att_hello_world(void)
{
    pr_info("att_lsm: Hello from att_hello_world()\n");
}

#endif /* _LINUX_ATT_H */
