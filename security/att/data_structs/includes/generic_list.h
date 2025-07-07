#ifndef _GENERIC_LIST_H_
#define _GENERIC_LIST_H_

#include <linux/slab.h>      /* kmalloc, kfree */
#include <linux/types.h>     /* size_t */
#include <linux/string.h>    /* memcpy, memset */
#include <linux/errno.h>     /* -EINVAL, -ENOMEM, -EOVERFLOW */
#include <linux/spinlock.h>  /* spinlock_t, spin_lock_init, spin_lock, spin_unlock */

/*
 * Unordered list with fixed-size elements and slot reuse.
 * Thread-safe via spinlock; supports automatic grow/shrink.
 */

struct generic_list {
    void       *data;        /* element storage */
    u8         *free_flags;  /* 1=free, 0=occupied */
    size_t     elem_size;    /* size of each element */
    size_t     length;       /* occupied slots count */
    size_t     capacity;     /* total slots */
    spinlock_t lock;         /* protects list */
};

/**
 * generic_list_init – Initialize a generic_list structure.
 * @l:           Pointer to an uninitialized list struct.
 * @elem_size:   Size in bytes of each element (>0).
 *
 * Returns 0 on success or -EINVAL if arguments are invalid.
 */
int generic_list_init(struct generic_list *l, size_t elem_size);

/**
 * generic_list_free – Release all memory used by the list.
 * @l:  Pointer to a list previously initialized by generic_list_init.
 *
 * Frees internal buffers and resets list fields to zero.
 */
void generic_list_free(struct generic_list *l);

/**
 * generic_list_add – Add an element, reusing a free slot if available.
 * @l:         Pointer to an initialized list.
 * @elem_ptr: Pointer to the element data to copy into the list.
 *
 * Returns 0 on success, -ENOMEM if allocation fails, or -EOVERFLOW
 * if the list grows beyond allowable size.
 */
int generic_list_add(struct generic_list *l, const void *elem_ptr);

/**
 * generic_list_delete – Remove an element, zeroing its data.
 * @l:      Pointer to an initialized list.
 * @index:  Index of the slot to delete (0 <= index < capacity).
 *
 * Returns 0 on success or -EINVAL if index is out of range or already free.
 * The slot’s memory is cleared before marking free to prevent data leakage.
 */
int generic_list_delete(struct generic_list *l, size_t index);

/**
 * generic_list_get_copy – Copy an element out of the list.
 * @l:        Pointer to an initialized list.
 * @index:    Index of the slot to read (0 <= index < capacity).
 * @out_buf:  Caller-provided buffer of at least elem_size bytes.
 *
 * Returns 0 on success or -EINVAL if index is invalid or slot is free.
 * The element at @index is copied into @out_buf under lock.
 */
int generic_list_get_copy(struct generic_list *l,
                          size_t index,
                          void *out_buf);

/**
 * generic_list_get – Direct pointer access to an element.
 * @l:      Pointer to an initialized list.
 * @index:  Index of the slot to read (0 <= index < capacity).
 *
 * Returns a pointer to the element or NULL if invalid or free.
 * WARNING: The returned pointer is only valid while the internal
 * spinlock remains held; callers must not use after release.
 * 
 * CAUTION: Use this function only in the same thread where add and delete
 * functions are being called. For cross-thread GET, use `generic_list_get_copy`.
 * This function cannot be "safe" even if it uses locks. 
 */
void *generic_list_get(struct generic_list *l, size_t index);

#endif /* _GENERIC_LIST_H_ */
