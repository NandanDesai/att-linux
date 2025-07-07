#ifndef ATT_APP_LIST_H
#define ATT_APP_LIST_H

#include <linux/kernel.h>      /* printk, pr_* */
#include <linux/slab.h>        /* kmalloc, kzalloc, krealloc, kfree */
#include <linux/string.h>      /* strlcpy, memset, memmove */
#include <linux/limits.h>      /* PATH_MAX */
#include <linux/types.h>       /* u8, u32, bool, pid_t */
#include <linux/errno.h>       /* error codes */
#include <crypto/sha2.h>       /* SHA256_DIGEST_SIZE and core SHA-256 definitions */
#include <linux/sched.h>       /* for pid_t definition */
#include <linux/string.h>      /* For strlcpy function */
#include <linux/spinlock.h>     /* spin_lock_t definition and spin_lock()/spin_unlock() APIs */
#include "data_structs/includes/generic_list.h"

#define MAX_APP_EVENT_DATA_SIZE 512

/* Shared library info */
struct shared_lib_info {
    char path[PATH_MAX];                  /* full path (up to 4096) */
    u8   hash[SHA256_DIGEST_SIZE];        /* raw SHA-256 digest (32 bytes) */
};

/* Info about one ELF binary + its missing/failed-sig libs */
struct elf_binary_info {
    pid_t pid;                            /* process ID for this event */
    char  full_path[PATH_MAX];            /* executable path */
    u8    hash[SHA256_DIGEST_SIZE];       /* raw SHA-256 digest */
    bool  signature_verified;             /* signature check result */

    struct generic_list missing_sig_libs; /* array[missing_sig_count], type:  struct shared_lib_info */ 

    struct generic_list failed_sig_libs;  /* array[failed_sig_count], type:  struct shared_lib_info */
};


/* App event info */
struct app_event {
    u8 event_type;                 
    u8 event_data[MAX_APP_EVENT_DATA_SIZE];
};

/* Top-level TPM event: app + ancestors + text events */
struct att_app {
    struct elf_binary_info app_info;      /* the primary application */

    struct generic_list ancestors;    /* array[ancestor_count], type:  struct elf_binary_info */ 

    struct generic_list app_events;       /* array of live app events, type: struct app_event  */
};

// Global variable to store all the .att process info
extern struct generic_list g_app_list; /* type: struct att_app */

/**
 * att_app_list_init – Initialize the global list of att_app entries.
 *
 * This must be called once (e.g. at system boot) before any calls to
 * att_app_create().  It sets up the internal generic_list used to track
 * all att_app allocations.
 *
 * Returns:
 *   0 on success,
 *   -ENOMEM if allocation of internal buffers fails,
 *   or any negative error propagated by generic_list_init().
 */
int att_app_list_init(void);

/**
 * att_app_create – Allocate and register a new att_app for a process.
 * @pid:       PID of the process to track.
 * @bin_path:  Kernel-space, NUL-terminated full path to the process binary.
 *
 * Allocates a zeroed struct att_app, initializes its nested lists
 * (missing/failed sig libs, ancestors, events), populates the pid and
 * full_path, and appends it into the already-initialized global list.
 *
 * Returns:
 *   0 on success (entry added),
 *   -EINVAL if bin_path is NULL or global list isn’t initialized,
 *   -ENAMETOOLONG if bin_path ≥ PATH_MAX,
 *   -ENOMEM if allocation fails,
 *   or any negative error from generic_list_init()/generic_list_add().
 */
int att_app_create(pid_t pid, const char *bin_path);

/**
 * att_app_find_by_pid – find a registered att_app by PID
 * @pid:  the process ID to search for
 *
 * Returns a pointer to the matching struct att_app, or NULL if not found
 * or if the global list is not initialized.  
 * 
 * WARNING: The caller must not
 * dereference the returned pointer if any other thread might modify
 * the list (i.e. add/delete) without first re-acquiring the spinlock.
 */
struct att_app *att_app_find_by_pid(pid_t pid);

/**
 * att_app_add_failed_sig_libs – append a shared_lib_info to an app’s failed_sig_libs
 * @pid:       PID of the target application
 * @lib_info:  pointer to a filled-in shared_lib_info struct
 *
 * Returns  0 on success
 *         -EINVAL if lib_info is NULL
 *         -ESRCH  if no att_app for that pid is found
 *         -ENOMEM/-EOVERFLOW/… propagated from generic_list_add()
 */
int att_app_add_failed_sig_libs(pid_t pid,
    const struct shared_lib_info *lib_info);

/**
 * att_app_add_missing_sig_libs – append a shared_lib_info to an app’s missing_sig_libs
 * @pid:       PID of the target application
 * @lib_info:  pointer to a filled-in shared_lib_info struct
 *
 * Returns  0 on success
 *         -EINVAL if lib_info is NULL
 *         -ESRCH  if no att_app for that pid is found
 *         -ENOMEM/-EOVERFLOW/… propagated from generic_list_add()
 */
int att_app_add_missing_sig_libs(pid_t pid,
    const struct shared_lib_info *lib_info);

/**
 * att_app_add_app_event – append an app_event to an app’s app_events list
 * @pid:    PID of the target application
 * @event:  pointer to a filled-in struct app_event
 *
 * Returns  0 on success
 *         -EINVAL if event is NULL
 *         -ESRCH  if no att_app for that pid is found
 *         -ENOMEM/-EOVERFLOW/... propagated from generic_list_add()
 */
int att_app_add_app_event(pid_t pid,
    const struct app_event *event);
/**
 * att_app_add_ancestor – append an elf_binary_info to an app’s ancestors list
 * @pid:        PID of the target application
 * @ancestor:   pointer to a filled-in struct elf_binary_info
 *
 * Returns  0 on success
 *         -EINVAL if ancestor is NULL
 *         -ESRCH  if no att_app for that pid is found
 *         -ENOMEM/-EOVERFLOW/… propagated from generic_list_add()
 */
int att_app_add_ancestor(pid_t pid,
    const struct elf_binary_info *ancestor);


/**
 * att_app_delete – remove an att_app by PID from g_app_list
 * @pid:  PID of the application to delete
 *
 * Finds the matching entry in the global list, frees all its nested
 * generic_list storage, and removes the slot.  Since we store the
 * struct by-value in the list’s internal buffer, we do NOT kfree() the
 * pointer itself—generic_list_delete() will reclaim the slot.
 *
 * Returns 0 on success,
 *        -EINVAL if the global list isn’t initialized,
 *        -ESRCH  if no entry for that PID is found,
 *        or the error from generic_list_delete().
 */
int att_app_delete(pid_t pid);


#endif