#!/bin/bash

for testfile in tests/* ; do
	miltertest -vv -s $testfile
done
