mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests-faiillogfile.conf")
-- try to connect to it
conn = mt.connect("inet:2424@127.0.0.1", 10, 0.25)
if conn == nil then
        error("mt.connect() failed")
end

if conn == nil then
        print "Connect to Milter failed: should fail"
else
        error("mt.connect() to milter. Test failed")
end
