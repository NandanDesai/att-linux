// generic_list.h
/*
USAGE EXAMPLE:

    struct generic_list ints;
    int             x = 42, y = 7;
    int            *p;

    generic_list_init(&ints, sizeof(int));
    if (generic_list_append(&ints, &x))
        return -ENOMEM;
    generic_list_append(&ints, &y);

    p = generic_list_get(&ints, 1);
    pr_info("second element = %d\n", p ? *p : -1);

    generic_list_free(&ints);

    // IMPORTANT: If x and y were allocated through kmalloc, then kfree responsibility is on the caller.
    return 0;
*/
/*
NOTE: Functions in this file are NOT thread-safe.
*/
#ifndef _GENERIC_LIST_H_
#define _GENERIC_LIST_H_

#include <linux/slab.h>    /* kmalloc, kfree, krealloc */
#include <linux/types.h>   /* size_t */
#include <linux/string.h>  /* memcpy */
#include <linux/errno.h>   /* -EINVAL, -ENOMEM, -EOVERFLOW */

/*
 * struct g_list – a generic, fixed-element-size list
 *
 * @data:      contiguous storage for elements
 * @elem_size: sizeof each element (must be > 0)
 * @length:    number of elements currently stored
 * @capacity:  number of elements that can be stored without growing
 */
struct generic_list {
    void   *data;
    size_t  elem_size;
    size_t  length;
    size_t  capacity;
};

/**
 * generic_list_init() – initialize a list
 * @l:         pointer to a g_list struct
 * @elem_size: sizeof each element (in bytes), must be > 0
 *
 * Returns 0 on success, -EINVAL if @l is NULL or elem_size is zero.
 */
static inline int generic_list_init(struct generic_list *l, size_t elem_size)
{
    if (l == NULL || elem_size == 0)
        return -EINVAL;

    l->data      = NULL;
    l->elem_size = elem_size;
    l->length    = 0;
    l->capacity  = 0;
    return 0;
}

/**
 * generic_list_append() – append one element by copying its bytes
 * @l:        the list
 * @elem_ptr: pointer to the element to copy (must be non-NULL)
 *
 * Returns  0 on success
 *        -EINVAL     if @l or @elem_ptr is NULL, or elem_size is zero
 *        -ENOMEM     if memory allocation fails
 *        -EOVERFLOW  if capacity*elem_size would overflow size_t
 */
static inline int generic_list_append(struct generic_list *l, const void *elem_ptr)
{
    size_t new_cap;
    void  *new_data;

    if (l == NULL || elem_ptr == NULL || l->elem_size == 0)
        return -EINVAL;

    /* grow if full */
    if (l->length == l->capacity) {
        new_cap = (l->capacity == 0) ? 4 : l->capacity * 2;

        /* overflow check */
        if (new_cap > SIZE_MAX / l->elem_size)
            return -EOVERFLOW;

        new_data = krealloc(l->data,
                            new_cap * l->elem_size,
                            GFP_KERNEL);
        if (new_data == NULL)
            return -ENOMEM;

        l->data     = new_data;
        l->capacity = new_cap;
    }

    /* copy the element in */
    memcpy((u8*)l->data + l->length * l->elem_size,
           elem_ptr,
           l->elem_size);
    l->length++;
    return 0;
}

/**
 * generic_list_get() – get a pointer to the element at index
 * @l:     the list
 * @index: zero-based index
 *
 * Returns pointer to element data, or NULL if @l is NULL or index >= length.
 */
static inline void *generic_list_get(struct generic_list *l, size_t index)
{
    if (l == NULL || index >= l->length)
        return NULL;
    return (char *)l->data + index * l->elem_size;
}

/**
 * generic_list_free() – free all internal storage
 * @l: the list
 *
 * Safe to call on NULL or on a list already freed.
 */
static inline void generic_list_free(struct generic_list *l)
{
    if (l == NULL)
        return;

    kfree(l->data);
    l->data     = NULL;
    l->length   = 0;
    l->capacity = 0;
}


/* TODO: REMOVE THIS FUNCTION LATER */
static inline void generic_list_stress_init(void)
{
    struct generic_list list;
    int ret, i;
    int *val;

    /* 1) init */
    ret = generic_list_init(&list, sizeof(int));
    if (ret) {
        pr_err("generic_list_init failed: %d\n", ret);
    }

    /* 2) append 1..1000 */
    for (i = 1; i <= 1000; i++) {
        ret = generic_list_append(&list, &i);
        if (ret) {
            pr_err("append %d failed: %d\n", i, ret);
            generic_list_free(&list);
        }
    }
    pr_info("Appended %zu elements\n", list.length);

    /* 3) display them */
    for (i = 0; i < list.length; i++) {
        val = generic_list_get(&list, i);
        if (val)
            pr_info(" list[%d] = %d\n", i, *val);
        else
            pr_warn(" list[%d] = <NULL>\n", i);
    }

    /* 4) free entire list */
    generic_list_free(&list);

    /* 5) confirm deletion */
    pr_info("After free: length = %zu, data ptr = %p\n",
            list.length, list.data);
}


#endif /* _GENERIC_LIST_H_ */
