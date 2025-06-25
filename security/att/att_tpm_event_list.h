#ifndef ATT_TPM_EVENTS_H
#define ATT_TPM_EVENTS_H

/*
 * tpm_event_list.c
 * Dynamic list management for struct tpm_app_event in kernel space
 */

 #include <linux/kernel.h>      /* printk, pr_* */
 #include <linux/slab.h>        /* kmalloc, kzalloc, krealloc, kfree */
 #include <linux/mutex.h>       /* mutex functions */
 #include <linux/string.h>      /* strlcpy, memset, memmove */
 #include <linux/limits.h>      /* PATH_MAX */
 #include <linux/types.h>       /* u8, u32, bool, pid_t */
 #include <linux/errno.h>       /* error codes */
 #include <linux/crypto.h>      /* crypto_alloc_shash(), crypto_free_shash() */ 
 #include <crypto/hash.h>       /* struct shash_desc and crypto_shash_*() APIs */
 #include <crypto/sha2.h>       /* SHA256_DIGEST_SIZE and core SHA-256 definitions */
 #include <linux/sched.h>       /* for pid_t definition */
 #include <linux/string.h>      /* For strlcpy function */
 
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
 
     u32   missing_sig_count;              /* # of libs without any signature */
     struct shared_lib_info *missing_sig_libs; /* array[missing_sig_count] */
 
     u32   failed_sig_count;               /* # of libs whose sig check failed */
     struct shared_lib_info *failed_sig_libs;  /* array[failed_sig_count] */
 };
 
 /* Top-level TPM event: app + ancestors + text events */
 struct tpm_app_event {
     struct elf_binary_info app_info;      /* the primary application */
 
     u32    ancestor_count;                /* how many ancestors */
     struct elf_binary_info *ancestor_info;/* array[ancestor_count] */
 
     u32    app_events_count;              /* how many descriptive strings */
     char  **app_events;                   /* array of C-string pointers */
 };
 
 
 int add_tpm_app_event(const char *full_path, pid_t pid);
 
 struct tpm_app_event *get_tpm_app_event_by_pid(pid_t pid);
 
 int delete_tpm_app_event_by_pid(pid_t pid);
 
 void print_tpm_event_list(void);

#endif // ATT_TPM_TREE_H
