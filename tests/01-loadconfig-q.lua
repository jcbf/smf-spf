mt.startfilter("./smf-spf", "-f", "-c","./smf-spf-tests-q.conf")
-- try to connect to it
conn = mt.connect("unix:/tmp/milter.sock", 4, 0.25)
if conn == nil then
        error("mt.connect() failed")
end

print "Connect to Milter:  OK"
