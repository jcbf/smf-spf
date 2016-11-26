mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests.conf")
-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 40, 0.25)
if conn == nil then
        error("mt.connect() failed")
end

print "Connect to Milter:  OK"
