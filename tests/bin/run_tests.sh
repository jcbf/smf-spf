#!/bin/bash

./smf-spf -h
for conf in tests/fail/smf-spf-tests-faiillogfile.conf tests/fail/does-not-exists.conf; do
	echo "#########################################"
	echo "Running $conf ..."
	echo "#########################################"
    ./smf-spf -f -c $conf 2>&1 > /dev/null
done
for testfile in tests/0* ; do
	echo "#########################################"
	echo "Running $testfile ..."
	echo "#########################################"
	miltertest -s $testfile && echo -e "Test OK" || echo -e "Test failed"
done
