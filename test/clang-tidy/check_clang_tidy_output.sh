#!/bin/sh
#
# Run clang-tidy and verify its command line output.

INPUT_FILE=$1
CHECK_TO_RUN=$2

clang-tidy --checks=${CHECK_TO_RUN} --disable-checks="" ${INPUT_FILE} \
    -- --std=c++11 \
  | FileCheck ${INPUT_FILE}
