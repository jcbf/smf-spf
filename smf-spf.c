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

#ifndef _REENTRANT
#error Compile with -D_REENTRANT flag
#endif

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifndef __sun__
#include <getopt.h>
#endif
#include <grp.h>
#include <libmilter/mfapi.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include "spf2/spf.h"

#define CONFIG_FILE		"/etc/mail/smfs/smf-spf.conf"
#define WORK_SPACE		"/var/run/smfs"
#define OCONN			"unix:" WORK_SPACE "/smf-spf.sock"
#define USER			"smfs"
#define TAG_STRING		"[SPF:fail]"
#define DEFHOSTNAME		"localhost"
#define QUARANTINE_BOX		"postmaster"
#define SYSLOG_FACILITY		LOG_MAIL
#define SPF_TTL			3600
#define RELAXED_LOCALPART	0
#define REFUSE_FAIL		1
#define REFUSE_NONE		0
#define REFUSE_NONE_HELO		0
#define SOFT_FAIL		0
#define ACCEPT_PERMERR		1
#define TAG_SUBJECT		1
#define ADD_HEADER		1
#define ADD_RECV_HEADER		0
#define QUARANTINE		0
#define DAEMONIZE		1
#define SKIP_AUTH		true
#define VERSION			"2.5.2"
#define REJECT_REASON	"Message was rejected during SPF policy evaluation. sender:%1$s client-ip:%2$s"
#define SYSLOG_DISABLE	-2
#define SKIP_NDR		false

#define MAX_HEADER_SIZE		2048
#define MAXLINE			258
#define MAXLOCALPART	64
#define HASH_POWER		16
#define FACILITIES_AMOUNT	10
#define IPV4_DOT_DECIMAL	"^[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}$"

#define SAFE_FREE(x)		if (x) { free(x); x = NULL; }

#define hash_size(x)		((unsigned long) 1 << x)
#define hash_mask(x)		(hash_size(x) - 1)

#ifdef __sun__
int daemon(int nochdir, int noclose) {
    pid_t pid;
    int fd = 0;

    if ((pid = fork()) < 0) {
	fprintf(stderr, "fork: %s\n", strerror(errno));
	return 1;
    }
    else
	if (pid > 0) _exit(0);
    if ((pid = setsid()) == -1) {
	fprintf(stderr, "setsid: %s\n", strerror(errno));
	return 1;
    }
    if ((pid = fork()) < 0) {
	fprintf(stderr, "fork: %s\n", strerror(errno));
	return 1;
    }
    else
	if (pid > 0) _exit(0);
    if (!nochdir && chdir("/")) {
	fprintf(stderr, "chdir: %s\n", strerror(errno));
	return 1;
    }
    if (!noclose) {
	dup2(fd, fileno(stdout));
	dup2(fd, fileno(stderr));
	dup2(open("/dev/null", O_RDONLY, 0), fileno(stdin));
    }
    return 0;
}
#endif

typedef struct cache_item {
    char *item;
    unsigned long hash;
    SPF_result_t status;
    time_t exptime;
    struct cache_item *next;
} cache_item;

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
    FILE *log_file;
    char *run_as_user;
    char *sendmail_socket;
    IPNAT *ipnats;
    CIDR *cidrs;
    STR *ptrs;
    STR *froms;
    STR *tos;
    int relaxed_localpart;
    int refuse_fail;
    int refuse_none;
    int refuse_none_helo;
    int soft_fail;
    int accept_temperror;
    int tag_subject;
    int add_header;
    int add_recv_spf_header;
    int quarantine;
    int syslog_facility;
    int daemonize;
    bool skip_ndr;
    unsigned long spf_ttl;
    char *fixed_ip;
    bool skip_auth;
    char *reject_reason;
} config;

typedef struct facilities {
    char *name;
    int facility;
} facilities;

struct context {
    char addr[64];
    char fqdn[MAXLINE];
    char site[MAXLINE];
    char helo[MAXLINE];
    char from[MAXLINE];
    char sender[MAXLINE+12];
    char rcpt[MAXLINE];
    char recipient[MAXLINE];
    char key[MAXLINE];
    char *subject;
    STR *rcpts;
    SPF_result_t status;
};

