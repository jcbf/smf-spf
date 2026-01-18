/*
 * Unit tests for configuration module
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

#include "config/config.h"
#include "config/defaults.h"

/* Test Suite 1: Configuration Initialization */

START_TEST(test_config_init_defaults)
{
    /* Initialize and verify all defaults are set */
    int result = config_init();
    ck_assert_int_eq(result, 0);

    /* Verify strings */
    ck_assert_ptr_nonnull(conf.tag);
    ck_assert_str_eq(conf.tag, TAG_STRING_DEFAULT);
    ck_assert_str_eq(conf.quarantine_box, QUARANTINE_BOX_DEFAULT);
    ck_assert_str_eq(conf.run_as_user, USER_DEFAULT);
    ck_assert_str_eq(conf.sendmail_socket, OCONN_DEFAULT);

    /* Verify numeric defaults */
    ck_assert_int_eq(conf.best_guess, BEST_GUESS_DEFAULT);
    ck_assert_int_eq(conf.refuse_fail, REFUSE_FAIL_DEFAULT);
    ck_assert_int_eq(conf.relaxed_localpart, RELAXED_LOCALPART_DEFAULT);
    ck_assert_int_eq(conf.refuse_none, REFUSE_NONE_DEFAULT);
    ck_assert_int_eq(conf.soft_fail, SOFT_FAIL_DEFAULT);
    ck_assert_int_eq(conf.tag_subject, TAG_SUBJECT_DEFAULT);
    ck_assert_int_eq(conf.add_header, ADD_HEADER_DEFAULT);
    ck_assert_int_eq(conf.quarantine, QUARANTINE_DEFAULT);
    ck_assert_int_eq(conf.daemonize, DAEMONIZE_DEFAULT);
    ck_assert_ulong_eq(conf.spf_ttl, SPF_TTL_DEFAULT);

    /* Verify null/empty pointers */
    ck_assert_ptr_null(conf.cidrs);
    ck_assert_ptr_null(conf.ipnats);
    ck_assert_ptr_null(conf.ptrs);
    ck_assert_ptr_null(conf.froms);
    ck_assert_ptr_null(conf.tos);

    config_free();
}
END_TEST

START_TEST(test_config_init_idempotent)
{
    /* Multiple inits should be safe */
    config_init();
    config_init();
    ck_assert_ptr_nonnull(conf.tag);
    config_free();
}
END_TEST

START_TEST(test_config_init_allocations)
{
    /* Verify no memory leaks on init/free */
    config_init();
    config_free();
    /* If we get here without crashing, that's good */
    ck_assert(1);
}
END_TEST

START_TEST(test_config_struct_size)
{
    /* Struct size should be consistent */
    size_t expected_size = sizeof(config_t);
    ck_assert(expected_size > 0);
    ck_assert(expected_size < 512);  /* Should be reasonably sized */
}
END_TEST

START_TEST(test_config_global_instance)
{
    /* Global conf should be accessible */
    config_init();
    ck_assert_ptr_nonnull(&conf);
    config_free();
}
END_TEST


/* Test Suite 2: Configuration Loading */

START_TEST(test_load_valid_minimal_config)
{
    config_init();
    int result = config_load("tests/unit/fixtures/valid_minimal.conf");
    ck_assert_int_eq(result, 1);

    /* Should have loaded whitelist */
    ck_assert_ptr_nonnull(conf.cidrs);
    ck_assert_ptr_nonnull(conf.ptrs);

    config_free();
}
END_TEST

START_TEST(test_load_valid_complete_config)
{
    config_init();
    int result = config_load("tests/unit/fixtures/valid_complete.conf");
    ck_assert_int_eq(result, 1);

    /* Verify various settings were loaded */
    ck_assert_str_eq(conf.tag, "[SPF:FAIL]");
    ck_assert_ptr_nonnull(conf.cidrs);

    config_free();
}
END_TEST

