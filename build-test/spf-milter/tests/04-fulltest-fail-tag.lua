
mt.echo("SPF fail  test")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","tests/conf/smf-spf-tests-q.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information
-- mt.negotiate() is called implicitly
-- mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
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

if mt.rcptto(conn, "<rcpt1@underspell.com>") ~= nil then
	error("mt.rcptto(rcpt1@underspell.com) failed")
end
if mt.rcptto(conn, "<rcpt2@underspell.com>") ~= nil then
	error("mt.rcptto(rcpt2@underspell.com) failed")
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
-- if mt.header(conn, "Subject", "Signing test") ~= nil then
-- 	error("mt.header(Subject) failed")
-- end
-- if mt.getreply(conn) ~= SMFIR_CONTINUE then
-- 	error("mt.header(Subject) unexpected reply")
-- end

-- end of message; let the filter react
if mt.eom(conn) ~= nil then
	error("mt.eom() failed")
end

-- verify that the right Authentication-Results header field got added
if mt.eom_check(conn, MT_HDRCHANGE, "Subject") or
   mt.eom_check(conn, MT_HDRADD, "Subject") or
   mt.eom_check(conn, MT_HDRINSERT, "Subject") then
	subject = mt.getheader(conn, "Subject", 0)
	if subject ~= nil and 
		string.find(subject, "SPF:fail", 1, true) == nil then
		error("incorrect Subject")
	else
		mt.echo("Found subject tag: " .. subject)
	end
else
	error("missing Subject")
end

mt.disconnect(conn)
