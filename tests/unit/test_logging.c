/*
 * test_logging.c - Unit tests for logging utilities
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "logging.h"

/* Test log_init and log_shutdown */
START_TEST(test_log_init_and_shutdown)
{
    /* Should initialize without crashing */
    log_init("test-daemon", SYSLOG_DISABLE, NULL, "testhost");
    log_shutdown();
}
END_TEST

START_TEST(test_log_init_with_file)
{
    FILE *logfile = tmpfile();
    ck_assert_ptr_nonnull(logfile);

    log_init("test-daemon", SYSLOG_DISABLE, logfile, "testhost");

    /* Log a message */
    log_message(LOG_INFO, "Test message");

    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_set_file)
{
    FILE *logfile = tmpfile();
    ck_assert_ptr_nonnull(logfile);

    log_init("test-daemon", SYSLOG_DISABLE, NULL, "testhost");
    log_set_file(logfile);

    /* Log a message */
    log_message(LOG_INFO, "Test message");

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_message_formatting)
{
    FILE *logfile = tmpfile();
    ck_assert_ptr_nonnull(logfile);

    log_init("test-daemon", SYSLOG_DISABLE, logfile, "testhost");

    log_message(LOG_INFO, "Test message with %s and %d", "string", 123);

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_multiple_levels)
{
    FILE *logfile = tmpfile();
    ck_assert_ptr_nonnull(logfile);

    log_init("test-daemon", SYSLOG_DISABLE, logfile, "testhost");

    log_message(LOG_EMERG, "Emergency message");
    log_message(LOG_ALERT, "Alert message");
    log_message(LOG_CRIT, "Critical message");
    log_message(LOG_ERR, "Error message");
    log_message(LOG_WARNING, "Warning message");
    log_message(LOG_NOTICE, "Notice message");
    log_message(LOG_INFO, "Info message");
    log_message(LOG_DEBUG, "Debug message");

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_init_syslog_disable)
{
    log_init("test-daemon", SYSLOG_DISABLE, NULL, "testhost");
    log_message(LOG_INFO, "Message with syslog disabled");
    log_shutdown();
}
END_TEST

START_TEST(test_log_message_without_init)
{
    /* Should handle gracefully */
    log_shutdown();
    log_message(LOG_INFO, "Message after shutdown");
}
END_TEST

START_TEST(test_log_empty_message)
{
    FILE *logfile = tmpfile();
    log_init("test-daemon", SYSLOG_DISABLE, logfile, "testhost");

    log_message(LOG_INFO, "");

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_long_message)
{
    FILE *logfile = tmpfile();
    log_init("test-daemon", SYSLOG_DISABLE, logfile, "testhost");

    /* Long message with multiple parameters */
    log_message(LOG_INFO,
                "This is a long message with multiple parameters: %s, %d, %s, %d",
                "first", 1, "second", 2);

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_null_hostname)
{
    FILE *logfile = tmpfile();
    log_init("test-daemon", SYSLOG_DISABLE, logfile, NULL);

    log_message(LOG_INFO, "Message with NULL hostname");

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

START_TEST(test_log_null_daemon_name)
{
    FILE *logfile = tmpfile();
    log_init(NULL, SYSLOG_DISABLE, logfile, "testhost");

    log_message(LOG_INFO, "Message with NULL daemon name");

    log_set_file(NULL);
    log_shutdown();
    fclose(logfile);
}
END_TEST

/* Create test suite */
Suite *logging_suite(void)
{
    Suite *s = suite_create("Logging Utils");
    TCase *tc_core = tcase_create("logging");

    tcase_add_test(tc_core, test_log_init_and_shutdown);
    tcase_add_test(tc_core, test_log_init_with_file);
    tcase_add_test(tc_core, test_log_set_file);
    tcase_add_test(tc_core, test_log_message_formatting);
    tcase_add_test(tc_core, test_log_multiple_levels);
    tcase_add_test(tc_core, test_log_init_syslog_disable);
    tcase_add_test(tc_core, test_log_message_without_init);
    tcase_add_test(tc_core, test_log_empty_message);
    tcase_add_test(tc_core, test_log_long_message);
    tcase_add_test(tc_core, test_log_null_hostname);
    tcase_add_test(tc_core, test_log_null_daemon_name);

    suite_add_tcase(s, tc_core);
    return s;
}