static regex_t re_ipv4;
static cache_item **cache = NULL;
static const char *config_file = CONFIG_FILE;
static int foreground = 0;
static config conf;
static char *daemon_name;
static char hostname[HOST_NAME_MAX+1];
static pid_t mypid = 0;
static pthread_mutex_t cache_mutex;
static facilities syslog_facilities[] = {
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
static char *authserv_id = NULL;

static sfsistat smf_connect(SMFICTX *, char *, _SOCK_ADDR *);
static sfsistat smf_helo(SMFICTX *, char *);
static sfsistat smf_envfrom(SMFICTX *, char **);
static sfsistat smf_envrcpt(SMFICTX *, char **);
static sfsistat smf_header(SMFICTX *, char *, char *);
static sfsistat smf_eom(SMFICTX *);
static sfsistat smf_close(SMFICTX *);
static char * trim_space(char *str);

static void log_init() {
	if ( conf.syslog_facility != SYSLOG_DISABLE) 
	    openlog(daemon_name, LOG_PID|LOG_NDELAY, conf.syslog_facility);
}

static void log_message(int log_level, const char *fmt, ...) {
	va_list ap;
	
	char time_str[32];
    struct tm *tm;

	if (conf.log_file) {
		va_start(ap, fmt);
		time_t now = time (0);
		tm = localtime (&now);		
		strftime (time_str, sizeof(time_str), "%h %e %T", tm);

		fprintf(conf.log_file,"%s %s %s[%d]: ",time_str,hostname,daemon_name,getpid());
		vfprintf(conf.log_file,fmt, ap);
		fprintf(conf.log_file,"\n");
		fflush(conf.log_file);
	}
	if ( conf.syslog_facility != SYSLOG_DISABLE) {
		va_start(ap, fmt);
		vsyslog(log_level, fmt, ap);
	}

	va_end(ap);
}

static void strscpy(register char *dst, register const char *src, size_t size) {
    register size_t i;

    for (i = 0; i < size && (dst[i] = src[i]) != 0; i++) continue;
    dst[i] = '\0';
}

static void strtolower(register char *str) {

    for (; *str; str++)
	if (isascii(*str) && isupper(*str)) *str = tolower(*str);
}

static unsigned long translate(char *value) {
    unsigned long unit;
    size_t len = strlen(value);

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
	    return atol(value);
    }
    return (atol(value) * unit);
}

