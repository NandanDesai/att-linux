#include "att_tree.h"

static struct proc_tree_node *root_node = NULL;
static DEFINE_SPINLOCK(tree_lock);

static struct proc_tree_node *alloc_proc_node(pid_t pid, const char *filename)
{
    struct proc_tree_node *node;

    node = kmalloc(sizeof(*node), GFP_KERNEL);
    if (!node)
        return NULL;

    node->pid = pid;
    strscpy(node->filename, filename, FILENAME_MAX_LEN);
    node->parent = NULL;

    INIT_LIST_HEAD(&node->children);
    INIT_LIST_HEAD(&node->sibling_node);

    return node;
}

struct search_stack_entry {
    struct proc_tree_node *node;
    struct list_head *next_child;
};

struct proc_tree_node *find_node_iterative(pid_t target_pid)
{
    struct search_stack_entry *stack;
    int top = -1;
    struct proc_tree_node *found = NULL;

    stack = kmalloc_array(MAX_SEARCH_DEPTH, sizeof(*stack), GFP_KERNEL);
    if (!stack)
        return NULL;

    spin_lock(&tree_lock);

    if (!root_node)
        goto out_unlock;

    top++;
    stack[top].node = root_node;
    stack[top].next_child = root_node->children.next;

    while (top >= 0) {
        struct proc_tree_node *current_node = stack[top].node;
        struct list_head *pos = stack[top].next_child;

        if (current_node->pid == target_pid) {
            found = current_node;
            break;
        }

        // If we've traversed all children, pop
        if (pos == &current_node->children) {
            top--;
            continue;
        }

        // Advance to next child
        stack[top].next_child = pos->next;

        // Push the child
        if (top + 1 >= MAX_SEARCH_DEPTH)
            goto out_unlock;  // Too deep — prevent overflow

        top++;
        stack[top].node = list_entry(pos, struct proc_tree_node, sibling_node);
        stack[top].next_child = stack[top].node->children.next;
    }

out_unlock:
    spin_unlock(&tree_lock);
    kfree(stack);
    return found;
}

int add_process_to_tree(pid_t pid, pid_t ppid, const char *filename)
{
    struct proc_tree_node *new_node = NULL;
    struct proc_tree_node *parent = NULL;

    if (!filename)
        return -EINVAL;

    new_node = alloc_proc_node(pid, filename);
    if (!new_node)
        return -ENOMEM;

    spin_lock(&tree_lock);

    if (!root_node) {
        root_node = new_node;
        spin_unlock(&tree_lock);
        return 0;
    }

    spin_unlock(&tree_lock);

    parent = find_node_iterative(ppid);
    if (!parent) {
        pr_warn("Parent PID %d not found for new PID %d\n", ppid, pid);
        kfree(new_node);
        return -ENOENT;
    }

    spin_lock(&tree_lock);
    new_node->parent = parent;
    list_add_tail(&new_node->sibling_node, &parent->children);
    spin_unlock(&tree_lock);

    return 0;
}

/**
 * get_ancestry - Fills in the ancestry of a process
 * @pid: PID of the target process
 * @ancestors: Pre-allocated array of pid_t to fill in
 * @max_depth: Size of the ancestors array
 * 
 * Return: number of ancestors found (0 or more), or -errno
 */
int get_ancestry(pid_t pid, pid_t *ancestors, size_t max_depth)
{
    struct proc_tree_node *node;
    size_t depth = 0;

    if (!ancestors || max_depth == 0)
        return -EINVAL;

    node = find_node_iterative(pid);
    if (!node)
        return -ENOENT;

    spin_lock(&tree_lock);

    node = node->parent; // Start with parent
    while (node && (depth < max_depth)) {
        ancestors[depth++] = node->pid;
        node = node->parent;
    }

    spin_unlock(&tree_lock);

    // If we hit max_depth but there are more ancestors, you could return -ENOSPC
    return depth;
}