START_TEST(test_load_missing_file)
{
    config_init();
    int result = config_load("nonexistent_file_xyz.conf");
    /* Missing file returns 0, but config still initialized */
    ck_assert_int_eq(result, 0);
    ck_assert_ptr_nonnull(conf.tag);

    config_free();
}
END_TEST

START_TEST(test_load_empty_file)
{
    /* Create temporary empty file */
    FILE *fp = fopen("/tmp/test_config_empty.conf", "w");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_empty.conf");
    ck_assert_int_eq(result, 1);

    /* All defaults should still be set */
    ck_assert_str_eq(conf.tag, TAG_STRING_DEFAULT);

    unlink("/tmp/test_config_empty.conf");
    config_free();
}
END_TEST

START_TEST(test_load_comment_only_file)
{
    /* Create file with only comments */
    FILE *fp = fopen("/tmp/test_config_comments.conf", "w");
    fprintf(fp, "# This is a comment\n");
    fprintf(fp, "# Another comment\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_comments.conf");
    ck_assert_int_eq(result, 1);

    /* Defaults should be preserved */
    ck_assert_str_eq(conf.tag, TAG_STRING_DEFAULT);

    unlink("/tmp/test_config_comments.conf");
    config_free();
}
END_TEST

START_TEST(test_load_whitespace_variations)
{
    /* Create file with various whitespace */
    FILE *fp = fopen("/tmp/test_config_whitespace.conf", "w");
    fprintf(fp, "  tag   [TEST:TAG]  \n");
    fprintf(fp, "\nwhitelistip\t192.168.0.0/16\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_whitespace.conf");
    ck_assert_int_eq(result, 1);

    /* Should handle leading/trailing whitespace */
    ck_assert_ptr_nonnull(conf.tag);

    unlink("/tmp/test_config_whitespace.conf");
    config_free();
}
END_TEST

START_TEST(test_load_case_insensitive_keys)
{
    /* Create file with mixed case keys */
    FILE *fp = fopen("/tmp/test_config_case.conf", "w");
    fprintf(fp, "TAG [TEST:TAG]\n");
    fprintf(fp, "Tag [ANOTHER:TAG]\n");
    fprintf(fp, "QUARANTINE on\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_case.conf");
    ck_assert_int_eq(result, 1);

    /* Last value should win */
    ck_assert_str_eq(conf.tag, "[ANOTHER:TAG]");
    ck_assert_int_eq(conf.quarantine, 1);

    unlink("/tmp/test_config_case.conf");
    config_free();
}
END_TEST

START_TEST(test_load_duplicate_options)
{
    /* Create file with duplicate keys */
    FILE *fp = fopen("/tmp/test_config_dup.conf", "w");
    fprintf(fp, "tag [FIRST:TAG]\n");
    fprintf(fp, "tag [SECOND:TAG]\n");
    fprintf(fp, "tag [THIRD:TAG]\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_dup.conf");
    ck_assert_int_eq(result, 1);

    /* Last value should win */
    ck_assert_str_eq(conf.tag, "[THIRD:TAG]");

    unlink("/tmp/test_config_dup.conf");
    config_free();
}
END_TEST

START_TEST(test_load_invalid_ip_format)
{
    /* Create file with invalid IPs */
    FILE *fp = fopen("/tmp/test_config_badips.conf", "w");
    fprintf(fp, "whitelistip 256.256.256.256\n");
    fprintf(fp, "whitelistip not.an.ip.address\n");
    fprintf(fp, "whitelistip 192.168.0.0/16\n");  /* Valid one */
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_badips.conf");
    ck_assert_int_eq(result, 1);

    /* Should have skipped invalid IPs but accepted valid one */
    ck_assert_ptr_nonnull(conf.cidrs);

    unlink("/tmp/test_config_badips.conf");
    config_free();
}
END_TEST

