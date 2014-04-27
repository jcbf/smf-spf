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
#include "spf2/spf.h"

#define CONFIG_FILE		"/etc/mail/smfs/smf-spf.conf"
#define WORK_SPACE		"/var/run/smfs"
#define OCONN			"unix:" WORK_SPACE "/smf-spf.sock"
#define USER			"smfs"
#define TAG_STRING		"[SPF:fail]"
#define QUARANTINE_BOX		"postmaster"
#define SYSLOG_FACILITY		LOG_MAIL
#define SPF_TTL			3600
#define REFUSE_FAIL		1
#define TAG_SUBJECT		1
#define ADD_HEADER		1
#define QUARANTINE		0

#define MAXLINE			128
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

typedef struct STR {
    char *str;
    struct STR *next;
} STR;

typedef struct config {
    char *tag;
    char *quarantine_box;
    char *run_as_user;
    char *sendmail_socket;
    CIDR *cidrs;
    STR *ptrs;
    STR *froms;
    STR *tos;
    int refuse_fail;
    int tag_subject;
    int add_header;
    int quarantine;
    int syslog_facility;
    unsigned long spf_ttl;
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
    char sender[MAXLINE];
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
static config conf;
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

static sfsistat smf_connect(SMFICTX *, char *, _SOCK_ADDR *);
static sfsistat smf_helo(SMFICTX *, char *);
static sfsistat smf_envfrom(SMFICTX *, char **);
static sfsistat smf_envrcpt(SMFICTX *, char **);
static sfsistat smf_header(SMFICTX *, char *, char *);
static sfsistat smf_eom(SMFICTX *);
static sfsistat smf_close(SMFICTX *);

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
    register size_t i, len = strlen(key);

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
    unsigned long hash = hash_code(key);
    cache_item *it = cache[hash & hash_mask(HASH_POWER)];
    time_t curtime = time(NULL);

    while (it) {
	if (it->hash == hash && it->exptime > curtime && it->item && !strcmp(key, it->item)) return it->status;
	it = it->next;
    }
    return SPF_RESULT_INVALID;
}

