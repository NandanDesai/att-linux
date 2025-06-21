#ifndef ATT_TREE_H
#define ATT_TREE_H
#include <linux/kernel.h>      // For printk, KERN_* log levels, common kernel macros
#include <linux/slab.h>        // For kmalloc, kfree, kmalloc_array
#include <linux/string.h>      // For strscpy (safe string copy)
#include <linux/spinlock.h>    // For spinlock_t and spin_lock/spin_unlock
#include <linux/sched.h>       // For current, task_struct, etc. (only needed if interacting with current task)
#include <linux/pid.h>         // For pid_t
#include <linux/list.h>        // For struct list_head and list manipulation macros

#define FILENAME_MAX_LEN 4096 // Max path length in ext4 file system
#define MAX_SEARCH_DEPTH 64  // Arbitrary safety limit to avoid infinite loops

struct proc_tree_node {
    pid_t pid;
    char filename[FILENAME_MAX_LEN];

    struct proc_tree_node *parent;

    struct list_head children;
    struct list_head sibling_node;
};

int add_process_to_tree(pid_t pid, pid_t ppid, const char *filename);
int get_ancestry(pid_t pid, pid_t *ancestors, size_t max_depth);
struct proc_tree_node *find_node_iterative(pid_t target_pid);

#endif // ATT_TREE_H
