// security/att/kunit/test_generic_list.c

#include <kunit/test.h>
#include "../data_structs/generic_list.h"

/* Test that init rejects NULL list or zero elem_size */
static void generic_list_init_failures(struct kunit *test)
{
    int ret;

    ret = generic_list_init(NULL, sizeof(int));
    KUNIT_EXPECT_EQ(test, ret, -EINVAL);

    /* zero element size */
    {
        struct generic_list list;
        ret = generic_list_init(&list, 0);
        KUNIT_EXPECT_EQ(test, ret, -EINVAL);
    }
}

/* Test that init sets up a clean list */
static void generic_list_init_success(struct kunit *test)
{
    struct generic_list list;
    int ret;

    ret = generic_list_init(&list, sizeof(int));
    KUNIT_ASSERT_EQ(test, ret, 0);

    KUNIT_EXPECT_PTR_EQ(test, list.data, NULL);
    KUNIT_EXPECT_EQ(test, list.elem_size, sizeof(int));
    KUNIT_EXPECT_EQ(test, list.length, 0);
    KUNIT_EXPECT_EQ(test, list.capacity, 0);
}

/* Test append error conditions */
static void generic_list_append_failures(struct kunit *test)
{
    int x = 5;
    struct generic_list list;

    /* NULL list */
    KUNIT_EXPECT_EQ(test,
        generic_list_append(NULL, &x),
        -EINVAL);

    /* NULL element pointer */
    generic_list_init(&list, sizeof(int));
    KUNIT_EXPECT_EQ(test,
        generic_list_append(&list, NULL),
        -EINVAL);

    /* zero elem_size */
    list.elem_size = 0;
    KUNIT_EXPECT_EQ(test,
        generic_list_append(&list, &x),
        -EINVAL);
}

/* Test a simple append sequence and get() */
static void generic_list_append_and_get(struct kunit *test)
{
    struct generic_list list;
    int vals[] = { 10, 20, 30, 40, 50 };
    int ret, i;
    int *p;

    ret = generic_list_init(&list, sizeof(*vals));
    KUNIT_ASSERT_EQ(test, ret, 0);

    /* append a few values */
    for (i = 0; i < ARRAY_SIZE(vals); i++) {
        ret = generic_list_append(&list, &vals[i]);
        KUNIT_EXPECT_EQ(test, ret, 0);
        /* after first append, capacity should be >= 4 */
        if (i == 0)
            KUNIT_EXPECT_GE(test, list.capacity, 4);
    }

    KUNIT_EXPECT_EQ(test, list.length, ARRAY_SIZE(vals));

    /* verify data via get() */
    for (i = 0; i < ARRAY_SIZE(vals); i++) {
        p = generic_list_get(&list, i);
        KUNIT_ASSERT_NOT_NULL(test, p);
        KUNIT_EXPECT_EQ(test, *p, vals[i]);
    }

    /* out-of-bounds get should return NULL */
    KUNIT_EXPECT_NULL(test,
        generic_list_get(&list, list.length));

    generic_list_free(&list);
    KUNIT_EXPECT_EQ(test, list.length, 0);
    KUNIT_EXPECT_EQ(test, list.capacity, 0);
    KUNIT_EXPECT_PTR_EQ(test, list.data, NULL);
}

static struct kunit_case generic_list_test_cases[] = {
    KUNIT_CASE(generic_list_init_failures),
    KUNIT_CASE(generic_list_init_success),
    KUNIT_CASE(generic_list_append_failures),
    KUNIT_CASE(generic_list_append_and_get),
    {}
};

static struct kunit_suite generic_list_test_suite = {
    .name = "att-generic-list",
    .test_cases = generic_list_test_cases,
};

kunit_test_suite(generic_list_test_suite);
