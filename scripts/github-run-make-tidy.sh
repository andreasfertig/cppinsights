#! /bin/bash

set -o pipefail
cmake --build . 2>&1 | tee output.txt
exit $?