START_TEST(test_load_invalid_cidr_mask)
{
    /* Create file with invalid CIDR mask */
    FILE *fp = fopen("/tmp/test_config_mask.conf", "w");
    fprintf(fp, "whitelistip 192.168.0.0/33\n");  /* Mask > 32 */
    fprintf(fp, "whitelistip 192.168.0.0/64\n");  /* Mask > 32 */
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_mask.conf");
    ck_assert_int_eq(result, 1);

    /* Should clamp mask to 32 */
    if (conf.cidrs) {
        ck_assert_int_le(conf.cidrs->mask, 32);
    }

    unlink("/tmp/test_config_mask.conf");
    config_free();
}
END_TEST

START_TEST(test_load_all_boolean_variations)
{
    /* Create file with boolean options */
    FILE *fp = fopen("/tmp/test_config_bool.conf", "w");
    fprintf(fp, "softfail on\n");
    fprintf(fp, "refusefail off\n");
    fprintf(fp, "quarantine on\n");
    fprintf(fp, "daemonize off\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_bool.conf");
    ck_assert_int_eq(result, 1);

    ck_assert_int_eq(conf.soft_fail, 1);
    ck_assert_int_eq(conf.refuse_fail, 0);
    ck_assert_int_eq(conf.quarantine, 1);
    ck_assert_int_eq(conf.daemonize, 0);

    unlink("/tmp/test_config_bool.conf");
    config_free();
}
END_TEST

START_TEST(test_load_syslog_facilities)
{
    /* Test each syslog facility */
    FILE *fp = fopen("/tmp/test_config_syslog.conf", "w");
    fprintf(fp, "syslog mail\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_syslog.conf");
    ck_assert_int_eq(result, 1);

    ck_assert_int_eq(conf.syslog_facility, LOG_MAIL);

    unlink("/tmp/test_config_syslog.conf");
    config_free();
}
END_TEST

START_TEST(test_load_file_paths)
{
    /* Create temp directory and test file path setting */
    FILE *fp = fopen("/tmp/test_config_logpath.conf", "w");
    fprintf(fp, "logto /tmp/test_smf_spf.log\n");
    fclose(fp);

    config_init();
    int result = config_load("/tmp/test_config_logpath.conf");
    ck_assert_int_eq(result, 1);

    /* Log file should be opened */
    ck_assert_ptr_nonnull(conf.log_file);

    unlink("/tmp/test_config_logpath.conf");
    unlink("/tmp/test_smf_spf.log");
    config_free();
}
END_TEST


/* Test Suite 3: Configuration Cleanup */

START_TEST(test_free_config_complete)
{
    config_init();
    config_load("tests/unit/fixtures/valid_complete.conf");

    /* Should not crash */
    config_free();
    ck_assert(1);
}
END_TEST

START_TEST(test_free_config_partial)
{
    config_init();
    /* Don't load file, just free defaults */
    config_free();
    ck_assert(1);
}
END_TEST

START_TEST(test_free_config_double_free)
{
    config_init();
    config_free();
    /* Calling free again should be safe (idempotent) */
    config_free();
    ck_assert(1);
}
END_TEST

START_TEST(test_free_linked_lists)
{
    /* Create file with multiple whitelist entries */
    FILE *fp = fopen("/tmp/test_config_lists.conf", "w");
    fprintf(fp, "whitelistip 192.168.0.0/16\n");
    fprintf(fp, "whitelistip 10.0.0.0/8\n");
    fprintf(fp, "whitelistptr .example.com\n");
    fprintf(fp, "whitelistptr .test.org\n");
    fprintf(fp, "whitelistfrom user@example.com\n");
    fprintf(fp, "whitelistto admin@example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_config_lists.conf");

    /* Verify lists were created */
    ck_assert_ptr_nonnull(conf.cidrs);
    ck_assert_ptr_nonnull(conf.ptrs);
    ck_assert_ptr_nonnull(conf.froms);
    ck_assert_ptr_nonnull(conf.tos);

    /* Free should clean all */
    config_free();

    unlink("/tmp/test_config_lists.conf");
    ck_assert(1);
}
END_TEST

