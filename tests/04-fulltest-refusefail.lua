-- Copyright (c) 2009-2013, The Trusted Domain Project.  All rights reserved.
mt.echo("SPF pass  test")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests-refuse.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information
-- mt.negotiate() is called implicitly
mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "localhost", "195.22.26.194") ~= nil then
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

if mt.getreply(conn) ~= SMFIR_REPLYCODE then
	        error("mt.mailfrom() unexpected reply")
end

print ("received SMFIR_REPLYCODE ")

mt.disconnect(conn)
