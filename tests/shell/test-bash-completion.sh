#! /bin/bash
#
# C++ Insights, copyright (C) by Andreas Fertig
# Distributed under an MIT license. See LICENSE for details
#
#------------------------------------------------------------------------------

PATH=$PATH:$1
failureIsOkay=$4

cd $2

source $3

ret=0

do_test()
{
COMP_WORDS=(insights $1)    # array of words in command line
COMP_CWORD=1                # index of the word containing cursor position

ins="$(complete -p insights 2>/dev/null|sed 's/.*-F \([^ ]*\) .*/\1/')"

$ins a a

expect=$2
res=${COMPREPLY[@]}

if [ "$res" == "$expect" ]; then
    echo "[PASSED] $1"
else
    echo "[FAILED] $1"
    echo "${COMPREPLY[@]}"
    ret=1
fi
}

echo "Running bash autocomplete tests..."

do_test "-- -std" "-std= -stdlib -stdlib++-isystem -stdlib="
do_test "-- -std = c++1" "c++11 c++14 c++17 c++1y c++1z"
do_test "--use" "--use-libc++"
do_test "--use-libc++ -- -Weff" "-Weffc++"
do_test "-- ---" "a.cpp another.cpp b.cpp test-bash-completion.sh"
do_test "-- -fmodule-name=a." "a.cpp another.cpp b.cpp test-bash-completion.sh"
do_test "" "a.cpp another.cpp b.cpp test-bash-completion.sh"
do_test "a" "a.cpp another.cpp"
do_test "a." "a.cpp"

if [ "$failureIsOkay" == "--failure-is-ok" ]; then
    echo "Accepting failed tests"
    exit 0
fi

exit $ret
