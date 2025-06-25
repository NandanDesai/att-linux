/*
 * tpm_event_list.c
 * Dynamic list management for struct tpm_app_event in kernel space
 */

#include "att_tpm_event_list.h"

/*Prototypes of internal/private functions*/
// static struct shared_lib_info *alloc_shared_lib_info(const char *path);
// static struct elf_binary_info *alloc_elf_binary_info(const char *path);

 /* Global dynamic list of TPM events */
 static struct tpm_app_event *tpm_events;
 static size_t tpm_events_count;       /* count of entries currently in use */
 static size_t tpm_events_capacity;    /* total slots allocated in tpm_events */
 static DEFINE_MUTEX(tpm_events_mutex);
 
 /**
  * add_tpm_app_event - Add a new TPM app event to the global list.
  * @full_path: Path of the executable to initialize app_info.full_path.
  * @pid:       Process ID to associate with this event.
  *
  * Allocates or grows the list, zeroes new slot, sets pid+path.
  * Checks return codes of memory and string ops.
  * Return: 0 on success, negative errno on failure.
  */
 int add_tpm_app_event(const char *full_path, pid_t pid)
 {
     struct tpm_app_event *tmp;
     size_t new_capacity;
     int ret = 0;
 
     mutex_lock(&tpm_events_mutex);
 
     if (!tpm_events) {
         new_capacity = 4;
         tpm_events = kmalloc_array(new_capacity,
                                    sizeof(*tpm_events),
                                    GFP_KERNEL);
         if (!tpm_events) {
             ret = -ENOMEM;
             goto out;
         }
         tpm_events_capacity = new_capacity;
         tpm_events_count = 0;
     } else if (tpm_events_count == tpm_events_capacity) {
         /* resize needed: double capacity for amortized O(1) appends */
         new_capacity = tpm_events_capacity * 2;
         tmp = krealloc(tpm_events,
                       new_capacity * sizeof(*tpm_events),
                       GFP_KERNEL);
         if (!tmp) {
             ret = -ENOMEM;
             goto out;
         }
         tpm_events = tmp;
         tpm_events_capacity = new_capacity;
     }
 
     /* initialize new event */
     struct tpm_app_event *event = &tpm_events[tpm_events_count];
     memset(event, 0, sizeof(*event));
     event->app_info.pid = pid;
 
     /* copy up to PATH_MAX-1 bytes */
     strncpy(event->app_info.full_path, full_path, PATH_MAX - 1);
     /* ensure NUL termination */
     event->app_info.full_path[PATH_MAX - 1] = '\0';
 
     tpm_events_count++;
 
 out:
     mutex_unlock(&tpm_events_mutex);
     return ret;
 }
 
 /**
  * get_tpm_app_event_by_pid - Look up a TPM event by process ID.
  * @pid: process ID to search for
  * Return: pointer to event or NULL if not found
  */
 struct tpm_app_event *get_tpm_app_event_by_pid(pid_t pid)
 {
     struct tpm_app_event *found = NULL;
     size_t i;
 
     mutex_lock(&tpm_events_mutex);
     for (i = 0; i < tpm_events_count; i++) {
         if (tpm_events[i].app_info.pid == pid) {
             found = &tpm_events[i];
             break;
         }
     }
     mutex_unlock(&tpm_events_mutex);
     return found;
 }
 
 /**
  * delete_tpm_app_event_by_pid - Remove and free a TPM event by PID.
  * @pid: process ID to remove
  * Return: 0 on success, -ENOENT if not found
  */
 int delete_tpm_app_event_by_pid(pid_t pid)
 {
     size_t i;
 
     mutex_lock(&tpm_events_mutex);
 
     /* find the index of the matching event */
     for (i = 0; i < tpm_events_count; i++) {
         if (tpm_events[i].app_info.pid == pid)
             break;
     }
 
     if (i == tpm_events_count) {
         mutex_unlock(&tpm_events_mutex);
         return -ENOENT; /* not found */
     }
 
     /* cleanup any allocated sub-structures */
     struct tpm_app_event *evt = &tpm_events[i];
     kfree(evt->app_info.missing_sig_libs);
     kfree(evt->app_info.failed_sig_libs);
     kfree(evt->ancestor_info);
     if (evt->app_events) {
         size_t j;
         for (j = 0; j < evt->app_events_count; j++)
             kfree(evt->app_events[j]);
         kfree(evt->app_events);
     }
 
     /* shift remaining entries down */
     if (i < tpm_events_count - 1) {
         memmove(&tpm_events[i],
                 &tpm_events[i + 1],
                 (tpm_events_count - i - 1) * sizeof(*tpm_events));
     }
     tpm_events_count--;
 
     mutex_unlock(&tpm_events_mutex);
     return 0;
 }
 
 /**
  * alloc_shared_lib_info - Allocate and initialize shared_lib_info with path.
  * Checks for allocation failure and path truncation.
  */
//  static struct shared_lib_info *alloc_shared_lib_info(const char *path)
//  {
//      struct shared_lib_info *info;
//      size_t copied;
 
//      info = kmalloc(sizeof(*info), GFP_KERNEL);
//      if (!info) {
//          pr_err("tpm_event_list: failed to alloc shared_lib_info\n");
//          return NULL;
//      }
 
//      copied = strlcpy(info->path, path, PATH_MAX);
//      if (copied >= PATH_MAX)
//          pr_warn("tpm_event_list: shared_lib path truncated\n");
//      memset(info->hash, 0, SHA256_DIGEST_SIZE);
//      return info;
//  }
 
 /**
  * alloc_elf_binary_info - Allocate and initialize elf_binary_info with path.
  * Checks for allocation failure and path truncation.
  */
//  static struct elf_binary_info *alloc_elf_binary_info(const char *path)
//  {
//      struct elf_binary_info *info;
//      size_t copied;
 
//      info = kmalloc(sizeof(*info), GFP_KERNEL);
//      if (!info) {
//          pr_err("tpm_event_list: failed to alloc elf_binary_info\n");
//          return NULL;
//      }
 
//      memset(info, 0, sizeof(*info));
//      copied = strlcpy(info->full_path, path, PATH_MAX);
//      if (copied >= PATH_MAX)
//          pr_warn("tpm_event_list: elf_binary path truncated\n");
//      return info;
//  }

 /**
 * print_tpm_event_list - Log all entries' PID and executable path
 * TODO: Remove this. This is only for debugging.
 */
void print_tpm_event_list(void)
{
    size_t i;

    mutex_lock(&tpm_events_mutex);
    for (i = 0; i < tpm_events_count; i++) {
        pr_info("att_tpm_event[%zu]: pid=%d, path=%s\n",
                i,
                tpm_events[i].app_info.pid,
                tpm_events[i].app_info.full_path);
    }
    mutex_unlock(&tpm_events_mutex);
}

 