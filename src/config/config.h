/*  Copyright (C) 2005, 2006 by Eugene Kurmanin <me@kurmanin.info>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdbool.h>

/* Data Structures */
typedef struct CIDR {
    unsigned long ip;
    unsigned short int mask;
    struct CIDR *next;
} CIDR;

typedef struct IPNAT {
    unsigned long srcip;
    unsigned long destip;
    struct IPNAT *next;
} IPNAT;

typedef struct STR {
    char *str;
    struct STR *next;
} STR;

typedef struct config {
    char *tag;
    char *quarantine_box;
    char *run_as_user;
    char *sendmail_socket;
    char *fixed_ip;
    char *reject_reason;

    IPNAT *ipnats;
    CIDR *cidrs;
    STR *ptrs;
    STR *froms;
    STR *tos;

    int relaxed_localpart;
    int best_guess;
    int refuse_fail;
    int refuse_none;
    int refuse_none_helo;
    int soft_fail;
    int accept_temperror;
    int tag_subject;
    int add_header;
    int add_recv_spf_header;
    int quarantine;
    int daemonize;
    bool skip_ndr;
    bool skip_auth;

    FILE *log_file;
    int syslog_facility;

    unsigned long spf_ttl;
} config_t;

/* Backward compatibility alias */
typedef config_t config;

/* Global Configuration Instance */
extern config_t conf;

/* Initialization and Cleanup */
int config_init(void);
int config_load(const char *filepath);
void config_free(void);

/* Whitelist Checkers */
int config_ip_check(unsigned long check_ip);
unsigned long config_natip_check(unsigned long check_ip);
int config_ptr_check(const char *ptr);
int config_from_check(const char *from);
int config_to_check(const char *to);

/* Helper Functions */
unsigned long config_translate_time(const char *str);

#endif /* CONFIG_H */
