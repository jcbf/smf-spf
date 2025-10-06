/*
 * ip_utils.c - IP address utilities for smf-spf
 *
 * This file is part of smf-spf.
 *
 * smf-spf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * smf-spf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ip_utils.h"
#include <arpa/inet.h>

int ip_cidr_match(unsigned long ip, unsigned short int mask, unsigned long check_ip) {
    unsigned long cidrip = 0;
    unsigned long ipaddr = 0;
    unsigned long subnet = 0;

    if (mask > 32) {
        mask = 32;
    }

    /* Create subnet mask */
    subnet = ~0UL;
    subnet = subnet << (32 - mask);

    /* Apply mask to both IPs and compare */
    cidrip = htonl(ip) & subnet;
    ipaddr = ntohl(check_ip) & subnet;

    return (cidrip == ipaddr) ? 1 : 0;
}
