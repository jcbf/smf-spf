/*
 * test_string_utils.c - Unit tests for string utilities
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "string_utils.h"

/* Test strscpy function */
START_TEST(test_strscpy_normal)
{
    char dst[20];
    strscpy(dst, "hello", sizeof(dst));
    ck_assert_str_eq(dst, "hello");
}
END_TEST

START_TEST(test_strscpy_truncation)
{
    char dst[5];
    strscpy(dst, "hello world", sizeof(dst));
    ck_assert_str_eq(dst, "hell");
    ck_assert_int_eq(strlen(dst), 4);
}
END_TEST

START_TEST(test_strscpy_empty_source)
{
    char dst[20];
    strcpy(dst, "existing");
    strscpy(dst, "", sizeof(dst));
    ck_assert_str_eq(dst, "");
}
END_TEST

START_TEST(test_strscpy_null_dst)
{
    /* Should not crash */
    strscpy(NULL, "hello", 10);
}
END_TEST

START_TEST(test_strscpy_null_src)
{
    char dst[20];
    strcpy(dst, "existing");
    strscpy(dst, NULL, sizeof(dst));
    ck_assert_str_eq(dst, "");
}
END_TEST

START_TEST(test_strscpy_zero_size)
{
    char dst[20];
    strcpy(dst, "existing");
    strscpy(dst, "hello", 0);
    /* dst should be unchanged when size is 0 */
    ck_assert_str_eq(dst, "existing");
}
END_TEST

/* Test strtolower function */
START_TEST(test_strtolower_uppercase)
{
    char str[] = "HELLO";
    strtolower(str);
    ck_assert_str_eq(str, "hello");
}
END_TEST

START_TEST(test_strtolower_mixed_case)
{
    char str[] = "HeLLo WoRLd";
    strtolower(str);
    ck_assert_str_eq(str, "hello world");
}
END_TEST

START_TEST(test_strtolower_already_lowercase)
{
    char str[] = "hello";
    strtolower(str);
    ck_assert_str_eq(str, "hello");
}
END_TEST

START_TEST(test_strtolower_with_numbers)
{
    char str[] = "Test123";
    strtolower(str);
    ck_assert_str_eq(str, "test123");
}
END_TEST

START_TEST(test_strtolower_empty)
{
    char str[] = "";
    strtolower(str);
    ck_assert_str_eq(str, "");
}
END_TEST

START_TEST(test_strtolower_null)
{
    /* Should not crash */
    strtolower(NULL);
}
END_TEST

/* Test trim_space function */
START_TEST(test_trim_space_leading)
{
    char str[] = "   hello";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "hello");
}
END_TEST

START_TEST(test_trim_space_trailing)
{
    char str[] = "hello   ";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "hello");
}
END_TEST

START_TEST(test_trim_space_both)
{
    char str[] = "  hello world  ";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "hello world");
}
END_TEST

START_TEST(test_trim_space_tabs_newlines)
{
    char str[] = "\t\nhello\n\t";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "hello");
}
END_TEST

START_TEST(test_trim_space_all_spaces)
{
    char str[] = "    ";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "");
}
END_TEST

START_TEST(test_trim_space_empty)
{
    char str[] = "";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "");
}
END_TEST

START_TEST(test_trim_space_no_trim_needed)
{
    char str[] = "hello";
    char *result = trim_space(str);
    ck_assert_str_eq(result, "hello");
}
END_TEST

START_TEST(test_trim_space_null)
{
    char *result = trim_space(NULL);
    ck_assert_ptr_null(result);
}
END_TEST

/* Create test suite */
Suite *string_utils_suite(void)
{
    Suite *s = suite_create("String Utils");

    TCase *tc_strscpy = tcase_create("strscpy");
    tcase_add_test(tc_strscpy, test_strscpy_normal);
    tcase_add_test(tc_strscpy, test_strscpy_truncation);
    tcase_add_test(tc_strscpy, test_strscpy_empty_source);
    tcase_add_test(tc_strscpy, test_strscpy_null_dst);
    tcase_add_test(tc_strscpy, test_strscpy_null_src);
    tcase_add_test(tc_strscpy, test_strscpy_zero_size);
    suite_add_tcase(s, tc_strscpy);

    TCase *tc_strtolower = tcase_create("strtolower");
    tcase_add_test(tc_strtolower, test_strtolower_uppercase);
    tcase_add_test(tc_strtolower, test_strtolower_mixed_case);
    tcase_add_test(tc_strtolower, test_strtolower_already_lowercase);
    tcase_add_test(tc_strtolower, test_strtolower_with_numbers);
    tcase_add_test(tc_strtolower, test_strtolower_empty);
    tcase_add_test(tc_strtolower, test_strtolower_null);
    suite_add_tcase(s, tc_strtolower);

    TCase *tc_trim_space = tcase_create("trim_space");
    tcase_add_test(tc_trim_space, test_trim_space_leading);
    tcase_add_test(tc_trim_space, test_trim_space_trailing);
    tcase_add_test(tc_trim_space, test_trim_space_both);
    tcase_add_test(tc_trim_space, test_trim_space_tabs_newlines);
    tcase_add_test(tc_trim_space, test_trim_space_all_spaces);
    tcase_add_test(tc_trim_space, test_trim_space_empty);
    tcase_add_test(tc_trim_space, test_trim_space_no_trim_needed);
    tcase_add_test(tc_trim_space, test_trim_space_null);
    suite_add_tcase(s, tc_trim_space);

    return s;
}
