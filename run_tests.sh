#!/bin/bash

./smf-spf -h
for testfile in tests/* ; do
	echo "#########################################"
	echo "Running $testfile ..."
	miltertest  -s $testfile
	status=$?
	echo -n "returned "
	[ $status -eq 0 ] && echo -e "\e[32msuccessful\e[39m" || echo -e "\e[31mfailed\e[39m"
done
