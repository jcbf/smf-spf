/*
 * logging.h - Logging abstraction for smf-spf
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

#ifndef SMF_SPF_LOGGING_H
#define SMF_SPF_LOGGING_H

#include <stdio.h>
#include <syslog.h>

/* Special value to disable syslog */
#define SYSLOG_DISABLE -2

/**
 * @brief Initialize the logging system
 *
 * Initializes both syslog and optional file logging.
 *
 * @param daemon_name Name of the daemon for syslog
 * @param syslog_facility Syslog facility (e.g., LOG_MAIL) or SYSLOG_DISABLE
 * @param log_file Optional file pointer for file logging (NULL to disable)
 * @param hostname Hostname for log messages
 */
void log_init(const char *daemon_name, int syslog_facility,
              FILE *log_file, const char *hostname);

/**
 * @brief Log a formatted message
 *
 * Logs a message to syslog and/or file depending on configuration.
 *
 * @param log_level Syslog priority level (e.g., LOG_INFO, LOG_ERR)
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
void log_message(int log_level, const char *fmt, ...);

/**
 * @brief Shutdown the logging system
 *
 * Closes syslog and file handles.
 */
void log_shutdown(void);

/**
 * @brief Update file logging pointer
 *
 * @param log_file New file pointer (NULL to disable)
 */
void log_set_file(FILE *log_file);

#endif /* SMF_SPF_LOGGING_H */