static unsigned long hash_code(register const unsigned char *key) {
    register unsigned long hash = 0;
    register size_t i, len = strlen((char *)key);

    for (i = 0; i < len; i++) {
	hash += key[i];
	hash += (hash << 10);
	hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static int cache_init(void) {

    if (!(cache = calloc(1, hash_size(HASH_POWER) * sizeof(void *)))) return 0;
    return 1;
}

static void cache_destroy(void) {
    unsigned long i, size = hash_size(HASH_POWER);
    cache_item *it, *it_next;

    for (i = 0; i < size; i++) {
	it = cache[i];
	while (it) {
	    it_next = it->next;
	    SAFE_FREE(it->item);
	    SAFE_FREE(it);
	    it = it_next;
	}
    }
    SAFE_FREE(cache);
}

static SPF_result_t cache_get(const char *key) {
    unsigned long hash = hash_code((unsigned char*) key);
    cache_item *it = cache[hash & hash_mask(HASH_POWER)];
    time_t curtime = time(NULL);

    while (it) {
	if (it->hash == hash && it->exptime > curtime && it->item && !strcmp(key, it->item)) return it->status;
	it = it->next;
    }
    return SPF_RESULT_INVALID;
}

static void cache_put(const char *key, unsigned long ttl, SPF_result_t status) {
    unsigned long hash = hash_code((unsigned char*) key);
    time_t curtime = time(NULL);
    cache_item *it, *parent = NULL;

    it = cache[hash & hash_mask(HASH_POWER)];
    while (it) {
	if (it->hash == hash && it->exptime > curtime && it->item && !strcmp(key, it->item)) return;
	it = it->next;
    }
    it = cache[hash & hash_mask(HASH_POWER)];
    while (it) {
	if (it->exptime < curtime) {
	    SAFE_FREE(it->item);
	    it->item = strdup(key);
	    it->hash = hash;
	    it->status = status;
	    it->exptime = curtime + ttl;
	    return;
	}
	parent = it;
	it = it->next;
    }
    if ((it = (cache_item *) calloc(1, sizeof(cache_item)))) {
	it->item = strdup(key);
	it->hash = hash;
	it->status = status;
	it->exptime = curtime + ttl;
	if (parent)
	    parent->next = it;
	else
	    cache[hash & hash_mask(HASH_POWER)] = it;
    }
}

static void free_config(void) {

    SAFE_FREE(conf.tag);
    SAFE_FREE(conf.quarantine_box);
    SAFE_FREE(conf.run_as_user);
    SAFE_FREE(conf.sendmail_socket);
    SAFE_FREE(conf.fixed_ip);
    SAFE_FREE(conf.reject_reason);
    if (conf.log_file != NULL)
		fclose(conf.log_file);

    if (conf.ipnats) {
	IPNAT *it = conf.ipnats, *it_next;
		while (it) {
			it_next = it->next;
			SAFE_FREE(it);
			it = it_next;
		}
    }
    if (conf.cidrs) {
	CIDR *it = conf.cidrs, *it_next;

	while (it) {
	    it_next = it->next;
	    SAFE_FREE(it);
	    it = it_next;
	}
    }
    if (conf.ptrs) {
	STR *it = conf.ptrs, *it_next;

	while (it) {
	    it_next = it->next;
	    SAFE_FREE(it->str);
	    SAFE_FREE(it);
	    it = it_next;
	}
    }
    if (conf.froms) {
	STR *it = conf.froms, *it_next;

	while (it) {
	    it_next = it->next;
	    SAFE_FREE(it->str);
	    SAFE_FREE(it);
	    it = it_next;
	}
    }
    if (conf.tos) {
	STR *it = conf.tos, *it_next;

	while (it) {
	    it_next = it->next;
	    SAFE_FREE(it->str);
	    SAFE_FREE(it);
	    it = it_next;
	}
    }
}


static int load_config(void) {
    FILE *fp;
    char buf[2 * MAXLINE];

    conf.tag = strdup(TAG_STRING);
    conf.quarantine_box = strdup(QUARANTINE_BOX);
    conf.log_file = NULL;
    conf.fixed_ip = NULL;
    conf.reject_reason = strdup(REJECT_REASON);
    conf.run_as_user = strdup(USER);
    conf.sendmail_socket = strdup(OCONN);
    conf.syslog_facility = SYSLOG_FACILITY;
    conf.refuse_fail = REFUSE_FAIL;
    conf.relaxed_localpart = RELAXED_LOCALPART;
    conf.refuse_none = REFUSE_NONE;
    conf.refuse_none_helo = REFUSE_NONE_HELO;
    conf.soft_fail = SOFT_FAIL;
    conf.accept_temperror = ACCEPT_PERMERR;
    conf.tag_subject = TAG_SUBJECT;
    conf.add_header = ADD_HEADER;
    conf.add_recv_spf_header = ADD_RECV_HEADER;
    conf.quarantine = QUARANTINE;
    conf.spf_ttl = SPF_TTL;
    conf.daemonize = DAEMONIZE;
    conf.skip_auth = SKIP_AUTH;
    conf.skip_ndr = SKIP_NDR;
    if (!(fp = fopen(config_file, "r"))) return 0;
    while (fgets(buf, sizeof(buf) - 1, fp)) {
	char key[MAXLINE];
	char val[MAXLINE];
	char value[MAXLINE];
	char *p = NULL;

	if ((p = strchr(buf, '#'))) *p = '\0';
	if (!(strlen(buf))) continue;
	if (sscanf(buf, "%127s %[^\n]s", key, value) != 2) continue;
	strcpy(val , trim_space(value));
	if (!strcasecmp(key, "whitelistip")) {
	    char *slash = NULL;
	    unsigned short int mask = 32;

	    if ((slash = strchr(val, '/'))) {
		*slash = '\0';
		if ((mask = atoi(++slash)) > 32) mask = 32;
	    }
	    if (val[0] && !regexec(&re_ipv4, val, 0, NULL, 0)) {
		CIDR *it = NULL;
		unsigned long ip;

		if ((ip = inet_addr(val)) == 0xffffffff) continue;
		if (!conf.cidrs)
		    conf.cidrs = (CIDR *) calloc(1, sizeof(CIDR));
		else
		    if ((it = (CIDR *) calloc(1, sizeof(CIDR)))) {
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
	if (!strcasecmp(key, "clientipnat")) {
		char *sep = NULL;
		unsigned long d_ip;

	    if ((sep = strchr(val, ':'))) {
			*sep++ = '\0';
			if (*sep && !regexec(&re_ipv4, sep, 0, NULL, 0)) {
				if ((d_ip = inet_addr(sep)) == 0xffffffff) {
					log_message(LOG_ERR, "[CONFIG ERROR] ignore nat dest error:%s (entry:%s)", sep, buf);
					continue;
				} 
			} else {
				log_message(LOG_ERR, "[CONFIG ERROR] invalid destination ip (%s)", sep);
				continue;
			}
	    } else {
			log_message(LOG_ERR, "[CONFIG ERROR] invalid entry(%s). Must be src_ip:dest:ip", val);
			continue;
		}
	    if (val[0] && !regexec(&re_ipv4, val, 0, NULL, 0)) {
			IPNAT *it = NULL;
			unsigned long s_ip;

			if ((s_ip = inet_addr(val)) == 0xffffffff) {
					log_message(LOG_ERR, "[CONFIG ERROR] ignore nat src error:%s (entry:%s/int:%li)", val, buf,s_ip);
					continue;
			} 
			if (!conf.ipnats)
				conf.ipnats = (IPNAT *) calloc(1, sizeof(IPNAT));
			else
				if ((it = (IPNAT *) calloc(1, sizeof(IPNAT)))) {
					it->next = conf.ipnats;
					conf.ipnats = it;
				}
			if (conf.ipnats) {
				conf.ipnats->srcip = s_ip;
				conf.ipnats->destip = d_ip;
			}
		} else 
			log_message(LOG_ERR, "[CONFIG ERROR] entry(%s) is not a valid IP address", val);
	    continue;
	}
	if (!strcasecmp(key, "whitelistptr")) {
	    STR *it = NULL;

	    if (!conf.ptrs)
		conf.ptrs = (STR *) calloc(1, sizeof(STR));
	    else
		if ((it = (STR *) calloc(1, sizeof(STR)))) {
		    it->next = conf.ptrs;
		    conf.ptrs = it;
		}
	    if (conf.ptrs && !conf.ptrs->str) conf.ptrs->str = strdup(val);
	    continue;
	}
	if (!strcasecmp(key, "whitelistfrom")) {
	    STR *it = NULL;

	    if (!conf.froms)
		conf.froms = (STR *) calloc(1, sizeof(STR));
	    else
		if ((it = (STR *) calloc(1, sizeof(STR)))) {
		    it->next = conf.froms;
		    conf.froms = it;
		}
	    if (conf.froms && !conf.froms->str) {
		strtolower(val);
		conf.froms->str = strdup(val);
	    }
	    continue;
	}
	if (!strcasecmp(key, "whitelistto")) {
	    STR *it = NULL;

	    if (!conf.tos)
		conf.tos = (STR *) calloc(1, sizeof(STR));
	    else
		if ((it = (STR *) calloc(1, sizeof(STR)))) {
		    it->next = conf.tos;
		    conf.tos = it;
		}
	    if (conf.tos && !conf.tos->str) {
		strtolower(val);
		conf.tos->str = strdup(val);
	    }
	    continue;
	}
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
	    conf.relaxed_localpart= 1;
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
	if (!strcasecmp(key, "tag")) {
	    SAFE_FREE(conf.tag);
	    conf.tag = strdup(val);
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
	if (!strcasecmp(key, "fixedclientip")) {
	    conf.fixed_ip = strdup(val);
	    continue;
	}
	if (!strcasecmp(key, "rejectreason")) {
	    SAFE_FREE(conf.reject_reason);
	    conf.reject_reason = strdup(val);
	    continue;
	}
	if (!strcasecmp(key, "logto")) {
			if (!(conf.log_file = fopen(val,"a"))){
				fprintf (stderr,"Error: Can't open file %s to write. (errno: %d - %s)\n",
					val,errno,strerror(errno));
				return 0;
			}
	    continue;
	}
	if (!strcasecmp(key, "quarantinebox")) {
	    SAFE_FREE(conf.quarantine_box);
	    conf.quarantine_box = strdup(val);
	    continue;
	}
	if (!strcasecmp(key, "authservid")) {
	    authserv_id = strdup(val);
	    continue;
	}
	if (!strcasecmp(key, "ttl")) {
	    conf.spf_ttl = translate(val);
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
	if (!strcasecmp(key, "syslog")) {
	    int i;
		if (!strcasecmp(val, "none")) 
		    conf.syslog_facility = SYSLOG_DISABLE;
		else
		    for (i = 0; i < FACILITIES_AMOUNT; i++)
			if (!strcasecmp(val, syslog_facilities[i].name))
			    conf.syslog_facility = syslog_facilities[i].facility;
		    continue;
	}
    }
    fclose(fp);
    return 1;
}

static int ip_cidr(const unsigned long ip, const short int mask, const unsigned long checkip) {
    unsigned long ipaddr = 0;
    unsigned long cidrip = 0;
    unsigned long subnet = 0;

    subnet = ~0;
    subnet = subnet << (32 - mask);
    cidrip = htonl(ip) & subnet;
    ipaddr = ntohl(checkip) & subnet;
    if (cidrip == ipaddr) return 1;
    return 0;
}

static int ip_check(const unsigned long checkip) {
    CIDR *it = conf.cidrs;

    while (it) {
	if (ip_cidr(it->ip, it->mask, checkip)) return 1;
	it = it->next;
    }
    return 0;
}

static unsigned long natip_check(const unsigned long checkip) {
    IPNAT *it = conf.ipnats;
    while (it) {
		if (it->srcip == checkip) return it->destip;
		it = it->next;
    }
    return 0;
}

static int ptr_check(const char *ptr) {
    STR *it = conf.ptrs;

    while (it) {
	if (it->str && strlen(it->str) <= strlen(ptr) && !strcasecmp(ptr + strlen(ptr) - strlen(it->str), it->str)) return 1;
	it = it->next;
    }
    return 0;
}

static int from_check(const char *from) {
    STR *it = conf.froms;

    while (it) {
	if (it->str && strstr(from, it->str)) return 1;
	it = it->next;
    }
    return 0;
}

static int to_check(const char *to) {
    STR *it = conf.tos;

    while (it) {
	if (it->str && strstr(to, it->str)) return 1;
	it = it->next;
    }
    return 0;
}
static char * trim_space(char *str) {
    char *end;
    while (isspace(*str)) { // skip leading whitespace
        str = str + 1;
    }
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) { // remove trailing whitespace
        end = end - 1;
    }

    *(end+1) = '\0'; //  write null character*/
    return str;
}
// LCOV_EXCL_START
static void die(const char *reason) {

    log_message(LOG_ERR, "[ERROR] die: %s", reason);
    smfi_stop();
    sleep(60);
    abort();
}
// LCOV_EXCL_STOP

static void mutex_lock(pthread_mutex_t *mutex) {

    if (pthread_mutex_lock(mutex)) die("pthread_mutex_lock");
}

static void mutex_unlock(pthread_mutex_t *mutex) {

    if (pthread_mutex_unlock(mutex)) die("pthread_mutex_unlock");
}

static int address_preparation(register char *dst, register const char *src) {
    register const char *start = NULL, *stop = NULL, *local = NULL;
    int tail;

    if (!(start = strchr(src, '<'))) return 0;
    if (!(stop = strrchr(src, '>'))) return 0;
    if (++start >= --stop) return 0;
    strscpy(dst, start, stop - start + 1);
    tail = strlen(dst) - 1;
    if ((dst[0] >= 0x07 && dst[0] <= 0x0d) || dst[0] == 0x20) return 0;
    if ((dst[tail] >= 0x07 && dst[tail] <= 0x0d) || dst[tail] == 0x20) return 0;
    local = strchr(start, '@');
    if (!local) return 0;
    if (!conf.relaxed_localpart && ((local - start) > MAXLOCALPART)) return 0;
    return 1;
}

static void add_rcpt(struct context *context) {
    STR *it = NULL;

    if (!context->rcpts)
	context->rcpts = (STR *) calloc(1, sizeof(STR));
    else
	if ((it = (STR *) calloc(1, sizeof(STR)))) {
	    it->next = context->rcpts;
	    context->rcpts = it;
	}
    if (context->rcpts && !context->rcpts->str) context->rcpts->str = strdup(context->rcpt);
}

static sfsistat smf_connect(SMFICTX *ctx, char *name, _SOCK_ADDR *sa) {
    struct context *context = NULL;
    char host[64];
	unsigned long int d_ip;

    if (authserv_id == NULL) {
        char* p = NULL;
        if (((p = smfi_getsymval(ctx, "{j}"))) == NULL) {
            log_message(LOG_ERR, "[ERROR] can't get MTA-name");
            authserv_id = strdup(DEFHOSTNAME);
        } else if ((authserv_id = strdup(p)) == NULL) {
            log_message(LOG_ERR, "[ERROR] can't save MTA-name"); // LCOV_EXCL_LINE
            return SMFIS_ACCEPT; // LCOV_EXCL_LINE
        }
    }

    if (sa == NULL)
    {
        log_message(LOG_NOTICE, "unknown sender IP address, skipping SPF check");
        return SMFIS_ACCEPT;
    }

    strscpy(host, "undefined", sizeof(host) - 1);
    switch (sa->sa_family) {
	case AF_INET: {
	    struct sockaddr_in *sin = (struct sockaddr_in *)sa;

	    inet_ntop(AF_INET, &sin->sin_addr.s_addr, host, sizeof(host));
	    break;
	}
	case AF_INET6: {
	    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

	    inet_ntop(AF_INET6, &sin6->sin6_addr, host, sizeof(host));
	    break;
	}
    }
    if (conf.cidrs && ip_check(inet_addr(host))) return SMFIS_ACCEPT;
    if (conf.ptrs && ptr_check(name)) return SMFIS_ACCEPT;
    if (!(context = calloc(1, sizeof(*context)))) {
			log_message(LOG_ERR, "[ERROR] %s", strerror(errno)); // LCOV_EXCL_LINE
			return SMFIS_ACCEPT; // LCOV_EXCL_LINE
    }
    smfi_setpriv(ctx, context);
    if (conf.ipnats && (d_ip = natip_check(inet_addr(host)))) {
		snprintf(context->addr, sizeof(context->addr), "%li.%li.%li.%li",
			d_ip & 0xff, d_ip >> 8 & 0xff, d_ip >> 16 & 0xff, d_ip >> 24 & 0xff);
        log_message(LOG_INFO, "Found  NAT IP address. Original: %s . Final %li (%s)", host, d_ip,context->addr);
	} else if (conf.fixed_ip) 
            strscpy(context->addr, conf.fixed_ip, sizeof(context->addr) - 1);
    else
            strscpy(context->addr, host, sizeof(context->addr) - 1);
    strscpy(context->fqdn, name, sizeof(context->fqdn) - 1);
    strscpy(context->helo, "undefined", sizeof(context->helo) - 1);
    return SMFIS_CONTINUE;
}

static sfsistat smf_helo(SMFICTX *ctx, char *arg) {
    struct context *context = (struct context *)smfi_getpriv(ctx);

    strscpy(context->helo, arg, sizeof(context->helo) - 1);
    return SMFIS_CONTINUE;
}

static sfsistat smf_envfrom(SMFICTX *ctx, char **args) {
    struct context *context = (struct context *)smfi_getpriv(ctx);
    const char *verify = smfi_getsymval(ctx, "{verify}");
    const char *site = NULL;
    SPF_server_t *spf_server = NULL;
    SPF_request_t *spf_request = NULL;
    SPF_response_t *spf_response = NULL;
    SPF_result_t status;

    if (verify && strcmp(verify, "OK") == 0) return SMFIS_ACCEPT;
    if (*args) strscpy(context->from, *args, sizeof(context->from) - 1);
    if (strstr(context->from, "<>")) {
		if (conf.skip_ndr) {
			log_message(LOG_INFO, "SPF skip : empty sender, helo=%s, ip=%s",context->helo,context->addr);
			return SMFIS_ACCEPT;
		}
	strtolower(context->helo);
	snprintf(context->sender, sizeof(context->sender), "postmaster@%s", context->helo);
    } else if (!address_preparation(context->sender, context->from)) {
        if (conf.soft_fail) {
                smfi_setreply(ctx, "450", "4.1.7", "Sender address does not conform to RFC-2821 syntax");
                return SMFIS_TEMPFAIL;
        } else {
                smfi_setreply(ctx, "550", "5.1.7", "Sender address does not conform to RFC-2821 syntax");
                return SMFIS_REJECT;
        }
	}
    if ((conf.skip_auth) && (smfi_getsymval(ctx, "{auth_authen}"))){
	    log_message(LOG_INFO, "SPF skip : username %s, from=%s", smfi_getsymval(ctx, "{auth_authen}"), context->from);
		return SMFIS_ACCEPT;
	}
    if (!strstr(context->from, "<>")) {
	strtolower(context->sender);
	if (conf.froms && from_check(context->sender)) return SMFIS_ACCEPT;
    }
    SAFE_FREE(context->rcpts);
    SAFE_FREE(context->subject);
    context->status = SPF_RESULT_NONE;
    if ((site = smfi_getsymval(ctx, "j")))
	strscpy(context->site, site, sizeof(context->site) - 1);
    else
	strscpy(context->site, "localhost", sizeof(context->site) - 1);
    snprintf(context->key, sizeof(context->key), "%s|%s", context->addr, strchr(context->sender, '@') + 1);
    if (cache && conf.spf_ttl) {
	mutex_lock(&cache_mutex);
	status = cache_get(context->key);
	mutex_unlock(&cache_mutex);
	if (status != SPF_RESULT_INVALID) {
	    log_message(LOG_INFO, "SPF %s (cached): ip=%s, fqdn=%s, helo=%s, from=%s", SPF_strresult(status), context->addr, context->fqdn, context->helo, context->from);
	    if (status == SPF_RESULT_FAIL && conf.refuse_fail && !conf.tos) {
		char reject[2 * MAXLINE];

		snprintf(reject, sizeof(reject), conf.reject_reason, context->sender, context->addr, context->site);
        if (conf.soft_fail) {
                smfi_setreply(ctx, "450", "4.7.23", reject);
                return SMFIS_TEMPFAIL;
        } else {
                smfi_setreply(ctx, "550", "5.7.23", reject);
                return SMFIS_REJECT;
        }
	    }
	    context->status = status;
	    return SMFIS_CONTINUE;
	}
    }
    if (!(spf_server = SPF_server_new(SPF_DNS_RESOLV, 0))) {
	log_message(LOG_ERR, "[ERROR] SPF engine init failed"); // LCOV_EXCL_LINE
	return SMFIS_ACCEPT; // LCOV_EXCL_LINE
    }
    SPF_server_set_rec_dom(spf_server, context->site);
    if (!(spf_request = SPF_request_new(spf_server))) goto done;
    SPF_request_set_ipv4_str(spf_request, context->addr);
    SPF_request_set_ipv6_str(spf_request, context->addr);
    SPF_request_set_helo_dom(spf_request, context->helo);
    SPF_request_set_env_from(spf_request, context->sender);
    if (SPF_request_query_mailfrom(spf_request, &spf_response)) {
	log_message(LOG_INFO, "SPF none: ip=%s, fqdn=%s, helo=%s, from=%s", context->addr, context->fqdn, context->helo, context->from);
    if ((status == SPF_RESULT_NONE) || (status == SPF_RESULT_INVALID)) {
            if (conf.refuse_none && !strstr(context->from, "<>")) {
                    char reject[2 * MAXLINE];
                    snprintf(reject, sizeof(reject), "Sorry %s, we only accept mail from SPF enabled domains.", context->sender);
                    if (spf_response) SPF_response_free(spf_response);
                    if (spf_request) SPF_request_free(spf_request);
                    if (spf_server) SPF_server_free(spf_server);
                    smfi_setreply(ctx, "550" , "5.7.1", reject);
                    return SMFIS_REJECT;
            }
            if (conf.refuse_none_helo && strstr(context->from, "<>")) {
                    char reject[2 * MAXLINE];
                    snprintf(reject, sizeof(reject), "Sorry %s, we only accept empty senders from enabled servers (HELO identity)", context->sender);
                    if (spf_response) SPF_response_free(spf_response);
                    if (spf_request) SPF_request_free(spf_request);
                    if (spf_server) SPF_server_free(spf_server);
                    smfi_setreply(ctx, "550" , "5.7.1", reject);
                    return SMFIS_REJECT;
            }
    }
	if (cache && conf.spf_ttl) {
	    mutex_lock(&cache_mutex);
	    cache_put(context->key, conf.spf_ttl, SPF_RESULT_NONE);
	    mutex_unlock(&cache_mutex);
	}
	goto done;
    }
    if (!spf_response) goto done;
    status = SPF_response_result(spf_response);
    log_message(LOG_NOTICE, "SPF %s: ip=%s, fqdn=%s, helo=%s, from=%s", SPF_strresult(status), context->addr, context->fqdn, context->helo, context->from);
    switch (status) {
	case SPF_RESULT_PASS:
	case SPF_RESULT_FAIL:
	case SPF_RESULT_SOFTFAIL:
	case SPF_RESULT_NEUTRAL:
	    context->status = status;
	    if (cache && conf.spf_ttl) {
		mutex_lock(&cache_mutex);
		cache_put(context->key, conf.spf_ttl, context->status);
		mutex_unlock(&cache_mutex);
	    }
	    break;
	default:
	    break;
    }
    if (status == SPF_RESULT_TEMPERROR && !conf.accept_temperror) {
		char reject[2 * MAXLINE];
		if (spf_response) {
			snprintf(reject, sizeof(reject), "Found a problem processing SFP for %s. Error: (no reason)", context->sender);
		} else {
			snprintf(reject, sizeof(reject), "Found a problem processing SFP for %s. Error: %s", context->sender,  SPF_strreason(spf_response->reason));
		}
		if (spf_response) SPF_response_free(spf_response);
		if (spf_request) SPF_request_free(spf_request);
		if (spf_server) SPF_server_free(spf_server);
		smfi_setreply(ctx, "451" , "4.4.3", reject);
		return SMFIS_TEMPFAIL;
	}
    if (status == SPF_RESULT_FAIL && conf.refuse_fail && !conf.tos) {
	char reject[2 * MAXLINE];

	snprintf(reject, sizeof(reject), conf.reject_reason, context->sender, context->addr, context->site);
	if (spf_response) SPF_response_free(spf_response);
	if (spf_request) SPF_request_free(spf_request);
	if (spf_server) SPF_server_free(spf_server);
    if (conf.soft_fail) {
            smfi_setreply(ctx, "450", "4.7.23", reject);
            return SMFIS_TEMPFAIL;
    } else {
            smfi_setreply(ctx, "550", "5.7.23", reject);
            return SMFIS_REJECT;
    }
    }
done:
    if (spf_response) SPF_response_free(spf_response);
    if (spf_request) SPF_request_free(spf_request);
    if (spf_server) SPF_server_free(spf_server);
    return SMFIS_CONTINUE;
}

static sfsistat smf_envrcpt(SMFICTX *ctx, char **args) {
    struct context *context = (struct context *)smfi_getpriv(ctx);

    if (*args) strscpy(context->rcpt, *args, sizeof(context->rcpt) - 1);
    if (!address_preparation(context->recipient, context->rcpt)) {
    if (conf.soft_fail) {
            smfi_setreply(ctx, "450", "4.1.3", "Recipient address does not conform to RFC-2821 syntax");
            return SMFIS_TEMPFAIL;
    } else {
            smfi_setreply(ctx, "550", "5.1.3", "Recipient address does not conform to RFC-2821 syntax");
            return SMFIS_REJECT;
    }
    }
    if (conf.tos) {
	strtolower(context->recipient);
	if (to_check(context->recipient)) return SMFIS_ACCEPT;
	if (context->status == SPF_RESULT_FAIL && conf.refuse_fail) {
	    char reject[2 * MAXLINE];

	    snprintf(reject, sizeof(reject), conf.reject_reason, context->sender, context->addr, context->site);
        if (conf.soft_fail) {
                smfi_setreply(ctx, "450", "4.1.1", reject);
                return SMFIS_TEMPFAIL;
        } else {
                smfi_setreply(ctx, "550", "5.1.1", reject);
                return SMFIS_REJECT;
        }
    }
    }
    if (conf.quarantine && (context->status == SPF_RESULT_FAIL || context->status == SPF_RESULT_SOFTFAIL)) add_rcpt(context);
    return SMFIS_CONTINUE;
}

static sfsistat smf_header(SMFICTX *ctx, char *name, char *value) {
    struct context *context = (struct context *)smfi_getpriv(ctx);

    if (!strcasecmp(name, "Subject") && (context->status == SPF_RESULT_FAIL || context->status == SPF_RESULT_SOFTFAIL) && conf.tag_subject && !context->subject) context->subject = strdup(value);
    return SMFIS_CONTINUE;
}

static sfsistat smf_eom(SMFICTX *ctx) {
    struct context *context = (struct context *)smfi_getpriv(ctx);

    if ((context->status == SPF_RESULT_FAIL || context->status == SPF_RESULT_SOFTFAIL) && conf.tag_subject) {
	char *subj = NULL;

	if (context->subject) {
	    size_t len = strlen(context->subject) + strlen(conf.tag) + 2;

	    if ((subj = calloc(1, len))) snprintf(subj, len, "%s %s", conf.tag, context->subject);
	}
	else {
	    size_t len = strlen(conf.tag) + 1;

	    if ((subj = calloc(1, len))) snprintf(subj, len, "%s", conf.tag);
	}
	if (subj) {
	    if (context->subject)
		smfi_chgheader(ctx, "Subject", 1, subj);
	    else
		smfi_addheader(ctx, "Subject", subj);
	    free(subj);
	}
    }
    if (conf.add_header) {
	char *spf_hdr = NULL;

	if ((spf_hdr = calloc(1, MAX_HEADER_SIZE))) {
	    switch (context->status) {
		case SPF_RESULT_PASS:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "%s; spf=%s smtp.mailfrom=%s smtp.helo=%s",
			authserv_id, "pass", context->sender, context->helo);
		    break;
		case SPF_RESULT_FAIL:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "%s; spf=%s smtp.mailfrom=%s smtp.helo=%s",
			authserv_id, "fail", context->sender, context->helo);
		    break;
		case SPF_RESULT_SOFTFAIL:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "%s; spf=%s smtp.mailfrom=%s smtp.helo=%s",
			authserv_id, "softfail", context->sender, context->helo);
		    break;
		case SPF_RESULT_NEUTRAL:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "%s; spf=%s smtp.mailfrom=%s smtp.helo=%s",
			authserv_id, "neutral", context->sender, context->helo);
		    break;
		case SPF_RESULT_NONE:
		default:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "%s; spf=%s smtp.mailfrom=%s smtp.helo=%s",
			authserv_id, "none", context->sender, context->helo);
		    break;
	    }
	    smfi_insheader(ctx, 1, "Authentication-Results", spf_hdr);
	    free(spf_hdr);
	}
    }


    if (conf.add_recv_spf_header) {
	char *spf_hdr = NULL;

	if ((spf_hdr = calloc(1, MAX_HEADER_SIZE))) {
	    switch (context->status) {
		case SPF_RESULT_PASS:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "Pass (%s: domain of %s\n\tdesignates %s as permitted sender)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->addr, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_FAIL:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "Fail (%s: domain of %s\n\tdoes not designate %s as permitted sender)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->addr, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_SOFTFAIL:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "SoftFail (%s: transitioning domain of %s\n\tdoes not designate %s as permitted sender)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->addr, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_NEUTRAL:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "Neutral (%s: %s is neither permitted\n\tnor denied by domain of %s)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->addr, context->sender, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_NONE:
		default:
		    snprintf(spf_hdr, MAX_HEADER_SIZE, "None (%s: domain of %s\n\tdoes not designate permitted sender hosts)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->site, context->addr, context->from, context->helo);
		    break;
	    }
	    smfi_addheader(ctx, "Received-SPF", spf_hdr);
	    free(spf_hdr);
	}
    }
    if (context->rcpts) {
	STR *it = context->rcpts;

	while (it) {
	    if (it->str) {
		smfi_delrcpt(ctx, it->str);
		smfi_addheader(ctx, "X-SPF-Original-To", it->str);
	    }
	    it = it->next;
	}
	smfi_addrcpt(ctx, conf.quarantine_box);
    }
    return SMFIS_CONTINUE;
}

static sfsistat smf_close(SMFICTX *ctx) {
    struct context *context = (struct context *)smfi_getpriv(ctx);

    if (context) {
	if (context->rcpts) {
	    STR *it = context->rcpts, *it_next;

	    while (it) {
		it_next = it->next;
		SAFE_FREE(it->str);
		SAFE_FREE(it);
		it = it_next;
	    }
	}
	SAFE_FREE(context->subject);
	free(context);
	smfi_setpriv(ctx, NULL);
    }
    return SMFIS_CONTINUE;
}

struct smfiDesc smfilter = {
    "smf-spf",
    SMFI_VERSION,
    SMFIF_ADDHDRS|SMFIF_CHGHDRS|SMFIF_ADDRCPT|SMFIF_DELRCPT,
    smf_connect,
    smf_helo,
    smf_envfrom,
    smf_envrcpt,
    smf_header,
    NULL,
    NULL,
    smf_eom,
    NULL,
    smf_close
};

int main(int argc, char **argv) {
    const char *ofile = NULL;
    int ch, ret = 0;
	char *strHelper;
	strHelper = argv[0];
	if (strHelper[(strlen(strHelper) - 1)] == '/')
        strHelper[(strlen(strHelper) - 1)] = '\0';

    (daemon_name = strrchr(strHelper, '/')) ? ++daemon_name : (daemon_name = strHelper);
	gethostname(hostname,HOST_NAME_MAX+1);

    while ((ch = getopt(argc, argv, "fhc:")) != -1) {
	switch (ch) {
	    case 'h':
		fprintf(stderr, "Usage: smf-spf [-f] -c <config file>\n");
		return 0;
	    case 'f':
		foreground = 1;
		break;
	    case 'c':
		if (optarg) config_file = optarg;
		break;
	    default:
		break;
	}
    }
    memset(&conf, 0, sizeof(conf));
    regcomp(&re_ipv4, IPV4_DOT_DECIMAL, REG_EXTENDED|REG_ICASE);
    if (!load_config()) { 
		fprintf(stderr, "Warning: smf-spf: loading configuration file %s failed\n", config_file);
		goto done;
	}
    tzset();
	log_init();
    log_message(LOG_INFO, "starting %s %s listening on %s", daemon_name, VERSION, conf.sendmail_socket);
    if (!strncmp(conf.sendmail_socket, "unix:", 5))
	ofile = conf.sendmail_socket + 5;
    else
	if (!strncmp(conf.sendmail_socket, "local:", 6)) ofile = conf.sendmail_socket + 6;
    if (ofile) unlink(ofile);
    if (!getuid()) {
	struct passwd *pw;

	if ((pw = getpwnam(conf.run_as_user)) == NULL) {
	    fprintf(stderr, "getpwnam %s: %s\n", conf.run_as_user, errno ? strerror(errno) : "User does not exists");
	    goto done;
	}
	// LCOV_EXCL_START
	setgroups(1, &pw->pw_gid);
	if (setgid(pw->pw_gid)) {								
	    fprintf(stderr, "setgid: %s\n", strerror(errno));	
	    goto done;
	}
	if (setuid(pw->pw_uid)) { 								
	    fprintf(stderr, "setuid: %s\n", strerror(errno));	
	    goto done;
	}
        log_message(LOG_INFO, "running as uid: %d, gid: %d", (int) pw->pw_uid, (int) pw->pw_gid);
    }
    if (smfi_setconn((char *)conf.sendmail_socket) != MI_SUCCESS) {
	fprintf(stderr, "smfi_setconn failed: %s\n", conf.sendmail_socket);
	goto done;
    }
    if (smfi_register(smfilter) != MI_SUCCESS) {
	fprintf(stderr, "smfi_register failed\n"); 
	goto done;
    }
    if (!foreground && conf.daemonize && daemon(0, 0)) {
	fprintf(stderr, "daemonize failed: %s\n", strerror(errno)); 
	goto done;
    }
    if (pthread_mutex_init(&cache_mutex, 0)) {
	fprintf(stderr, "pthread_mutex_init failed\n");
	goto done;
    }
	// LCOV_EXCL_END
    umask(0177);
    if (conf.spf_ttl && !cache_init()) log_message(LOG_ERR, "[ERROR] cache engine init failed");
    ret = smfi_main();
    if (ret != MI_SUCCESS) log_message(LOG_ERR, "[ERROR] terminated due to a fatal error");
    else log_message(LOG_NOTICE, "stopping %s %s listening on %s", daemon_name, VERSION, conf.sendmail_socket);
    if (cache) cache_destroy();
    pthread_mutex_destroy(&cache_mutex);
done:
    free_config();
    closelog();
    return ret;
}

