#! /bin/bash

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
    make tests
fi
