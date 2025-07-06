#include "includes/generic_list.h"


int generic_list_init(struct generic_list *l, size_t elem_size)
{
    // Check for invalid arguments.
    if (!l || elem_size == 0)
        return -EINVAL;

    // Initialize all struct members.
    spin_lock_init(&l->lock);
    l->data       = NULL;
    l->free_flags = NULL;
    l->elem_size  = elem_size;
    l->length     = 0;
    l->capacity   = 0;
    return 0;
}

void generic_list_free(struct generic_list *l)
{
    if (!l)
        return;

    // Free allocated memory and reset struct members to prevent use-after-free.
    kfree(l->data);
    kfree(l->free_flags);
    l->data       = NULL;
    l->free_flags = NULL;
    l->length     = 0;
    l->capacity   = 0;
}

static inline int __generic_list_grow(struct generic_list *l, size_t new_cap)
{
    // Initialize pointers to NULL to prevent use of uninitialized values.
    u8   *new_flags = NULL;
    void *new_data  = NULL;

    if (new_cap <= l->capacity)
        return 0;
    // Check for potential integer overflow before calculating allocation size.
    if (new_cap > SIZE_MAX / l->elem_size)
        return -EOVERFLOW;

    // GFP_ATOMIC is used because this function is called with a spinlock held.
    new_flags = kmalloc(new_cap * sizeof(u8), GFP_ATOMIC);
    if (!new_flags)
        return -ENOMEM;

    new_data = kmalloc(new_cap * l->elem_size, GFP_ATOMIC);
    if (!new_data) {
        kfree(new_flags); // Clean up partially allocated resources on failure.
        return -ENOMEM;
    }

    // Copy existing data to the new, larger buffers.
    if (l->data)
        memcpy(new_data, l->data, l->capacity * l->elem_size);
    if (l->free_flags)
        memcpy(new_flags, l->free_flags, l->capacity * sizeof(u8));

    // Mark the newly allocated slots as free (1 = free).
    memset(new_flags + l->capacity, 1,
           (new_cap - l->capacity) * sizeof(u8));

    // Free the old, smaller buffers.
    kfree(l->data);
    kfree(l->free_flags);

    // Point the list to the new buffers and update capacity.
    l->data       = new_data;
    l->free_flags = new_flags;
    l->capacity   = new_cap;
    return 0;
}

static inline int __generic_list_shrink(struct generic_list *l)
{
    // Initialize variables to safe default values.
    size_t highest = 0;
    size_t i = 0;
    size_t new_cap = 0;
    u8   *new_flags = NULL;
    void *new_data  = NULL;

    if (!l || l->capacity == 0)
        return 0;

    // Find the last occupied slot to determine the minimum required capacity.
    for (i = l->capacity; i > 0; i--) {
        if (!l->free_flags[i - 1]) { // 0 = not free
            highest = i;
            break;
        }
    }
    // If the list is completely empty, free all memory.
    if (highest == 0) {
        kfree(l->data);
        kfree(l->free_flags);
        l->data = NULL;
        l->free_flags = NULL;
        l->capacity = 0;
        l->length = 0;
        return 0;
    }

    // Determine the new capacity, with a minimum of 4 to avoid frequent reallocations.
    new_cap = (highest < 4) ? 4 : highest;
    if (new_cap == l->capacity)
        return 0; // No shrink necessary.

    // GFP_ATOMIC is used because this function is called with a spinlock held.
    new_flags = kmalloc(new_cap * sizeof(u8), GFP_ATOMIC);
    if (!new_flags)
        return -ENOMEM;

    new_data = kmalloc(new_cap * l->elem_size, GFP_ATOMIC);
    if (!new_data) {
        kfree(new_flags);
        return -ENOMEM;
    }

    // Copy the used portion of the old data to the new, smaller buffers.
    memcpy(new_data, l->data, new_cap * l->elem_size);
    memcpy(new_flags, l->free_flags, new_cap * sizeof(u8));

    // Free the old, larger buffers.
    kfree(l->data);
    kfree(l->free_flags);

    // Update the list to point to the new buffers.
    l->data       = new_data;
    l->free_flags = new_flags;
    l->capacity   = new_cap;
    return 0;
}

