#include <kunit/test.h>
#include "../data_structs/includes/generic_list.h"

/*
 * KUnit test suite for generic_list_get_copy function.
 */

/*
 * Test case: NULL list pointer or NULL output buffer should return -EINVAL
 */
static void test_generic_list_get_copy_null_args(struct kunit *test)
{
    int buffer;

    /* NULL list pointer */
    KUNIT_EXPECT_EQ(test, generic_list_get_copy(NULL, 0, &buffer), -EINVAL);

    /* NULL output buffer */
    struct generic_list l = {0};
    KUNIT_ASSERT_EQ(test, generic_list_init(&l, sizeof(int)), 0);
    KUNIT_EXPECT_EQ(test, generic_list_get_copy(&l, 0, NULL), -EINVAL);
    generic_list_free(&l);
}

/*
 * Test case: index out of range or free slot should return -EINVAL
 */
static void test_generic_list_get_copy_invalid_index(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, buffer;

    /* Initialize list and add one element */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);
    int val = 55;
    KUNIT_ASSERT_EQ(test, generic_list_add(&l, &val), 0);

    /* Index >= capacity */
    KUNIT_EXPECT_EQ(test, generic_list_get_copy(&l, l.capacity, &buffer), -EINVAL);

    /* Free the only slot and then attempt get_copy */
    KUNIT_ASSERT_EQ(test, generic_list_delete(&l, 0), 0);
    KUNIT_EXPECT_EQ(test, generic_list_get_copy(&l, 0, &buffer), -EINVAL);

    generic_list_free(&l);
}

/*
 * Test case: get_copy returns correct value for valid index
 */
static void test_generic_list_get_copy_basic(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, out;

    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);

    /* Add an element and verify get_copy */
    int value = 1234;
    KUNIT_ASSERT_EQ(test, generic_list_add(&l, &value), 0);
    KUNIT_EXPECT_EQ(test, l.length, 1);

    ret = generic_list_get_copy(&l, 0, &out);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, out, value);

    generic_list_free(&l);
}

/*
 * Test case: stress get_copy—add and then randomly verify values
 */
static void test_generic_list_get_copy_stress(struct kunit *test)
{
    struct generic_list l = {0};
    int ret, out;
    const size_t num = 1024;

    /* Initialize and add elements */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);
    for (size_t i = 0; i < num; i++) {
        int v = (int)i * 2;
        ret = generic_list_add(&l, &v);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "add failed at %zu", i);
    }
    KUNIT_EXPECT_EQ(test, l.length, num);

    /* Spot-check some indices */
    size_t indices[] = {0, num/2, num-1};
    for (int j = 0; j < ARRAY_SIZE(indices); j++) {
        size_t idx = indices[j];
        ret = generic_list_get_copy(&l, idx, &out);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "get_copy failed at %zu", idx);
        KUNIT_EXPECT_EQ(test, out, (int)idx * 2);
    }

    generic_list_free(&l);
}

static struct kunit_case generic_list_get_copy_test_cases[] = {
    KUNIT_CASE(test_generic_list_get_copy_null_args),
    KUNIT_CASE(test_generic_list_get_copy_invalid_index),
    KUNIT_CASE(test_generic_list_get_copy_basic),
    KUNIT_CASE(test_generic_list_get_copy_stress),
    {}
};

static struct kunit_suite generic_list_get_copy_test_suite = {
    .name = "generic_list_get_copy_test",
    .test_cases = generic_list_get_copy_test_cases,
};

kunit_test_suite(generic_list_get_copy_test_suite);
