mt.echo("Ip address whitelisted")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information

mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "a.server.name.local", "10.0.0.1") ~= nil then
	error("mt.conninfo() failed")
end

if mt.getreply(conn) ~= SMFIR_CONTINUE then
        error("mt.conninfo() unexpected reply")
end


mt.macro(conn, SMFIC_MAIL, "i", "t-verify-malformed")
if mt.mailfrom(conn, "<something@underspell.com>") ~= nil then
        error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
        error("mt.mailfrom() unexpected reply")
end

mt.macro(conn, SMFIC_RCPT, "i", "t-verify-malformed")
if mt.rcptto(conn, "<spamlover@example.com>") ~= nil then
        error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_ACCEPT then
        error("mt.mailfrom() unexpected reply")
end

print ("Received SMFIR_ACCEPT");