static void cache_put(const char *key, unsigned long ttl, SPF_result_t status) {
    unsigned long hash = hash_code(key);
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
    conf.run_as_user = strdup(USER);
    conf.sendmail_socket = strdup(OCONN);
    conf.syslog_facility = SYSLOG_FACILITY;
    conf.refuse_fail = REFUSE_FAIL;
    conf.tag_subject = TAG_SUBJECT;
    conf.add_header = ADD_HEADER;
    conf.quarantine = QUARANTINE;
    conf.spf_ttl = SPF_TTL;
    if (!(fp = fopen(config_file, "r"))) return 0;
    while (fgets(buf, sizeof(buf) - 1, fp)) {
	char key[MAXLINE];
	char val[MAXLINE];
	char *p = NULL;

	if ((p = strchr(buf, '#'))) *p = '\0';
	if (!(strlen(buf))) continue;
	if (sscanf(buf, "%127s %127s", key, val) != 2) continue;
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
	if (!strcasecmp(key, "quarantine") && !strcasecmp(val, "on")) {
	    conf.quarantine = 1;
	    continue;
	}
	if (!strcasecmp(key, "quarantinebox")) {
	    SAFE_FREE(conf.quarantine_box);
	    conf.quarantine_box = strdup(val);
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

static void die(const char *reason) {

    syslog(LOG_ERR, "[ERROR] die: %s", reason);
    smfi_stop();
    sleep(60);
    abort();
}

static void mutex_lock(pthread_mutex_t *mutex) {

    if (pthread_mutex_lock(mutex)) die("pthread_mutex_lock");
}

static void mutex_unlock(pthread_mutex_t *mutex) {

    if (pthread_mutex_unlock(mutex)) die("pthread_mutex_unlock");
}

static int address_preparation(register char *dst, register const char *src) {
    register const char *start = NULL, *stop = NULL;
    int tail;

    if (!(start = strchr(src, '<'))) return 0;
    if (!(stop = strrchr(src, '>'))) return 0;
    if (++start >= --stop) return 0;
    strscpy(dst, start, stop - start + 1);
    tail = strlen(dst) - 1;
    if ((dst[0] >= 0x07 && dst[0] <= 0x0d) || dst[0] == 0x20) return 0;
    if ((dst[tail] >= 0x07 && dst[tail] <= 0x0d) || dst[tail] == 0x20) return 0;
    if (!strchr(dst, '@')) return 0;
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
	syslog(LOG_ERR, "[ERROR] %s", strerror(errno));
	return SMFIS_ACCEPT;
    }
    smfi_setpriv(ctx, context);
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

    if (smfi_getsymval(ctx, "{auth_authen}")) return SMFIS_ACCEPT;
    if (verify && strcmp(verify, "OK") == 0) return SMFIS_ACCEPT;
    if (*args) strscpy(context->from, *args, sizeof(context->from) - 1);
    if (strstr(context->from, "<>")) {
	strtolower(context->helo);
	snprintf(context->sender, sizeof(context->sender), "postmaster@%s", context->helo);
    }
    else
	if (!address_preparation(context->sender, context->from)) {
	    smfi_setreply(ctx, "550", "5.1.7", "Sender address does not conform to RFC-2821 syntax");
	    return SMFIS_REJECT;
	}
    if (!strstr(context->from, "<>")) {
	strtolower(context->sender);
	if (conf.froms && from_check(context->sender)) return SMFIS_ACCEPT;
    }
    if (context->rcpts) {
	STR *it = context->rcpts, *it_next;

	while (it) {
	    it_next = it->next;
	    SAFE_FREE(it->str);
	    SAFE_FREE(it);
	    it = it_next;
	}
	context->rcpts = NULL;
    }
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
	    syslog(LOG_INFO, "SPF %s (cached): %s, %s, %s, %s", SPF_strresult(status), context->addr, context->fqdn, context->helo, context->from);
	    if (status == SPF_RESULT_FAIL && conf.refuse_fail && !conf.tos) {
		char reject[2 * MAXLINE];

		snprintf(reject, sizeof(reject), "Rejected, look at http://www.openspf.org/why.html?sender=%s&ip=%s&receiver=%s", context->sender, context->addr, context->site);
		smfi_setreply(ctx, "550", "5.7.1", reject);
		return SMFIS_REJECT;
	    }
	    context->status = status;
	    return SMFIS_CONTINUE;
	}
    }
    if (!(spf_server = SPF_server_new(SPF_DNS_RESOLV, 0))) {
	syslog(LOG_ERR, "[ERROR] SPF engine init failed");
	return SMFIS_ACCEPT;
    }
    SPF_server_set_rec_dom(spf_server, context->site);
    if (!(spf_request = SPF_request_new(spf_server))) goto done;
    SPF_request_set_ipv4_str(spf_request, context->addr);
    SPF_request_set_helo_dom(spf_request, context->helo);
    SPF_request_set_env_from(spf_request, context->sender);
    if (SPF_request_query_mailfrom(spf_request, &spf_response)) {
	syslog(LOG_INFO, "SPF none: %s, %s, %s, %s", context->addr, context->fqdn, context->helo, context->from);
	if (cache && conf.spf_ttl) {
	    mutex_lock(&cache_mutex);
	    cache_put(context->key, conf.spf_ttl, SPF_RESULT_NONE);
	    mutex_unlock(&cache_mutex);
	}
	goto done;
    }
    if (!spf_response) goto done;
    status = SPF_response_result(spf_response);
    syslog(LOG_NOTICE, "SPF %s: %s, %s, %s, %s", SPF_strresult(status), context->addr, context->fqdn, context->helo, context->from);
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
    if (status == SPF_RESULT_FAIL && conf.refuse_fail && !conf.tos) {
	char reject[2 * MAXLINE];

	snprintf(reject, sizeof(reject), "Rejected, look at http://www.openspf.org/why.html?sender=%s&ip=%s&receiver=%s", context->sender, context->addr, context->site);
	if (spf_response) SPF_response_free(spf_response);
	if (spf_request) SPF_request_free(spf_request);
	if (spf_server) SPF_server_free(spf_server);
	smfi_setreply(ctx, "550", "5.7.1", reject);
	return SMFIS_REJECT;
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
	smfi_setreply(ctx, "550", "5.1.3", "Recipient address does not conform to RFC-2821 syntax");
	return SMFIS_REJECT;
    }
    if (conf.tos) {
	strtolower(context->recipient);
	if (to_check(context->recipient)) return SMFIS_CONTINUE;
	if (context->status == SPF_RESULT_FAIL && conf.refuse_fail) {
	    char reject[2 * MAXLINE];

	    snprintf(reject, sizeof(reject), "Rejected, look at http://www.openspf.org/why.html?sender=%s&ip=%s&receiver=%s", context->sender, context->addr, context->site);
	    smfi_setreply(ctx, "550", "5.1.1", reject);
	    return SMFIS_REJECT;
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

	smfi_addheader(ctx, "X-SPF-Scan-By", "smf-spf v2.0.2 - http://smfs.sf.net/");
	if ((spf_hdr = calloc(1, 512))) {
	    switch (context->status) {
		case SPF_RESULT_PASS:
		    snprintf(spf_hdr, 512, "Pass (%s: domain of %s\n\tdesignates %s as permitted sender)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->addr, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_FAIL:
		    snprintf(spf_hdr, 512, "Fail (%s: domain of %s\n\tdoes not designate %s as permitted sender)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->addr, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_SOFTFAIL:
		    snprintf(spf_hdr, 512, "SoftFail (%s: transitioning domain of %s\n\tdoes not designate %s as permitted sender)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->sender, context->addr, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_NEUTRAL:
		    snprintf(spf_hdr, 512, "Neutral (%s: %s is neither permitted\n\tnor denied by domain of %s)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
			context->site, context->addr, context->sender, context->site, context->addr, context->from, context->helo);
		    break;
		case SPF_RESULT_NONE:
		default:
		    snprintf(spf_hdr, 512, "None (%s: domain of %s\n\tdoes not designate permitted sender hosts)\n\treceiver=%s; client-ip=%s;\n\tenvelope-from=%s; helo=%s;",
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

    while ((ch = getopt(argc, argv, "hc:")) != -1) {
	switch (ch) {
	    case 'h':
		fprintf(stderr, "Usage: smf-spf -c <config file>\n");
		return 0;
	    case 'c':
		if (optarg) config_file = optarg;
		break;
	    default:
		break;
	}
    }
    memset(&conf, 0, sizeof(conf));
    regcomp(&re_ipv4, IPV4_DOT_DECIMAL, REG_EXTENDED|REG_ICASE);
    if (!load_config()) fprintf(stderr, "Warning: smf-spf configuration file load failed\n");
    tzset();
    openlog("smf-spf", LOG_PID|LOG_NDELAY, conf.syslog_facility);
    if (!strncmp(conf.sendmail_socket, "unix:", 5))
	ofile = conf.sendmail_socket + 5;
    else
	if (!strncmp(conf.sendmail_socket, "local:", 6)) ofile = conf.sendmail_socket + 6;
    if (ofile) unlink(ofile);
    if (!getuid()) {
	struct passwd *pw;

	if ((pw = getpwnam(conf.run_as_user)) == NULL) {
	    fprintf(stderr, "%s: %s\n", conf.run_as_user, strerror(errno));
	    goto done;
	}
	setgroups(1, &pw->pw_gid);
	if (setgid(pw->pw_gid)) {
	    fprintf(stderr, "setgid: %s\n", strerror(errno));
	    goto done;
	}
	if (setuid(pw->pw_uid)) {
	    fprintf(stderr, "setuid: %s\n", strerror(errno));
	    goto done;
	}
    }
    if (smfi_setconn((char *)conf.sendmail_socket) != MI_SUCCESS) {
	fprintf(stderr, "smfi_setconn failed: %s\n", conf.sendmail_socket);
	goto done;
    }
    if (smfi_register(smfilter) != MI_SUCCESS) {
	fprintf(stderr, "smfi_register failed\n");
	goto done;
    }
    if (daemon(0, 0)) {
	fprintf(stderr, "daemonize failed: %s\n", strerror(errno));
	goto done;
    }
    if (pthread_mutex_init(&cache_mutex, 0)) {
	fprintf(stderr, "pthread_mutex_init failed\n");
	goto done;
    }
    umask(0177);
    if (conf.spf_ttl && !cache_init()) syslog(LOG_ERR, "[ERROR] cache engine init failed");
    ret = smfi_main();
    if (ret != MI_SUCCESS) syslog(LOG_ERR, "[ERROR] terminated due to a fatal error");
    if (cache) cache_destroy();
    pthread_mutex_destroy(&cache_mutex);
done:
    free_config();
    closelog();
    return ret;
}

