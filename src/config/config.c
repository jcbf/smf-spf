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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "config.h"
#include "defaults.h"

/* Forward declarations to avoid circular include with arpa/inet.h */
extern unsigned long inet_addr(const char *cp);
#define htonl(x) ((unsigned long)(x))
#define ntohl(x) ((unsigned long)(x))

/* Global configuration instance */
config_t conf;

/* Facilities mapping for syslog */
typedef struct {
    char *name;
    int facility;
} facilities_t;

static facilities_t syslog_facilities[] = {
    { "daemon", LOG_DAEMON },
    { "mail", LOG_MAIL },
    { "local0", LOG_LOCAL0 },
    { "local1", LOG_LOCAL1 },
    { "local2", LOG_LOCAL2 },
    { "local3", LOG_LOCAL3 },
    { "local4", LOG_LOCAL4 },
    { "local5", LOG_LOCAL5 },
    { "local6", LOG_LOCAL6 },
    { "local7", LOG_LOCAL7 }
};

/* Helper macros */
#define SAFE_FREE(x) if (x) { free(x); x = NULL; }

/* Forward declarations */
static unsigned long config_ip_cidr(unsigned long ip, unsigned short int mask, unsigned long checkip);
static char *config_trim_space(char *str);
static void config_strtolower(char *str);


/**
 * Trim leading and trailing whitespace from string
 * Returns pointer to trimmed string (modified in-place)
 */
static char *config_trim_space(char *str) {
    char *end;

    while (isspace((unsigned char)*str))
        str++;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    *(end + 1) = '\0';
    return str;
}


/**
 * Convert string to lowercase (ASCII only)
 */
static void config_strtolower(char *str) {
    for (; *str; str++)
        if (isascii(*str) && isupper(*str))
            *str = tolower(*str);
}


/**
 * config_translate_time - Convert time string with suffix to seconds
 * @str: String like "60m", "1h", "1d", or plain number
 *
 * Supports: m/M for minutes, h/H for hours, d/D for days
 * Returns: Number of seconds, or 0 on invalid input
 */
unsigned long config_translate_time(const char *str) {
    unsigned long unit;
    char *value;
    size_t len;

    if (!str || !*str)
        return 0;

    value = strdup(str);
    if (!value)
        return 0;

    len = strlen(value);
    if (len == 0) {
        free(value);
        return 0;
    }

    switch (value[len - 1]) {
    case 'm':
    case 'M':
        unit = 60;
        value[len - 1] = '\0';
        break;
    case 'h':
    case 'H':
        unit = 3600;
        value[len - 1] = '\0';
        break;
    case 'd':
    case 'D':
        unit = 86400;
        value[len - 1] = '\0';
        break;
    default:
        unit = 1;
        break;
    }

    unsigned long result = atol(value) * unit;
    free(value);
    return result;
}


/**
 * config_ip_cidr - Check if IP is within CIDR range
 * @ip: Network IP address
 * @mask: CIDR mask (0-32)
 * @checkip: IP address to check
 *
 * Returns: 1 if checkip is in range, 0 otherwise
 */
static unsigned long config_ip_cidr(unsigned long ip, unsigned short int mask, unsigned long checkip) {
    unsigned long subnet, cidrip, ipaddr;

    subnet = ~0UL;
    subnet = subnet << (32 - mask);
    cidrip = htonl(ip) & subnet;
    ipaddr = ntohl(checkip) & subnet;

    return (cidrip == ipaddr) ? 1 : 0;
}


/**
 * config_free - Free all allocated configuration resources
 *
 * Safely frees all linked lists, strings, and file handles.
 * Can be called multiple times safely.
 */
