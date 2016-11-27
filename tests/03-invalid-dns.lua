mt.echo("Invalid SPF record ")

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
-- badspf.underspell.com.  7200  TXT "v=spf1 redirect:badspf.underspell.com"
-- badspf2.underspell.com. 7200  TXT "v=spf1 "

mt.macro(conn, SMFIC_MAIL, "i", "DEADBEAF")
if mt.mailfrom(conn, "<user@badspf2.underspell.com>") ~= nil then
        error("mt.mailfrom() failed")
end
if mt.getreply(conn) ~= SMFIR_CONTINUE then
        error("mt.mailfrom() unexpected reply")
end

print ("received SMFIR_CONTINUE ") 
