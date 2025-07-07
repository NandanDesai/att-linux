#include "att_app_list.h"

struct generic_list g_app_list;

/* Flag: true once att_app_list_init() has successfully run */
/* TODO: change this */
static bool g_app_list_inited = false;

int att_app_list_init(void)
{
    int ret = 0;

    if (g_app_list_inited) {
        pr_warn("att_app: g_app_list already initialized\n");
        return 0;
    }

    /* Initialize g_app_list to hold pointers to struct att_app */
    ret = generic_list_init(&g_app_list, sizeof(struct att_app));
    if (ret) {
        pr_err("att_app: g_app_list init failed (%d)\n", ret);
        return ret;
    }

    g_app_list_inited = true;
    pr_info("att_app: g_app_list initialized\n");
    return 0;
}

int att_app_create(pid_t pid, const char *bin_path)
{
    struct att_app *app      = NULL;
    size_t         path_len  = 0;
    int            ret       = 0;

    if (!bin_path) {
        pr_err("att_app_create: bin_path is NULL\n");
        return -EINVAL;
    }

    path_len = strnlen(bin_path, PATH_MAX);
    if (path_len >= PATH_MAX) {
        pr_err("att_app_create: bin_path too long (%zu >= %d)\n",
               path_len, PATH_MAX);
        return -ENAMETOOLONG;
    }

    /* Ensure global list is initialized */
    if (!g_app_list_inited) {
        pr_err("att_app_create: g_app_list not initialized\n");
        return -EINVAL;
    }

    /* Allocate zeroed att_app */
    app = kzalloc(sizeof(*app), GFP_KERNEL);
    if (!app) {
        pr_err("att_app_create: kzalloc(att_app) failed\n");
        return -ENOMEM;
    }

    /* Initialize nested lists */
    ret = generic_list_init(&app->app_info.missing_sig_libs,
                            sizeof(struct shared_lib_info));
    if (ret) {
        pr_err("att_app_create: init missing_sig_libs failed (%d)\n", ret);
        goto err_free_app;
    }

    ret = generic_list_init(&app->app_info.failed_sig_libs,
                            sizeof(struct shared_lib_info));
    if (ret) {
        pr_err("att_app_create: init failed_sig_libs failed (%d)\n", ret);
        goto err_free_missing;
    }

    ret = generic_list_init(&app->ancestors,
                            sizeof(struct elf_binary_info));
    if (ret) {
        pr_err("att_app_create: init ancestors failed (%d)\n", ret);
        goto err_free_failed;
    }

    ret = generic_list_init(&app->app_events,
                            sizeof(struct app_event));
    if (ret) {
        pr_err("att_app_create: init app_events failed (%d)\n", ret);
        goto err_free_anc;
    }

    /* Populate required fields */
    app->app_info.pid = pid;
    strlcpy(app->app_info.full_path, bin_path, PATH_MAX);

    /* hash[] zeroed by kzalloc, signature_verified defaults to false */

    /* Add to global list */
    ret = generic_list_add(&g_app_list, &app);
    if (ret) {
        pr_err("att_app_create: add to g_app_list failed (%d)\n", ret);
        goto err_free_events;
    }

    /* List holds its own copy, so free our original */
    kfree(app);

    pr_info("att_app_create: added pid=%d path=%s\n", pid, bin_path);
    return 0;

/* Error-unwind in reverse init order */
err_free_events:
    generic_list_free(&app->app_events);
err_free_anc:
    generic_list_free(&app->ancestors);
err_free_failed:
    generic_list_free(&app->app_info.failed_sig_libs);
err_free_missing:
    generic_list_free(&app->app_info.missing_sig_libs);
err_free_app:
    kfree(app);
    return ret;
}

struct att_app *att_app_find_by_pid(pid_t pid)
{
    size_t             i       = 0;
    size_t             cap     = 0;
    struct att_app    *found   = NULL;
    struct att_app    *candidate = NULL;
    u8                *slot    = NULL;

    /* Ensure global list was initialized */
    if (!g_app_list_inited)
        return NULL;

    /* Acquire lock once for whole traversal */
    spin_lock(&g_app_list.lock);
    cap = g_app_list.capacity;

    for (i = 0; i < cap; i++) {
        if (g_app_list.free_flags[i] == 0) {
            /* compute pointer to stored struct att_app* */
            slot = (u8 *)g_app_list.data
                   + (i * g_app_list.elem_size);
            candidate = *(struct att_app **)slot;

            if (candidate && candidate->app_info.pid == pid) {
                found = candidate;
                break;
            }
        }
    }

    spin_unlock(&g_app_list.lock);
    return found;
}

