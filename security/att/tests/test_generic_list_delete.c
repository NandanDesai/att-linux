#include <kunit/test.h>
#include "../data_structs/includes/generic_list.h"

/*
 * KUnit test suite for generic_list_delete function.
 */

/*
 * Test case: NULL pointer for delete should return -EINVAL
 */
static void test_generic_list_delete_null(struct kunit *test)
{
    KUNIT_EXPECT_EQ(test, generic_list_delete(NULL, 0), -EINVAL);
}

/*
 * Test case: delete with invalid index (>= capacity) or on free slot returns -EINVAL
 */
static void test_generic_list_delete_invalid_index_and_free(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, val;

    /* Initialize and add one element */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);
    val = 99;
    KUNIT_ASSERT_EQ(test, generic_list_add(&l, &val), 0);

    /* Invalid index >= capacity */
    KUNIT_EXPECT_EQ(test, generic_list_delete(&l, l.capacity), -EINVAL);
    /* Delete valid index */
    KUNIT_EXPECT_EQ(test, generic_list_delete(&l, 0), 0);
    /* Deleting again the same slot (now free) */
    KUNIT_EXPECT_EQ(test, generic_list_delete(&l, 0), -EINVAL);

    generic_list_free(&l);
}

/*
 * Test case: basic delete behavior—slot is zeroed, marked free, and length decremented
 */
static void test_generic_list_delete_basic(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, val, out;

    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);
    val = 123;
    KUNIT_ASSERT_EQ(test, generic_list_add(&l, &val), 0);
    KUNIT_EXPECT_EQ(test, l.length, 1);

    /* Delete the element */
    ret = generic_list_delete(&l, 0);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, l.length, 0);
    /* Now get_copy should fail with -EINVAL */
    ret = generic_list_get_copy(&l, 0, &out);
    KUNIT_EXPECT_EQ(test, ret, -EINVAL);

    generic_list_free(&l);
}

/*
 * Test case: stress delete—add and then delete 65536 elements sequentially
 */
static void test_generic_list_delete_stress(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, value;
    const size_t num = 65536;

    /* Initialize and add elements 0..65535 */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);
    for (size_t i = 0; i < num; i++) {
        value = (int)i;
        ret = generic_list_add(&l, &value);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "add failed at %zu", i);
    }
    KUNIT_EXPECT_EQ(test, l.length, num);

    /* Delete all elements and verify length decrements */
    for (size_t i = 0; i < num; i++) {
        ret = generic_list_delete(&l, i);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "delete failed at %zu", i);
        KUNIT_EXPECT_EQ_MSG(test, l.length, num - i - 1,
            "length mismatch after deleting %zu", i);
    }
    /* After all deletes, length should be zero */
    KUNIT_EXPECT_EQ(test, l.length, 0);

    generic_list_free(&l);
}

/*
 * Test case: shrink behavior—when list usage falls below 25%, shrink capacity
 * Deletes from the end to ensure highest occupied index drops.
 */
static void test_generic_list_delete_shrink(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, value;
    const int initial = 8;

    /* Initialize and add 8 elements to reach capacity 8 */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);
    for (int i = 0; i < initial; i++) {
        value = i;
        KUNIT_ASSERT_EQ(test, generic_list_add(&l, &value), 0);
    }
    KUNIT_EXPECT_EQ(test, l.capacity, initial);

    /* Delete 7 elements from the end to drop highest occupied index */
    for (int i = initial - 1; i > 0; i--) {
        ret = generic_list_delete(&l, i);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "delete failed at index %d", i);
    }
    /* After deletes, only index 0 remains => shrink to 4 */
    KUNIT_EXPECT_EQ(test, l.capacity, 4);
    KUNIT_EXPECT_EQ(test, l.length, 1);

    generic_list_free(&l);
}

static struct kunit_case generic_list_delete_test_cases[] = {
    KUNIT_CASE(test_generic_list_delete_null),
    KUNIT_CASE(test_generic_list_delete_invalid_index_and_free),
    KUNIT_CASE(test_generic_list_delete_basic),
    KUNIT_CASE(test_generic_list_delete_stress),
    KUNIT_CASE(test_generic_list_delete_shrink),
    {}
};

static struct kunit_suite generic_list_delete_test_suite = {
    .name = "generic_list_delete_test",
    .test_cases = generic_list_delete_test_cases,
};

kunit_test_suite(generic_list_delete_test_suite);
