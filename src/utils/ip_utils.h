/*
 * ip_utils.h - IP address utilities for smf-spf
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

#ifndef SMF_SPF_IP_UTILS_H
#define SMF_SPF_IP_UTILS_H

/**
 * @brief Check if an IP address matches a CIDR block
 *
 * Tests whether the given IP address falls within the specified
 * CIDR network range.
 *
 * @param ip CIDR network address (network byte order)
 * @param mask CIDR mask (number of bits, 0-32)
 * @param check_ip IP address to check (network byte order)
 * @return 1 if IP matches CIDR block, 0 otherwise
 */
int ip_cidr_match(unsigned long ip, unsigned short int mask, unsigned long check_ip);

#endif /* SMF_SPF_IP_UTILS_H */
