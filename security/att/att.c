// SPDX-License-Identifier: GPL-2.0
#include <linux/lsm_hooks.h>    // LSM hook infrastructure and macros
#include <linux/cred.h>         // For credentials (e.g., current_cred())
#include <linux/fs.h>           // For inode structures
#include <linux/binfmts.h>      // For linux_binprm
#include <linux/init.h>         // For __init, __ro_after_init
#include <linux/kernel.h>       // For pr_info(), printk levels
#include <linux/array_size.h>   // For ARRAY_SIZE
#include <linux/tpm.h>          // For TPM-related operations
#include <linux/kernel.h>       // For panic() function
#include <linux/init.h>         // For late_initcall function

#define ATT_MODULE_NAME "att"

// LSM ID structure
static struct lsm_id att_lsmid __ro_after_init = {
    .name = ATT_MODULE_NAME,
    .id = LSM_ID_ATT,
};

//int att_enabled_boot __initdata = 1;

/* Forward declaration of hook functions */
static int att_bprm_check(struct linux_binprm *bprm);
static int att_inode_permission(struct inode *inode, int mask);
static int __init att_init(void);

/* Hook list */
static struct security_hook_list att_hooks[] __ro_after_init = {
    LSM_HOOK_INIT(bprm_check_security, att_bprm_check),
    LSM_HOOK_INIT(inode_permission, att_inode_permission),
};

/* Hook functions */
static int att_bprm_check(struct linux_binprm *bprm)
{
    // It's good practice to check if bprm and bprm->filename are not NULL
    if (bprm && bprm->filename) {
        pr_info("att: checking bprm for executable: %s\n", bprm->filename);
    } else {
        pr_warn("att: bprm_check called with NULL bprm or filename\n");
    }
    // Return 0 to allow the operation, or an error code (e.g., -EPERM) to deny.
    return 0;
}

static int att_inode_permission(struct inode *inode, int mask)
{
    // It's good practice to check if inode is not NULL
    if (inode && inode->i_sb) { // Check inode and superblock for i_ino
        pr_info("att: inode_permission on inode %lu (dev %s) mask %x\n",
                inode->i_ino, inode->i_sb->s_id, mask);
    } else if (inode) {
        pr_info("att: inode_permission on inode (no sb/ino?) mask %x\n", mask);
    } else {
        pr_warn("att: inode_permission called with NULL inode\n");
    }
    // Return 0 to allow the operation, or an error code (e.g., -EPERM) to deny.
    // Returning 0 effectively means this hook doesn't block anything yet.
    return 0;
}

/* LSM initialization function */
static __init int att_lsm_init(void)
{
    pr_info("att_lsm: Initializing ATT LSM\n");

    /*
     * Register the hooks with the security subsystem.
     * The lsm_id provides the name used for identification.
     */
    security_add_hooks(att_hooks, ARRAY_SIZE(att_hooks), &att_lsmid);
    return 0;
}

/* Actual init call for the att module */
static int __init att_init(void){
    struct tpm_chip *att_tpm_chip = tpm_default_chip();
	if (!att_tpm_chip){
		pr_info("No TPM chip found, exiting!\n");
        panic("TPM not found. Aborting boot for security reasons.\n");
        return -1;
    }
    pr_info("TPM chip found by att module.\n");
    return 0;
}

// Define att module as an early launch module
DEFINE_EARLY_LSM(att) = {
    .name = "att",
    .init = att_lsm_init,
};


late_initcall(att_init);
