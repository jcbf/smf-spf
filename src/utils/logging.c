/*
 * logging.c - Logging abstraction for smf-spf
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

#include "logging.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Module state */
static const char *g_daemon_name = NULL;
static int g_syslog_facility = SYSLOG_DISABLE;
static FILE *g_log_file = NULL;
static const char *g_hostname = NULL;

void log_init(const char *daemon_name, int syslog_facility,
              FILE *log_file, const char *hostname) {
    g_daemon_name = daemon_name;
    g_syslog_facility = syslog_facility;
    g_log_file = log_file;
    g_hostname = hostname;

    if (g_syslog_facility != SYSLOG_DISABLE) {
        openlog(daemon_name, LOG_PID | LOG_NDELAY, syslog_facility);
    }
}

void log_message(int log_level, const char *fmt, ...) {
    va_list ap;

    /* Log to file if configured */
    if (g_log_file) {
        char time_str[32];
        struct tm *tm;
        time_t now = time(0);

        tm = localtime(&now);
        strftime(time_str, sizeof(time_str), "%h %e %T", tm);

        fprintf(g_log_file, "%s %s %s[%d]: ",
                time_str,
                g_hostname ? g_hostname : "localhost",
                g_daemon_name ? g_daemon_name : "smf-spf",
                (int)getpid());

        va_start(ap, fmt);
        vfprintf(g_log_file, fmt, ap);
        va_end(ap);

        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    }

    /* Log to syslog if configured */
    if (g_syslog_facility != SYSLOG_DISABLE) {
        va_start(ap, fmt);
        vsyslog(log_level, fmt, ap);
        va_end(ap);
    }
}

void log_shutdown(void) {
    if (g_syslog_facility != SYSLOG_DISABLE) {
        closelog();
    }

    g_daemon_name = NULL;
    g_syslog_facility = SYSLOG_DISABLE;
    g_log_file = NULL;
    g_hostname = NULL;
}

void log_set_file(FILE *log_file) {
    g_log_file = log_file;
}
