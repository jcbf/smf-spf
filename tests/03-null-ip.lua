mt.echo("*** SPF test")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
	error("mt.connect() failed")
end

-- send connection information

mt.macro(conn, SMFIC_CONNECT, "j", "mta.name.local")
if mt.conninfo(conn, "a.server.name.local", "unspec") ~= nil then
	error("mt.conninfo() failed")
end

print ("mt.conninfo() received ",mt.getreply(conn) )
