#! /bin/bash
# From: https://github.com/ainfosec/ci_helloworld/blob/master/.travis.yml

OUTPUT_FILE=$1
IGNORE_FILE=$2

if [ ! -f ${OUTPUT_FILE} ]; then
    echo "You need to run a clang-tidy enabled build and pipe the output to output.txt:"
    echo "make -j4 2>&1 | tee output.txt"
    exit 0
fi

CLEANED_OUTPUT=$(grep -E -i -v -f ${IGNORE_FILE} ${OUTPUT_FILE})

if [[ -n $(echo ${CLEANED_OUTPUT} | grep "warning: ") ]] || [[ -n $(echo ${CLEANED_OUTPUT} | grep "error: ") ]]; then
    echo -e "\033[1;31mYou must pass the clang tidy checks before submitting a pull request.\033[0m"
    echo ""
    grep --color -E '^|warning: |error: ' output.txt
    exit 1;
else
    echo -e "\033[1;32m\xE2\x9C\x93 passed:\033[0m $1";
fi

exit 0
