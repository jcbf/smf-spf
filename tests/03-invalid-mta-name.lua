
mt.echo("SPF fail  test")

-- try to start the filter
mt.startfilter("./smf-spf", "-f", "-c","tests/conf/smf-spf-tests-q.conf")

-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 10, 0.50)
if conn == nil then
	error("mt.connect() failed")
end

if mt.conninfo(conn, "localhost", "10.11.12.13") ~= nil then
	error("mt.conninfo() failed")
end
if mt.getreply(conn) == SMFIR_CONTINUE then
	mt.echo("empty MTA name test passed")
else
	error("mt.conninfo() unexpected reply")
end
mt.disconnect(conn)
