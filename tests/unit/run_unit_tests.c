/*
 * run_unit_tests.c - Main test runner for unit tests
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>

/* Declare test suite functions from other test files */
extern Suite *string_utils_suite(void);
extern Suite *ip_utils_suite(void);
extern Suite *memory_suite(void);
extern Suite *logging_suite(void);
extern Suite *config_suite(void);

int main(void)
{
    SRunner *sr;
    int number_failed = 0;

    /* Create test runner */
    sr = srunner_create(string_utils_suite());

    /* Add additional test suites */
    srunner_add_suite(sr, ip_utils_suite());
    srunner_add_suite(sr, memory_suite());
    srunner_add_suite(sr, logging_suite());
    srunner_add_suite(sr, config_suite());

    /* Run the tests */
    srunner_run_all(sr, CK_VERBOSE);

    /* Count failed tests */
    number_failed = srunner_ntests_failed(sr);

    /* Clean up */
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
