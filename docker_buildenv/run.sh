#! /bin/sh

BASE_PATH=$1

if [ "$#" -ne 1 ]; then
    BASE_PATH=`pwd`
fi


INSIGHTS_PATH=$(realpath ${BASE_PATH}/..)

docker run -v $INSIGHTS_PATH:/home/builder/clang -v `pwd`/build_docker/:/home/builder/build_docker --rm -it --security-opt seccomp=unconfined  cppinsights-buildenv2 /bin/bash -l
