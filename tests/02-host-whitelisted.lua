mt.echo("Host whitelisted")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information

mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "server.example.org", "2001:0101:DEAD:BEEF::1") ~= nil then
	error("mt.conninfo() failed")
end

if mt.getreply(conn) ~= SMFIR_ACCEPT then
        error("mt.conninfo() unexpected reply")
end
print ("Received SMFIR_ACCEPT");
