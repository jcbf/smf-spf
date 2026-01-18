mt.echo("Invalid sender hardfail - ./smf-spf-tests-refuse.conf")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","tests/conf/smf-spf-tests-fixedip-fail.conf")

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
mt.macro(conn, SMFIC_MAIL, "i", "DEADBEAF")
if mt.mailfrom(conn, "underspell.com") ~= nil then
        error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_REPLYCODE then
        error("mt.mailfrom() unexpected reply")
end
print ("received SMFIR_REPLYCODE ") 

if mt.mailfrom(conn, "<12345678901234567890123456789012345678901234567890123456789012345@underspell.com>") ~= nil then
        error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_REPLYCODE then
        error("mt.mailfrom() unexpected reply")
end
print ("received SMFIR_REPLYCODE ")