int att_app_add_failed_sig_libs(pid_t pid,
                                   const struct shared_lib_info *lib_info)
{
    struct att_app *app = NULL;
    int ret = 0;

    if (!lib_info) {
        pr_err("att_app_add_failed_sig_libs: lib_info is NULL\n");
        return -EINVAL;
    }

    /* Lookup the att_app by PID */
    app = att_app_find_by_pid(pid);
    if (!app) {
        pr_err("att_app_add_failed_sig_libs: no entry for pid %d\n", pid);
        return -ESRCH;
    }

    /* Append into the failed_sig_libs list */
    ret = generic_list_add(&app->app_info.failed_sig_libs, lib_info);
    if (ret) {
        pr_err("att_app_add_failed_sig_libs: list_add failed (%d)\n", ret);
        return ret;
    }

    pr_info("att_app_add_failed_sig_libs: added lib '%s' for pid %d\n",
            lib_info->path, pid);
    return 0;
}

int att_app_add_missing_sig_libs(pid_t pid,
                                 const struct shared_lib_info *lib_info)
{
    struct att_app *app      = NULL;
    int            ret       = 0;

    if (!lib_info) {
        pr_err("att_app_add_missing_sig_libs: lib_info is NULL\n");
        return -EINVAL;
    }

    app = att_app_find_by_pid(pid);
    if (!app) {
        pr_err("att_app_add_missing_sig_libs: no entry for pid %d\n", pid);
        return -ESRCH;
    }

    ret = generic_list_add(&app->app_info.missing_sig_libs, lib_info);
    if (ret) {
        pr_err("att_app_add_missing_sig_libs: list_add failed (%d)\n", ret);
        return ret;
    }

    pr_info("att_app_add_missing_sig_libs: added lib '%s' for pid %d\n",
            lib_info->path, pid);
    return 0;
}

int att_app_add_app_event(pid_t pid,
                          const struct app_event *event)
{
    struct att_app *app      = NULL;
    int            ret       = 0;

    if (!event) {
        pr_err("att_app_add_app_event: event is NULL\n");
        return -EINVAL;
    }

    app = att_app_find_by_pid(pid);
    if (!app) {
        pr_err("att_app_add_app_event: no entry for pid %d\n", pid);
        return -ESRCH;
    }

    /* Append the event to the app_events list */
    ret = generic_list_add(&app->app_events, event);
    if (ret) {
        pr_err("att_app_add_app_event: list_add failed (%d)\n", ret);
        return ret;
    }

    pr_info("att_app_add_app_event: added event type %u for pid %d\n",
            event->event_type, pid);
    return 0;
}

int att_app_add_ancestor(pid_t pid,
                         const struct elf_binary_info *ancestor)
{
    struct att_app             *app      = NULL;
    int                         ret      = 0;

    if (!ancestor) {
        pr_err("att_app_add_ancestor: ancestor is NULL\n");
        return -EINVAL;
    }

    app = att_app_find_by_pid(pid);
    if (!app) {
        pr_err("att_app_add_ancestor: no entry for pid %d\n", pid);
        return -ESRCH;
    }

    /* Append into the ancestors list */
    ret = generic_list_add(&app->ancestors, ancestor);
    if (ret) {
        pr_err("att_app_add_ancestor: list_add failed (%d)\n", ret);
        return ret;
    }

    pr_info("att_app_add_ancestor: added ancestor pid=%d path=%s\n",
            ancestor->pid, ancestor->full_path);
    return 0;
}

int att_app_delete(pid_t pid)
{
    size_t          i       = 0;
    size_t          cap     = 0;
    size_t          idx     = 0;
    struct att_app *app     = NULL;
    bool            found   = false;
    int             ret     = 0;

    /* Ensure the global list is initialized */
    if (!g_app_list_inited) {
        pr_err("att_app_delete: g_app_list not initialized\n");
        return -EINVAL;
    }

    /* Locate the entry under lock */
    spin_lock(&g_app_list.lock);
    cap = g_app_list.capacity;
    for (i = 0; i < cap; i++) {
        if (g_app_list.free_flags[i] == 0) {
            struct att_app *candidate =
                (struct att_app *)((u8 *)g_app_list.data
                                   + i * g_app_list.elem_size);
            if (candidate->app_info.pid == pid) {
                app   = candidate;
                idx   = i;
                found = true;
                break;
            }
        }
    }
    spin_unlock(&g_app_list.lock);

    if (!found) {
        pr_err("att_app_delete: no entry for pid %d\n", pid);
        return -ESRCH;
    }

    /* Free all nested lists inside the inline struct */
    generic_list_free(&app->app_events);
    generic_list_free(&app->ancestors);
    generic_list_free(&app->app_info.failed_sig_libs);
    generic_list_free(&app->app_info.missing_sig_libs);

    /* Remove the slot from the global list (clears the by-value struct) */
    ret = generic_list_delete(&g_app_list, idx);
    if (ret) {
        pr_err("att_app_delete: generic_list_delete failed for pid %d idx %zu (%d)\n",
               pid, idx, ret);
        return ret;
    }

    pr_info("att_app_delete: removed entry for pid %d (slot %zu)\n", pid, idx);
    return 0;
}