void config_free(void) {
    SAFE_FREE(conf.tag);
    SAFE_FREE(conf.quarantine_box);
    SAFE_FREE(conf.run_as_user);
    SAFE_FREE(conf.sendmail_socket);
    SAFE_FREE(conf.fixed_ip);
    SAFE_FREE(conf.reject_reason);

    if (conf.log_file != NULL) {
        fclose(conf.log_file);
        conf.log_file = NULL;
    }

    /* Free IPNAT list */
    if (conf.ipnats) {
        IPNAT *it = conf.ipnats, *it_next;
        while (it) {
            it_next = it->next;
            SAFE_FREE(it);
            it = it_next;
        }
        conf.ipnats = NULL;
    }

    /* Free CIDR list */
    if (conf.cidrs) {
        CIDR *it = conf.cidrs, *it_next;
        while (it) {
            it_next = it->next;
            SAFE_FREE(it);
            it = it_next;
        }
        conf.cidrs = NULL;
    }

    /* Free PTR list */
    if (conf.ptrs) {
        STR *it = conf.ptrs, *it_next;
        while (it) {
            it_next = it->next;
            SAFE_FREE(it->str);
            SAFE_FREE(it);
            it = it_next;
        }
        conf.ptrs = NULL;
    }

    /* Free From list */
    if (conf.froms) {
        STR *it = conf.froms, *it_next;
        while (it) {
            it_next = it->next;
            SAFE_FREE(it->str);
            SAFE_FREE(it);
            it = it_next;
        }
        conf.froms = NULL;
    }

    /* Free To list */
    if (conf.tos) {
        STR *it = conf.tos, *it_next;
        while (it) {
            it_next = it->next;
            SAFE_FREE(it->str);
            SAFE_FREE(it);
            it = it_next;
        }
        conf.tos = NULL;
    }
}


/**
 * config_init - Initialize configuration with defaults
 *
 * Sets up all configuration variables with their default values.
 * Must be called before config_load().
 *
 * Returns: 0 on success, -1 on failure
 */
int config_init(void) {
    /* Initialize string fields */
    conf.tag = strdup(TAG_STRING_DEFAULT);
    conf.quarantine_box = strdup(QUARANTINE_BOX_DEFAULT);
    conf.run_as_user = strdup(USER_DEFAULT);
    conf.sendmail_socket = strdup(OCONN_DEFAULT);
    conf.fixed_ip = NULL;
    conf.reject_reason = strdup(REJECT_REASON_DEFAULT);

    /* Initialize lists */
    conf.ipnats = NULL;
    conf.cidrs = NULL;
    conf.ptrs = NULL;
    conf.froms = NULL;
    conf.tos = NULL;

    /* Initialize boolean flags */
    conf.relaxed_localpart = RELAXED_LOCALPART_DEFAULT;
    conf.best_guess = BEST_GUESS_DEFAULT;
    conf.refuse_fail = REFUSE_FAIL_DEFAULT;
    conf.refuse_none = REFUSE_NONE_DEFAULT;
    conf.refuse_none_helo = REFUSE_NONE_HELO_DEFAULT;
    conf.soft_fail = SOFT_FAIL_DEFAULT;
    conf.accept_temperror = ACCEPT_PERMERR_DEFAULT;
    conf.tag_subject = TAG_SUBJECT_DEFAULT;
    conf.add_header = ADD_HEADER_DEFAULT;
    conf.add_recv_spf_header = ADD_RECV_HEADER_DEFAULT;
    conf.quarantine = QUARANTINE_DEFAULT;
    conf.daemonize = DAEMONIZE_DEFAULT;
    conf.skip_ndr = SKIP_NDR_DEFAULT;
    conf.skip_auth = SKIP_AUTH_DEFAULT;

    /* Initialize file and system settings */
    conf.log_file = NULL;
    conf.syslog_facility = SYSLOG_FACILITY_DEFAULT;
    conf.spf_ttl = SPF_TTL_DEFAULT;

    return 0;
}


/**
 * config_load - Load configuration from file
 * @filepath: Path to configuration file
 *
 * Parses configuration file and populates conf structure.
 * Must call config_init() first.
 *
 * Returns: 1 on success, 0 on failure or file not found
 */
