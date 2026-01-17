/*
 * test_memory.c - Unit tests for memory utilities
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

/* Test safe_calloc function */
START_TEST(test_safe_calloc_normal)
{
    void *ptr = safe_calloc(10, sizeof(int));
    ck_assert_ptr_nonnull(ptr);

    /* Verify zeroed out */
    int *arr = (int *)ptr;
    for (int i = 0; i < 10; i++) {
        ck_assert_int_eq(arr[i], 0);
    }

    free(ptr);
}
END_TEST

START_TEST(test_safe_calloc_large)
{
    void *ptr = safe_calloc(1000, sizeof(char));
    ck_assert_ptr_nonnull(ptr);

    char *arr = (char *)ptr;
    for (int i = 0; i < 1000; i++) {
        ck_assert_int_eq(arr[i], 0);
    }

    free(ptr);
}
END_TEST

START_TEST(test_safe_calloc_zero_count)
{
    void *ptr = safe_calloc(0, sizeof(int));
    /* May return NULL or valid pointer, implementation dependent */
    if (ptr) {
        free(ptr);
    }
}
END_TEST

START_TEST(test_safe_calloc_zero_size)
{
    void *ptr = safe_calloc(10, 0);
    /* May return NULL or valid pointer, implementation dependent */
    if (ptr) {
        free(ptr);
    }
}
END_TEST

/* Test safe_strdup function */
START_TEST(test_safe_strdup_normal)
{
    const char *original = "hello world";
    char *dup = safe_strdup(original);

    ck_assert_ptr_nonnull(dup);
    ck_assert_str_eq(dup, original);
    ck_assert_ptr_ne(dup, original);

    free(dup);
}
END_TEST

START_TEST(test_safe_strdup_empty)
{
    char *dup = safe_strdup("");

    ck_assert_ptr_nonnull(dup);
    ck_assert_str_eq(dup, "");

    free(dup);
}
END_TEST

START_TEST(test_safe_strdup_long_string)
{
    const char *original = "This is a much longer string with several words and special chars: @#$%";
    char *dup = safe_strdup(original);

    ck_assert_ptr_nonnull(dup);
    ck_assert_str_eq(dup, original);
    ck_assert_int_eq(strlen(dup), strlen(original));

    free(dup);
}
END_TEST

START_TEST(test_safe_strdup_null)
{
    char *dup = safe_strdup(NULL);
    ck_assert_ptr_null(dup);
}
END_TEST

START_TEST(test_safe_strdup_with_nulls)
{
    /* Test strdup doesn't include anything after \0 */
    const char *original = "hello";
    char *dup = safe_strdup(original);

    ck_assert_int_eq(strlen(dup), 5);
    ck_assert_str_eq(dup, "hello");

    free(dup);
}
END_TEST

/* Test SAFE_FREE macro */
START_TEST(test_safe_free_normal)
{
    void *ptr = malloc(100);
    ck_assert_ptr_nonnull(ptr);

    SAFE_FREE(ptr);
    ck_assert_ptr_null(ptr);
}
END_TEST

START_TEST(test_safe_free_null)
{
    void *ptr = NULL;
    SAFE_FREE(ptr);
    ck_assert_ptr_null(ptr);
}
END_TEST

START_TEST(test_safe_free_double_free_safe)
{
    void *ptr = malloc(100);

    SAFE_FREE(ptr);
    ck_assert_ptr_null(ptr);

    /* Second free is safe due to NULL check */
    SAFE_FREE(ptr);
    ck_assert_ptr_null(ptr);
}
END_TEST

/* Create test suite */
Suite *memory_suite(void)
{
    Suite *s = suite_create("Memory Utils");

    TCase *tc_safe_calloc = tcase_create("safe_calloc");
    tcase_add_test(tc_safe_calloc, test_safe_calloc_normal);
    tcase_add_test(tc_safe_calloc, test_safe_calloc_large);
    tcase_add_test(tc_safe_calloc, test_safe_calloc_zero_count);
    tcase_add_test(tc_safe_calloc, test_safe_calloc_zero_size);
    suite_add_tcase(s, tc_safe_calloc);

    TCase *tc_safe_strdup = tcase_create("safe_strdup");
    tcase_add_test(tc_safe_strdup, test_safe_strdup_normal);
    tcase_add_test(tc_safe_strdup, test_safe_strdup_empty);
    tcase_add_test(tc_safe_strdup, test_safe_strdup_long_string);
    tcase_add_test(tc_safe_strdup, test_safe_strdup_null);
    tcase_add_test(tc_safe_strdup, test_safe_strdup_with_nulls);
    suite_add_tcase(s, tc_safe_strdup);

    TCase *tc_safe_free = tcase_create("SAFE_FREE");
    tcase_add_test(tc_safe_free, test_safe_free_normal);
    tcase_add_test(tc_safe_free, test_safe_free_null);
    tcase_add_test(tc_safe_free, test_safe_free_double_free_safe);
    suite_add_tcase(s, tc_safe_free);

    return s;
}
