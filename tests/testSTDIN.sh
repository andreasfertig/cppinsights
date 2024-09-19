#! /bin/bash

# fail immediately
set -e

testCppfile="testSTDIN.cpp"

# Good case
cat AutoHandler3Test.cpp | $1 -stdin $testCppfile -- -std=c++17 > /dev/null

# More than one file in STDIN mode -> not allowed
cat AutoHandler3Test.cpp | $1 -stdin $testCppfile other.cpp -- -std=c++17 > /dev/null || echo -n ""

# blank file -> allowed
echo "" | $1 -stdin $testCppfile -- -std=c++17 > /dev/null

# empty file -> not allowed
`echo -n ""` | $1 -stdin $testCppfile -- -std=c++17 > /dev/null || echo -n ""

# Testing an option
$1 -version

$1 AutoHandler3Test.cpp -output /tmp/output.cpp && [ -f /tmp/output.cpp ]

# close stdin
exec 0<&-

# results in: Bad file descriptor
$1 -stdin $testCppfile -- -std=c++17 || echo -n ""

exit 0