int config_load(const char *filepath) {
    FILE *fp;
    char buf[2 * MAXLINE];
    int cidr_count = 0;

    if (!filepath)
        filepath = CONFIG_FILE_DEFAULT;

    fp = fopen(filepath, "r");
    if (!fp)
        return 0;  /* File not found is not an error */

    while (fgets(buf, sizeof(buf) - 1, fp)) {
        char key[MAXLINE];
        char val[MAXLINE];
        char value[MAXLINE];
        char *p = NULL;

        /* Remove comments */
        if ((p = strchr(buf, '#')))
            *p = '\0';

        /* Skip empty lines */
        if (strlen(buf) == 0)
            continue;

        /* Parse key-value pair */
        if (sscanf(buf, "%127s %[^\n]s", key, value) != 2)
            continue;

        strcpy(val, config_trim_space(value));

        /* whitelistip key: whitelist_ip[/mask] */
        if (!strcasecmp(key, "whitelistip")) {
            char *slash = NULL;
            unsigned short int mask = 32;

            if ((slash = strchr(val, '/'))) {
                *slash = '\0';
                mask = atoi(++slash);
                if (mask > 32)
                    mask = 32;
            }

            if (val[0]) {
                CIDR *it = NULL;
                unsigned long ip;

                if ((ip = inet_addr(val)) == 0xffffffff)
                    continue;

                /* Check resource limits */
                if (cidr_count++ > MAX_WHITELIST_ENTRIES) {
                    syslog(LOG_ERR, "[CONFIG] Too many CIDR entries (>%d), skipping",
                           MAX_WHITELIST_ENTRIES);
                    continue;
                }

                if (!conf.cidrs)
                    conf.cidrs = (CIDR *)calloc(1, sizeof(CIDR));
                else if ((it = (CIDR *)calloc(1, sizeof(CIDR)))) {
                    it->next = conf.cidrs;
                    conf.cidrs = it;
                }

                if (conf.cidrs) {
                    conf.cidrs->ip = ip;
                    conf.cidrs->mask = mask;
                }
            }
            continue;
        }

        /* clientipnat key: src_ip:dest_ip */
        if (!strcasecmp(key, "clientipnat")) {
            char *sep = NULL;
            unsigned long d_ip;

            if ((sep = strchr(val, ':'))) {
                *sep++ = '\0';
                if (*sep) {
                    if ((d_ip = inet_addr(sep)) == 0xffffffff) {
                        syslog(LOG_ERR, "[CONFIG] Invalid NAT destination IP: %s", sep);
                        continue;
                    }
                } else {
                    syslog(LOG_ERR, "[CONFIG] Invalid destination IP format: %s", sep);
                    continue;
                }
            } else {
                syslog(LOG_ERR, "[CONFIG] Invalid NAT entry format: %s (must be src_ip:dest_ip)", val);
                continue;
            }

            if (val[0]) {
                IPNAT *it = NULL;
                unsigned long s_ip;

                if ((s_ip = inet_addr(val)) == 0xffffffff) {
                    syslog(LOG_ERR, "[CONFIG] Invalid NAT source IP: %s", val);
                    continue;
                }

                if (!conf.ipnats)
                    conf.ipnats = (IPNAT *)calloc(1, sizeof(IPNAT));
                else if ((it = (IPNAT *)calloc(1, sizeof(IPNAT)))) {
                    it->next = conf.ipnats;
                    conf.ipnats = it;
                }

                if (conf.ipnats) {
                    conf.ipnats->srcip = s_ip;
                    conf.ipnats->destip = d_ip;
                }
            } else {
                syslog(LOG_ERR, "[CONFIG] Invalid source IP format: %s", val);
            }
            continue;
        }

        /* whitelistptr key */
        if (!strcasecmp(key, "whitelistptr")) {
            STR *it = NULL;

            if (!conf.ptrs)
                conf.ptrs = (STR *)calloc(1, sizeof(STR));
            else if ((it = (STR *)calloc(1, sizeof(STR)))) {
                it->next = conf.ptrs;
                conf.ptrs = it;
            }

            if (conf.ptrs && !conf.ptrs->str)
                conf.ptrs->str = strdup(val);

            continue;
        }

        /* whitelistfrom key */
        if (!strcasecmp(key, "whitelistfrom")) {
            STR *it = NULL;

            if (!conf.froms)
                conf.froms = (STR *)calloc(1, sizeof(STR));
            else if ((it = (STR *)calloc(1, sizeof(STR)))) {
                it->next = conf.froms;
                conf.froms = it;
            }

            if (conf.froms && !conf.froms->str) {
                config_strtolower(val);
                conf.froms->str = strdup(val);
            }

            continue;
        }

        /* whitelistto key */
        if (!strcasecmp(key, "whitelistto")) {
            STR *it = NULL;

            if (!conf.tos)
                conf.tos = (STR *)calloc(1, sizeof(STR));
            else if ((it = (STR *)calloc(1, sizeof(STR)))) {
                it->next = conf.tos;
                conf.tos = it;
            }

            if (conf.tos && !conf.tos->str) {
                config_strtolower(val);
                conf.tos->str = strdup(val);
            }

            continue;
        }

        /* Boolean options */
        if (!strcasecmp(key, "accepttemperror") && !strcasecmp(val, "off")) {
            conf.accept_temperror = 0;
            continue;
        }
        if (!strcasecmp(key, "softfail") && !strcasecmp(val, "on")) {
            conf.soft_fail = 1;
            continue;
        }
        if (!strcasecmp(key, "refusespfnone") && !strcasecmp(val, "on")) {
            conf.refuse_none = 1;
            continue;
        }
        if (!strcasecmp(key, "refusespfnonehelo") && !strcasecmp(val, "on")) {
            conf.refuse_none_helo = 1;
            continue;
        }
        if (!strcasecmp(key, "relaxedlocalpart") && !strcasecmp(val, "on")) {
            conf.relaxed_localpart = 1;
            continue;
        }
        if (!strcasecmp(key, "spfbestguess") && !strcasecmp(val, "off")) {
            conf.best_guess = 0;
            continue;
        }
        if (!strcasecmp(key, "refusefail") && !strcasecmp(val, "off")) {
            conf.refuse_fail = 0;
            continue;
        }
        if (!strcasecmp(key, "tagsubject") && !strcasecmp(val, "off")) {
            conf.tag_subject = 0;
            continue;
        }
        if (!strcasecmp(key, "addheader") && !strcasecmp(val, "off")) {
            conf.add_header = 0;
            continue;
        }
        if (!strcasecmp(key, "addreceivedheader") && !strcasecmp(val, "on")) {
            conf.add_recv_spf_header = 1;
            continue;
        }
        if (!strcasecmp(key, "skipndr") && !strcasecmp(val, "on")) {
            conf.skip_ndr = true;
            continue;
        }
        if (!strcasecmp(key, "skipauth") && !strcasecmp(val, "off")) {
            conf.skip_auth = false;
            continue;
        }
        if (!strcasecmp(key, "quarantine") && !strcasecmp(val, "on")) {
            conf.quarantine = 1;
            continue;
        }
        if (!strcasecmp(key, "daemonize") && !strcasecmp(val, "off")) {
            conf.daemonize = 0;
            continue;
        }

        /* String configuration options */
        if (!strcasecmp(key, "tag")) {
            SAFE_FREE(conf.tag);
            conf.tag = strdup(val);
            continue;
        }
        if (!strcasecmp(key, "fixedclientip")) {
            conf.fixed_ip = strdup(val);
            continue;
        }
        if (!strcasecmp(key, "rejectreason")) {
            SAFE_FREE(conf.reject_reason);
            conf.reject_reason = strdup(val);
            continue;
        }
        if (!strcasecmp(key, "quarantinebox")) {
            SAFE_FREE(conf.quarantine_box);
            conf.quarantine_box = strdup(val);
            continue;
        }
        if (!strcasecmp(key, "user")) {
            SAFE_FREE(conf.run_as_user);
            conf.run_as_user = strdup(val);
            continue;
        }
        if (!strcasecmp(key, "socket")) {
            SAFE_FREE(conf.sendmail_socket);
            conf.sendmail_socket = strdup(val);
            continue;
        }

        /* File and logging options */
        if (!strcasecmp(key, "logto")) {
            FILE *logfp = fopen(val, "a");
            if (!logfp) {
                fprintf(stderr, "Error: Cannot open log file %s: %s\n", val, strerror(errno));
                fclose(fp);
                return 0;
            }
            if (conf.log_file)
                fclose(conf.log_file);
            conf.log_file = logfp;
            continue;
        }

        /* TTL option */
        if (!strcasecmp(key, "ttl")) {
            unsigned long ttl = config_translate_time(val);
            if (ttl < 60)
                syslog(LOG_WARNING, "[CONFIG] TTL %lu is very low, consider >= 60s", ttl);
            if (ttl > 86400)
                syslog(LOG_WARNING, "[CONFIG] TTL %lu is very high, consider <= 86400s (24h)", ttl);
            conf.spf_ttl = ttl;
            continue;
        }

        /* Syslog facility */
        if (!strcasecmp(key, "syslog")) {
            int i;
            if (!strcasecmp(val, "none")) {
                conf.syslog_facility = SYSLOG_DISABLE;
            } else {
                for (i = 0; i < FACILITIES_AMOUNT; i++) {
                    if (!strcasecmp(val, syslog_facilities[i].name)) {
                        conf.syslog_facility = syslog_facilities[i].facility;
                        break;
                    }
                }
            }
            continue;
        }

        /* Note: authservid is handled by main, not configuration module */
    }

    fclose(fp);
    return 1;
}