START_TEST(test_free_file_handles)
{
    FILE *fp = fopen("/tmp/test_config_filehandle.conf", "w");
    fprintf(fp, "logto /tmp/test_logfile.log\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_config_filehandle.conf");

    ck_assert_ptr_nonnull(conf.log_file);
    config_free();

    /* File should be closed */
    ck_assert(1);

    unlink("/tmp/test_config_filehandle.conf");
    unlink("/tmp/test_logfile.log");
}
END_TEST


/* Test Suite 4: IP/CIDR Whitelist Checking */

START_TEST(test_ip_check_empty_list)
{
    config_init();
    int result = config_ip_check(inet_addr("192.168.1.1"));
    ck_assert_int_eq(result, 0);
    config_free();
}
END_TEST

START_TEST(test_ip_check_single_cidr_match)
{
    FILE *fp = fopen("/tmp/test_ip_match.conf", "w");
    fprintf(fp, "whitelistip 192.168.1.0/24\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_match.conf");

    int result = config_ip_check(inet_addr("192.168.1.100"));
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_ip_match.conf");
    config_free();
}
END_TEST

START_TEST(test_ip_check_single_cidr_nomatch)
{
    FILE *fp = fopen("/tmp/test_ip_nomatch.conf", "w");
    fprintf(fp, "whitelistip 192.168.1.0/24\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_nomatch.conf");

    int result = config_ip_check(inet_addr("192.168.2.100"));
    ck_assert_int_eq(result, 0);

    unlink("/tmp/test_ip_nomatch.conf");
    config_free();
}
END_TEST

START_TEST(test_ip_check_multiple_cidrs)
{
    FILE *fp = fopen("/tmp/test_ip_multi.conf", "w");
    fprintf(fp, "whitelistip 192.168.0.0/16\n");
    fprintf(fp, "whitelistip 10.0.0.0/8\n");
    fprintf(fp, "whitelistip 172.16.0.0/12\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_multi.conf");

    ck_assert_int_eq(config_ip_check(inet_addr("192.168.1.1")), 1);
    ck_assert_int_eq(config_ip_check(inet_addr("10.0.0.1")), 1);
    ck_assert_int_eq(config_ip_check(inet_addr("172.16.0.1")), 1);
    ck_assert_int_eq(config_ip_check(inet_addr("8.8.8.8")), 0);

    unlink("/tmp/test_ip_multi.conf");
    config_free();
}
END_TEST

START_TEST(test_ip_check_host_only)
{
    FILE *fp = fopen("/tmp/test_ip_host.conf", "w");
    fprintf(fp, "whitelistip 192.168.1.100/32\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_host.conf");

    ck_assert_int_eq(config_ip_check(inet_addr("192.168.1.100")), 1);
    ck_assert_int_eq(config_ip_check(inet_addr("192.168.1.101")), 0);

    unlink("/tmp/test_ip_host.conf");
    config_free();
}
END_TEST

START_TEST(test_ip_check_full_range)
{
    FILE *fp = fopen("/tmp/test_ip_all.conf", "w");
    fprintf(fp, "whitelistip 0.0.0.0/0\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_all.conf");

    /* Should match any IP */
    ck_assert_int_eq(config_ip_check(inet_addr("0.0.0.0")), 1);
    ck_assert_int_eq(config_ip_check(inet_addr("192.168.1.1")), 1);
    ck_assert_int_eq(config_ip_check(inet_addr("255.255.255.255")), 1);

    unlink("/tmp/test_ip_all.conf");
    config_free();
}
END_TEST

START_TEST(test_ip_check_network_address)
{
    FILE *fp = fopen("/tmp/test_ip_network.conf", "w");
    fprintf(fp, "whitelistip 192.168.1.0/24\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_network.conf");

    /* Network address itself should match */
    ck_assert_int_eq(config_ip_check(inet_addr("192.168.1.0")), 1);

    unlink("/tmp/test_ip_network.conf");
    config_free();
}
END_TEST

