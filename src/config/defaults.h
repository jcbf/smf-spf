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

#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include <stdbool.h>
#include <syslog.h>

/* File paths and constants */
#define CONFIG_FILE_DEFAULT		"/etc/mail/smfs/smf-spf.conf"
#define WORK_SPACE			"/var/run/smfs"
#define OCONN_DEFAULT			"unix:" WORK_SPACE "/smf-spf.sock"
#define USER_DEFAULT			"smfs"
#define TAG_STRING_DEFAULT		"[SPF:fail]"
#define QUARANTINE_BOX_DEFAULT		"postmaster"
#define REJECT_REASON_DEFAULT		"Message was rejected during SPF policy evaluation. sender:%1$s client-ip:%2$s"

/* Default boolean and numeric settings */
#define SYSLOG_FACILITY_DEFAULT		LOG_MAIL
#define SPF_TTL_DEFAULT			3600
#define RELAXED_LOCALPART_DEFAULT	0
#define BEST_GUESS_DEFAULT		1
#define REFUSE_FAIL_DEFAULT		1
#define REFUSE_NONE_DEFAULT		0
#define REFUSE_NONE_HELO_DEFAULT	0
#define SOFT_FAIL_DEFAULT		0
#define ACCEPT_PERMERR_DEFAULT		1
#define TAG_SUBJECT_DEFAULT		1
#define ADD_HEADER_DEFAULT		1
#define ADD_RECV_HEADER_DEFAULT		0
#define QUARANTINE_DEFAULT		0
#define DAEMONIZE_DEFAULT		1
#define SKIP_AUTH_DEFAULT		true
#define SKIP_NDR_DEFAULT		false
#define SYSLOG_DISABLE			-2

/* Special values */
#define SPF_GUESS_RECORD 		"v=spf1 a/24 mx/24 ptr ?all"
#define SPF_GUESS_TEXT 			" (SPF best guess)"

/* Size limits */
#define MAX_HEADER_SIZE			2048
#define MAXLINE				258
#define MAXLOCALPART			64
#define IPV4_DOT_DECIMAL		"^[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}$"
#define FACILITIES_AMOUNT		10
#define MAX_WHITELIST_ENTRIES		10000

#endif /* CONFIG_DEFAULTS_H */
