#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NOCOLOR='\033[0m' 
./smf-spf -h
for conf in ./tests/fail/* /file/does/not/exists ; do
	echo "#########################################"
	echo "Running $conf ..."
	echo "#########################################"
    ./smf-spf -f -c $conf 2>&1 > /dev/null
done
for testfile in tests/0* ; do
	echo "#########################################"
	echo "Running $testfile ..."
	echo "#########################################"
	miltertest -s $testfile && echo -e "$GREEN Test OK $NOCOLOR" || echo -e "$RED Test failed $NOCOLOR"
done
