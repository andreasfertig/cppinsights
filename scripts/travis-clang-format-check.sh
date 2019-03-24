#! /bin/bash

CLANG_FORMAT=$1

echo -n "Running clang-format checks, version: "
${CLANG_FORMAT} --version

if [ 0 != $? ]; then
    echo -e "\033[1;31mclang-format missing: ${CLANG_FORMAT}\033[0m"
    exit 1
fi

if [ "$TRAVIS_PULL_REQUEST" == "false" ] ; then
  # Not in a pull request, so compare against parent commit
  base_commit="HEAD^"
  echo "Checking against parent commit $(git rev-parse $base_commit)"
else
  base_commit="$TRAVIS_COMMIT_RANGE"
  echo "Checking against commit $base_commit"
fi

filesToCheck="$(git diff --name-only --diff-filter=d ${base_commit} | grep -e '.\(\.cpp\|\.h\)$' || true)"



for f in $filesToCheck; do
    echo "  Checking: ${f}"
    d=$(diff -u "$f" <($CLANG_FORMAT -style=file "$f") || true)
    if ! [ -z "$d" ]; then
        echo "$d"
        fail=1
    fi
done

if [ "$fail" = 1 ]; then
    echo -e "\033[1;31mYou must pass the clang-format checks before submitting a pull request.\033[0m"
    exit 1
fi

echo -e "\033[1;32m\xE2\x9C\x93 passed clang-format checks\033[0m $1";
exit 0
