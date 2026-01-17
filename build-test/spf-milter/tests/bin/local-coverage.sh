#!/bin/bash

make clean
make coverage
./run_tests.sh
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info 'tests/*' '/usr/*' --output-file coverage.info
lcov --list coverage.info