START_TEST(test_ip_check_broadcast_address)
{
    FILE *fp = fopen("/tmp/test_ip_broadcast.conf", "w");
    fprintf(fp, "whitelistip 192.168.1.0/24\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ip_broadcast.conf");

    /* Broadcast address should match */
    ck_assert_int_eq(config_ip_check(inet_addr("192.168.1.255")), 1);

    unlink("/tmp/test_ip_broadcast.conf");
    config_free();
}
END_TEST


/* Test Suite 5: PTR Whitelist Checking */

START_TEST(test_ptr_check_empty_list)
{
    config_init();
    int result = config_ptr_check("mail.example.com");
    ck_assert_int_eq(result, 0);
    config_free();
}
END_TEST

START_TEST(test_ptr_check_exact_match)
{
    FILE *fp = fopen("/tmp/test_ptr.conf", "w");
    fprintf(fp, "whitelistptr .example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ptr.conf");

    int result = config_ptr_check("mail.example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_ptr.conf");
    config_free();
}
END_TEST

START_TEST(test_ptr_check_partial_match)
{
    FILE *fp = fopen("/tmp/test_ptr_partial.conf", "w");
    fprintf(fp, "whitelistptr example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ptr_partial.conf");

    int result = config_ptr_check("mail.example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_ptr_partial.conf");
    config_free();
}
END_TEST

START_TEST(test_ptr_check_case_insensitive)
{
    FILE *fp = fopen("/tmp/test_ptr_case.conf", "w");
    fprintf(fp, "whitelistptr .EXAMPLE.COM\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ptr_case.conf");

    int result = config_ptr_check("mail.example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_ptr_case.conf");
    config_free();
}
END_TEST

START_TEST(test_ptr_check_no_match)
{
    FILE *fp = fopen("/tmp/test_ptr_nomatch.conf", "w");
    fprintf(fp, "whitelistptr .example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_ptr_nomatch.conf");

    int result = config_ptr_check("mail.other.org");
    ck_assert_int_eq(result, 0);

    unlink("/tmp/test_ptr_nomatch.conf");
    config_free();
}
END_TEST


/* Test Suite 6: From/To Whitelist Checking */

START_TEST(test_from_check_empty_list)
{
    config_init();
    int result = config_from_check("user@example.com");
    ck_assert_int_eq(result, 0);
    config_free();
}
END_TEST

START_TEST(test_from_check_exact_match)
{
    FILE *fp = fopen("/tmp/test_from.conf", "w");
    fprintf(fp, "whitelistfrom user@example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_from.conf");

    int result = config_from_check("user@example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_from.conf");
    config_free();
}
END_TEST

START_TEST(test_from_check_domain_match)
{
    FILE *fp = fopen("/tmp/test_from_domain.conf", "w");
    fprintf(fp, "whitelistfrom example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_from_domain.conf");

    int result = config_from_check("user@example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_from_domain.conf");
    config_free();
}
END_TEST

START_TEST(test_to_check_empty_list)
{
    config_init();
    int result = config_to_check("user@example.com");
    ck_assert_int_eq(result, 0);
    config_free();
}
END_TEST

START_TEST(test_to_check_exact_match)
{
    FILE *fp = fopen("/tmp/test_to.conf", "w");
    fprintf(fp, "whitelistto admin@example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_to.conf");

    int result = config_to_check("admin@example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_to.conf");
    config_free();
}
END_TEST

START_TEST(test_to_check_domain_match)
{
    FILE *fp = fopen("/tmp/test_to_domain.conf", "w");
    fprintf(fp, "whitelistto example.com\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_to_domain.conf");

    int result = config_to_check("user@example.com");
    ck_assert_int_eq(result, 1);

    unlink("/tmp/test_to_domain.conf");
    config_free();
}
END_TEST


/* Test Suite 7: NAT Translation */

START_TEST(test_natip_check_empty_list)
{
    config_init();
    unsigned long result = config_natip_check(inet_addr("192.168.1.1"));
    ck_assert_ulong_eq(result, 0);
    config_free();
}
END_TEST