int generic_list_add(struct generic_list *l, const void *elem_ptr)
{
    // Initialize all local variables.
    size_t idx = 0;
    size_t old_cap = 0;
    size_t new_cap = 0;
    int    ret = 0;

    if (!l || !elem_ptr)
        return -EINVAL;

    spin_lock(&l->lock);

    // Find the first available free slot.
    while (idx < l->capacity && !l->free_flags[idx])
        idx++;

    // If no free slot is found, the list needs to grow.
    if (idx == l->capacity) {
        old_cap = l->capacity;
        // Check for integer overflow before doubling capacity.
        if (old_cap > SIZE_MAX / 2) {
            spin_unlock(&l->lock);
            return -EOVERFLOW;
        }
        // Double the capacity, or start with 4 if it's the first element.
        new_cap = old_cap ? old_cap * 2 : 4;
        ret = __generic_list_grow(l, new_cap);
        if (ret) {
            spin_unlock(&l->lock);
            return ret; // Propagate error from grow function.
        }
        // The new element will go in the first new slot.
        idx = old_cap;
    }

    // Copy the user's element into the list.
    memcpy((u8 *)l->data + idx * l->elem_size, elem_ptr, l->elem_size);
    l->free_flags[idx] = 0; // Mark slot as not free.
    l->length++;

    spin_unlock(&l->lock);
    return 0; // Return success. Index is not returned.
}

int generic_list_delete(struct generic_list *l, size_t index)
{
    int ret = 0;
    if (!l)
        return -EINVAL;

    spin_lock(&l->lock);
    // Validate index and ensure the element at the index is not already free.
    if (index >= l->capacity || l->free_flags[index]) {
        spin_unlock(&l->lock);
        return -EINVAL;
    }

    // Mark the slot as free.
    memset((u8 *)l->data + index * l->elem_size, 0, l->elem_size); // zero out data.
    l->free_flags[index] = 1; // 1 = free.
    l->length--;

    // If list usage is low (less than 25%), shrink the capacity to save memory.
    if (l->capacity > 4 && l->length < l->capacity / 4){
        ret = __generic_list_shrink(l);
    }

    spin_unlock(&l->lock);
    return ret;
}

int generic_list_get_copy(struct generic_list *l,
                                        size_t index,
                                        void *out_buf)
{
    int ret = 0;
    if (!l || !out_buf)
        return -EINVAL;

    spin_lock(&l->lock);
    // Validate index and ensure the element exists.
    if (index >= l->capacity || l->free_flags[index]) {
        ret = -EINVAL;
    } else {
        // Safely copy the element data to the user-provided buffer.
        memcpy(out_buf, (u8 *)l->data + index * l->elem_size, l->elem_size);
    }
    spin_unlock(&l->lock);
    return ret;
}


/*
CAUTION: PLEASE TRY TO AVOID THIS FUNCTION. 
I'm keeping this here to show why `generic_list_get_copy` is necessary.
*/
void *generic_list_get(struct generic_list *l, size_t index)
{
    // Initialize pointer to NULL. This is the value returned on failure.
    void *ptr = NULL;

    if (!l)
        return NULL;

    spin_lock(&l->lock);
    // Validate index and check if the element exists.
    if (index < l->capacity && !l->free_flags[index])
        ptr = (u8 *)l->data + index * l->elem_size;
    spin_unlock(&l->lock);
    
    // WARNING: The returned pointer is valid only as long as the list is not
    // modified. Any add/delete operation could invalidate this pointer.
    // The caller must not use this pointer after another thread might have
    // modified the list.
    return ptr;
}