/**
 * config_ip_check - Check if IP is in whitelist
 * @check_ip: IP address to check
 *
 * Returns: 1 if IP is whitelisted, 0 otherwise
 */
int config_ip_check(unsigned long check_ip) {
    CIDR *it = conf.cidrs;

    while (it) {
        if (config_ip_cidr(it->ip, it->mask, check_ip))
            return 1;
        it = it->next;
    }
    return 0;
}


/**
 * config_natip_check - Check and translate IP via NAT rules
 * @check_ip: IP address to check
 *
 * Returns: Translated IP if match found, 0 otherwise
 */
unsigned long config_natip_check(unsigned long check_ip) {
    IPNAT *it = conf.ipnats;

    while (it) {
        if (it->srcip == check_ip)
            return it->destip;
        it = it->next;
    }
    return 0;
}


/**
 * config_ptr_check - Check if PTR is in whitelist
 * @ptr: PTR record to check
 *
 * Uses substring matching (PTR must end with whitelisted string)
 * Returns: 1 if whitelisted, 0 otherwise
 */
int config_ptr_check(const char *ptr) {
    STR *it = conf.ptrs;

    while (it) {
        if (it->str && strlen(it->str) <= strlen(ptr) &&
            !strcasecmp(ptr + strlen(ptr) - strlen(it->str), it->str))
            return 1;
        it = it->next;
    }
    return 0;
}


/**
 * config_from_check - Check if From is in whitelist
 * @from: From address to check
 *
 * Uses substring matching
 * Returns: 1 if whitelisted, 0 otherwise
 */
int config_from_check(const char *from) {
    STR *it = conf.froms;

    while (it) {
        if (it->str && strstr(from, it->str))
            return 1;
        it = it->next;
    }
    return 0;
}


/**
 * config_to_check - Check if To is in whitelist
 * @to: To address to check
 *
 * Uses substring matching
 * Returns: 1 if whitelisted, 0 otherwise
 */
int config_to_check(const char *to) {
    STR *it = conf.tos;

    while (it) {
        if (it->str && strstr(to, it->str))
            return 1;
        it = it->next;
    }
    return 0;
}