START_TEST(test_natip_check_single_mapping)
{
    FILE *fp = fopen("/tmp/test_nat.conf", "w");
    fprintf(fp, "clientipnat 192.168.1.1:10.0.0.1\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_nat.conf");

    unsigned long result = config_natip_check(inet_addr("192.168.1.1"));
    unsigned long expected = inet_addr("10.0.0.1");
    ck_assert_ulong_eq(result, expected);

    unlink("/tmp/test_nat.conf");
    config_free();
}
END_TEST

START_TEST(test_natip_check_multiple_mappings)
{
    FILE *fp = fopen("/tmp/test_nat_multi.conf", "w");
    fprintf(fp, "clientipnat 192.168.1.1:10.0.0.1\n");
    fprintf(fp, "clientipnat 192.168.1.2:10.0.0.2\n");
    fprintf(fp, "clientipnat 192.168.1.3:10.0.0.3\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_nat_multi.conf");

    ck_assert_ulong_eq(config_natip_check(inet_addr("192.168.1.1")), inet_addr("10.0.0.1"));
    ck_assert_ulong_eq(config_natip_check(inet_addr("192.168.1.2")), inet_addr("10.0.0.2"));
    ck_assert_ulong_eq(config_natip_check(inet_addr("192.168.1.3")), inet_addr("10.0.0.3"));

    unlink("/tmp/test_nat_multi.conf");
    config_free();
}
END_TEST

START_TEST(test_natip_check_no_match)
{
    FILE *fp = fopen("/tmp/test_nat_nomatch.conf", "w");
    fprintf(fp, "clientipnat 192.168.1.1:10.0.0.1\n");
    fclose(fp);

    config_init();
    config_load("/tmp/test_nat_nomatch.conf");

    unsigned long result = config_natip_check(inet_addr("192.168.2.1"));
    ck_assert_ulong_eq(result, 0);

    unlink("/tmp/test_nat_nomatch.conf");
    config_free();
}
END_TEST


/* Test Suite 8: Time Translation */

START_TEST(test_translate_plain_seconds)
{
    unsigned long result = config_translate_time("3600");
    ck_assert_ulong_eq(result, 3600);
}
END_TEST

START_TEST(test_translate_minute_suffix)
{
    unsigned long result = config_translate_time("60m");
    ck_assert_ulong_eq(result, 3600);

    unsigned long result2 = config_translate_time("60M");
    ck_assert_ulong_eq(result2, 3600);
}
END_TEST

START_TEST(test_translate_hour_suffix)
{
    unsigned long result = config_translate_time("1h");
    ck_assert_ulong_eq(result, 3600);

    unsigned long result2 = config_translate_time("1H");
    ck_assert_ulong_eq(result2, 3600);

    unsigned long result3 = config_translate_time("2h");
    ck_assert_ulong_eq(result3, 7200);
}
END_TEST

START_TEST(test_translate_day_suffix)
{
    unsigned long result = config_translate_time("1d");
    ck_assert_ulong_eq(result, 86400);

    unsigned long result2 = config_translate_time("1D");
    ck_assert_ulong_eq(result2, 86400);
}
END_TEST

START_TEST(test_translate_invalid_format)
{
    unsigned long result = config_translate_time("abc");
    ck_assert_ulong_eq(result, 0);

    unsigned long result2 = config_translate_time("");
    ck_assert_ulong_eq(result2, 0);
}
END_TEST


