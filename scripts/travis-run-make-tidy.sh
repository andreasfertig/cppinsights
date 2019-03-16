#! /bin/bash

set -o pipefail
make -j4 2>&1 | tee output.txt
exit $?
