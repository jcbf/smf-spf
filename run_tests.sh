#!/bin/bash

for testfile in tests/* ; do
	echo "#########################################"
	echo "Running $testfile ..."
	echo "#########################################"
	miltertest -vv -s $testfile
done