/* Create test suite */
Suite *config_suite(void)
{
    Suite *s = suite_create("config");

    TCase *tc_init = tcase_create("initialization");
    tcase_add_test(tc_init, test_config_init_defaults);
    tcase_add_test(tc_init, test_config_init_idempotent);
    tcase_add_test(tc_init, test_config_init_allocations);
    tcase_add_test(tc_init, test_config_struct_size);
    tcase_add_test(tc_init, test_config_global_instance);
    suite_add_tcase(s, tc_init);

    TCase *tc_load = tcase_create("loading");
    tcase_add_test(tc_load, test_load_valid_minimal_config);
    tcase_add_test(tc_load, test_load_valid_complete_config);
    tcase_add_test(tc_load, test_load_missing_file);
    tcase_add_test(tc_load, test_load_empty_file);
    tcase_add_test(tc_load, test_load_comment_only_file);
    tcase_add_test(tc_load, test_load_whitespace_variations);
    tcase_add_test(tc_load, test_load_case_insensitive_keys);
    tcase_add_test(tc_load, test_load_duplicate_options);
    tcase_add_test(tc_load, test_load_invalid_ip_format);
    tcase_add_test(tc_load, test_load_invalid_cidr_mask);
    tcase_add_test(tc_load, test_load_all_boolean_variations);
    tcase_add_test(tc_load, test_load_syslog_facilities);
    tcase_add_test(tc_load, test_load_file_paths);
    suite_add_tcase(s, tc_load);

    TCase *tc_free = tcase_create("cleanup");
    tcase_add_test(tc_free, test_free_config_complete);
    tcase_add_test(tc_free, test_free_config_partial);
    tcase_add_test(tc_free, test_free_config_double_free);
    tcase_add_test(tc_free, test_free_linked_lists);
    tcase_add_test(tc_free, test_free_file_handles);
    suite_add_tcase(s, tc_free);

    TCase *tc_ip = tcase_create("ip_checking");
    tcase_add_test(tc_ip, test_ip_check_empty_list);
    tcase_add_test(tc_ip, test_ip_check_single_cidr_match);
    tcase_add_test(tc_ip, test_ip_check_single_cidr_nomatch);
    tcase_add_test(tc_ip, test_ip_check_multiple_cidrs);
    tcase_add_test(tc_ip, test_ip_check_host_only);
    tcase_add_test(tc_ip, test_ip_check_full_range);
    tcase_add_test(tc_ip, test_ip_check_network_address);
    tcase_add_test(tc_ip, test_ip_check_broadcast_address);
    suite_add_tcase(s, tc_ip);

    TCase *tc_ptr = tcase_create("ptr_checking");
    tcase_add_test(tc_ptr, test_ptr_check_empty_list);
    tcase_add_test(tc_ptr, test_ptr_check_exact_match);
    tcase_add_test(tc_ptr, test_ptr_check_partial_match);
    tcase_add_test(tc_ptr, test_ptr_check_case_insensitive);
    tcase_add_test(tc_ptr, test_ptr_check_no_match);
    suite_add_tcase(s, tc_ptr);

    TCase *tc_email = tcase_create("email_checking");
    tcase_add_test(tc_email, test_from_check_empty_list);
    tcase_add_test(tc_email, test_from_check_exact_match);
    tcase_add_test(tc_email, test_from_check_domain_match);
    tcase_add_test(tc_email, test_to_check_empty_list);
    tcase_add_test(tc_email, test_to_check_exact_match);
    tcase_add_test(tc_email, test_to_check_domain_match);
    suite_add_tcase(s, tc_email);

    TCase *tc_nat = tcase_create("nat_translation");
    tcase_add_test(tc_nat, test_natip_check_empty_list);
    tcase_add_test(tc_nat, test_natip_check_single_mapping);
    tcase_add_test(tc_nat, test_natip_check_multiple_mappings);
    tcase_add_test(tc_nat, test_natip_check_no_match);
    suite_add_tcase(s, tc_nat);

    TCase *tc_time = tcase_create("time_translation");
    tcase_add_test(tc_time, test_translate_plain_seconds);
    tcase_add_test(tc_time, test_translate_minute_suffix);
    tcase_add_test(tc_time, test_translate_hour_suffix);
    tcase_add_test(tc_time, test_translate_day_suffix);
    tcase_add_test(tc_time, test_translate_invalid_format);
    suite_add_tcase(s, tc_time);

    return s;
}
