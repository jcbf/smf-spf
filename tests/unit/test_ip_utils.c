/*
 * test_ip_utils.c - Unit tests for IP address utilities
 */

#include <check.h>
#include <arpa/inet.h>
#include "ip_utils.h"

/* Helper function to convert dotted quad to network byte order */
static unsigned long ip_aton(const char *str)
{
    struct in_addr addr;
    inet_aton(str, &addr);
    return addr.s_addr;
}

/* Test ip_cidr_match function */
START_TEST(test_ip_cidr_match_simple_match)
{
    /* 192.168.1.0/24 contains 192.168.1.100 */
    unsigned long network = ip_aton("192.168.1.0");
    unsigned long check = ip_aton("192.168.1.100");
    int result = ip_cidr_match(network, 24, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_simple_no_match)
{
    /* 192.168.1.0/24 does not contain 192.168.2.100 */
    unsigned long network = ip_aton("192.168.1.0");
    unsigned long check = ip_aton("192.168.2.100");
    int result = ip_cidr_match(network, 24, check);
    ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_ip_cidr_match_network_address)
{
    /* Network address should match itself */
    unsigned long network = ip_aton("192.168.1.0");
    unsigned long check = ip_aton("192.168.1.0");
    int result = ip_cidr_match(network, 24, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_broadcast_address)
{
    /* Broadcast address should match */
    unsigned long network = ip_aton("192.168.1.0");
    unsigned long check = ip_aton("192.168.1.255");
    int result = ip_cidr_match(network, 24, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_32_bit_mask)
{
    /* /32 should only match exact IP */
    unsigned long network = ip_aton("192.168.1.100");
    unsigned long check = ip_aton("192.168.1.100");
    int result = ip_cidr_match(network, 32, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_32_bit_no_match)
{
    /* /32 with different IP */
    unsigned long network = ip_aton("192.168.1.100");
    unsigned long check = ip_aton("192.168.1.101");
    int result = ip_cidr_match(network, 32, check);
    ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_ip_cidr_match_8_bit_mask)
{
    /* 192.0.0.0/8 contains 192.255.255.255 */
    unsigned long network = ip_aton("192.0.0.0");
    unsigned long check = ip_aton("192.255.255.255");
    int result = ip_cidr_match(network, 8, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_8_bit_no_match)
{
    /* 192.0.0.0/8 does not contain 191.255.255.255 */
    unsigned long network = ip_aton("192.0.0.0");
    unsigned long check = ip_aton("191.255.255.255");
    int result = ip_cidr_match(network, 8, check);
    ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_ip_cidr_match_16_bit_mask)
{
    /* 172.16.0.0/16 contains 172.16.255.255 */
    unsigned long network = ip_aton("172.16.0.0");
    unsigned long check = ip_aton("172.16.255.255");
    int result = ip_cidr_match(network, 16, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_16_bit_no_match)
{
    /* 172.16.0.0/16 does not contain 172.17.0.0 */
    unsigned long network = ip_aton("172.16.0.0");
    unsigned long check = ip_aton("172.17.0.0");
    int result = ip_cidr_match(network, 16, check);
    ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_ip_cidr_match_zero_mask)
{
    /* 0.0.0.0/0 matches everything */
    unsigned long network = ip_aton("0.0.0.0");
    unsigned long check = ip_aton("255.255.255.255");
    int result = ip_cidr_match(network, 0, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_localhost)
{
    /* 127.0.0.0/8 contains 127.0.0.1 */
    unsigned long network = ip_aton("127.0.0.0");
    unsigned long check = ip_aton("127.0.0.1");
    int result = ip_cidr_match(network, 8, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_private_10)
{
    /* 10.0.0.0/8 (private range) */
    unsigned long network = ip_aton("10.0.0.0");
    unsigned long check = ip_aton("10.255.255.255");
    int result = ip_cidr_match(network, 8, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_mask_over_32)
{
    /* Mask > 32 should be treated as 32 */
    unsigned long network = ip_aton("192.168.1.100");
    unsigned long check = ip_aton("192.168.1.100");
    int result = ip_cidr_match(network, 33, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_ip_cidr_match_mask_50)
{
    /* Mask of 50 should be treated as 32 (exact match required) */
    unsigned long network = ip_aton("192.168.1.100");
    unsigned long check = ip_aton("192.168.1.100");
    int result = ip_cidr_match(network, 50, check);
    ck_assert_int_eq(result, 1);
}
END_TEST

/* Create test suite */
Suite *ip_utils_suite(void)
{
    Suite *s = suite_create("IP Utils");
    TCase *tc_core = tcase_create("ip_cidr_match");

    tcase_add_test(tc_core, test_ip_cidr_match_simple_match);
    tcase_add_test(tc_core, test_ip_cidr_match_simple_no_match);
    tcase_add_test(tc_core, test_ip_cidr_match_network_address);
    tcase_add_test(tc_core, test_ip_cidr_match_broadcast_address);
    tcase_add_test(tc_core, test_ip_cidr_match_32_bit_mask);
    tcase_add_test(tc_core, test_ip_cidr_match_32_bit_no_match);
    tcase_add_test(tc_core, test_ip_cidr_match_8_bit_mask);
    tcase_add_test(tc_core, test_ip_cidr_match_8_bit_no_match);
    tcase_add_test(tc_core, test_ip_cidr_match_16_bit_mask);
    tcase_add_test(tc_core, test_ip_cidr_match_16_bit_no_match);
    tcase_add_test(tc_core, test_ip_cidr_match_zero_mask);
    tcase_add_test(tc_core, test_ip_cidr_match_localhost);
    tcase_add_test(tc_core, test_ip_cidr_match_private_10);
    tcase_add_test(tc_core, test_ip_cidr_match_mask_over_32);
    tcase_add_test(tc_core, test_ip_cidr_match_mask_50);

    suite_add_tcase(s, tc_core);
    return s;
}
