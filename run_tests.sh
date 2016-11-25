#!/bin/bash

for testfile in tests/* ; do
	miltertest -s $testfile
done
