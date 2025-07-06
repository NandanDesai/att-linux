#include <kunit/test.h>
#include "../data_structs/includes/generic_list.h"

/*
 * KUnit test suite for generic_list_add function.
 * This suite includes error-path checks, growth behavior, and stress testing.
 */

/*
 * Test case: passing NULL pointers for list or element should return -EINVAL
 */
static void test_generic_list_add_null_args(struct kunit *test)
{
    struct generic_list l = {0};
    int val = 1;

    /* NULL list pointer */
    KUNIT_EXPECT_EQ(test, generic_list_add(NULL, &val), -EINVAL);

    /* NULL element pointer */
    KUNIT_ASSERT_EQ(test, generic_list_init(&l, sizeof(int)), 0);
    KUNIT_EXPECT_EQ(test, generic_list_add(&l, NULL), -EINVAL);
    generic_list_free(&l);
}


/*
 * Test case: growth behavior—initial capacity is 4, doubles to 8 when exceeded, and values are retained
 */
static void test_generic_list_add_growth_behavior(struct kunit *test)
{
    struct generic_list l = {0};
    int ret;
    int val;
    int out;

    /* Initialize list */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);

    /* Add 4 elements to fill initial capacity */
    for (int i = 0; i < 4; i++) {
        val = i;
        ret = generic_list_add(&l, &val);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "add failed at %d", i);
    }
    /* Verify capacity remains at initial 4 */
    KUNIT_EXPECT_EQ(test, l.capacity, 4);

    /* Add one more to trigger growth to 8 */
    val = 4;
    ret = generic_list_add(&l, &val);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, l.capacity, 8);

    /* Verify the new element at index 4 */
    ret = generic_list_get_copy(&l, 4, &out);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, out, 4);

    generic_list_free(&l);
}

/*
 * Test case: stress test by adding 65536 elements, verifying length and spot-checking values
 */
static void test_generic_list_add_stress(struct kunit *test)
{
    struct generic_list l = {0};
    int ret = 0;
    const size_t num = 65536;
    int value = 0;
    int out = 0;

    /* Initialize list */
    ret = generic_list_init(&l, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);

    /* Stress test: add 65536 elements */
    for (size_t i = 0; i < num; i++) {
        value = (int)i;
        ret = generic_list_add(&l, &value);
        KUNIT_ASSERT_EQ_MSG(test, ret, 0, "generic_list_add failed at index %zu", i);
    }
    /* Verify total length */
    KUNIT_EXPECT_EQ(test, l.length, num);

    /* Spot-check first, middle, and last elements */
    ret = generic_list_get_copy(&l, 0, &out);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, out, 0);

    ret = generic_list_get_copy(&l, num / 2, &out);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, out, num / 2);

    ret = generic_list_get_copy(&l, num - 1, &out);
    KUNIT_ASSERT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, out, num - 1);

    generic_list_free(&l);
}

static struct kunit_case generic_list_add_test_cases[] = {
    KUNIT_CASE(test_generic_list_add_null_args),
    KUNIT_CASE(test_generic_list_add_growth_behavior),
    KUNIT_CASE(test_generic_list_add_stress),
    {}
};

static struct kunit_suite generic_list_add_test_suite = {
    .name = "generic_list_add_test",
    .test_cases = generic_list_add_test_cases,
};

kunit_test_suite(generic_list_add_test_suite);
