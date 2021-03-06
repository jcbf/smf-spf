-- Copyright (c) 2009-2013, The Trusted Domain Project.  All rights reserved.
mt.echo("SPF helo pass test")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","tests/conf/smf-spf-tests-natip.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information
-- mt.negotiate() is called implicitly
mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "helo.underspell.com","10.11.12.13") ~= nil then
	error("mt.conninfo() failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.conninfo() unexpected reply")
end

if mt.helo(conn, "helo.underspell.com") ~= nil then
	error("mt.helo() failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
	error("mt.helo() unexpected reply")
end

-- send envelope macros and sender data
-- mt.helo() is called implicitly
mt.macro(conn, SMFIC_MAIL, "i", "t-empty-sender")
if mt.mailfrom(conn, "<>") ~= nil then
	error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_ACCEPT then
	error("mt.mailfrom() unexpected reply")
else
	mt.echo ("Got ACCEPT")
end
mt.disconnect(conn)
