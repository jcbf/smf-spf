-- Copyright (c) 2009-2013, The Trusted Domain Project.  All rights reserved.

mt.echo("*** malformed From: field message")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information
-- mt.negotiate() is called implicitly
mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "localhost", "10.11.12.13") ~= nil then
	error("mt.conninfo() failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.conninfo() unexpected reply")
end

-- send envelope macros and sender data
-- mt.helo() is called implicitly
mt.macro(conn, SMFIC_MAIL, "i", "t-verify-malformed")
if mt.mailfrom(conn, "<user@underspell.com>") ~= nil then
	error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.mailfrom() unexpected reply")
end

-- send headers
-- mt.rcptto() is called implicitly
if mt.header(conn, "From", "user") ~= nil then
	error("mt.header(From) failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.header(From) unexpected reply")
end
if mt.header(conn, "Date", "Tue, 22 Dec 2009 13:04:12 -0800") ~= nil then
	error("mt.header(Date) failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.header(Date) unexpected reply")
end
if mt.header(conn, "Subject", "Signing test") ~= nil then
	error("mt.header(Subject) failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.header(Subject) unexpected reply")
end

-- end of message; let the filter react
if mt.eom(conn) ~= nil then
	error("mt.eom() failed")
end

-- verify that the right Authentication-Results header field got added
if mt.eom_check(conn, MT_HDRINSERT, "Authentication-Results") or
   mt.eom_check(conn, MT_HDRADD, "Authentication-Results") then
	ar = mt.getheader(conn, "Authentication-Results", 0)
	if string.find(ar, "spf=fail", 1, true) == nil then
		error("incorrect Authentication-Results field")
	end
else
	mt.echo ("Got header Authentication-Results: " .. ar)
	error("missing Authentication-Results field")
end

mt.disconnect(conn)
