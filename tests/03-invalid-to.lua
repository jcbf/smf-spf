mt.echo("Invalid recipient")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information

mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "a.server.name.local", "10.11.12.13") ~= nil then
	error("mt.conninfo() failed")
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
mt.macro(conn, SMFIC_RCPT, "i", "t-verify-malformed")
if mt.rcptto(conn, "user@underspell.com") ~= nil then
        error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_REJECT then
        error("mt.mailfrom() unexpected reply")
end

print ("received SMFIR_REJECT ") 
