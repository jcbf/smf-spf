#!/bin/bash

./smf-spf -h
RET=0
for testfile in tests/* ; do
	echo "#########################################"
	echo "Running $testfile ..."
	miltertest  -s $testfile
	status=$?
	echo -n "returned "
	if [ $status -eq 0 ] ; then 
            echo -e "\e[32msuccessful\e[39m"
    else 
            echo -e "\e[31mfailed\e[39m"
            RET=1
            FAILED+="\n\t$testfile"
    fi
done

[[ -z "$FAILED" ]] || echo -e "#########################################\nFailed tests $FAILED"
exit $RET
