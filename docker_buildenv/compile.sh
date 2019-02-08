#! /bin/sh

BASE_PATH=$1

if [ "$#" -ne 1 ]; then
    BASE_PATH=`pwd`
fi

INSIGHTS_PATH=$(realpath ${BASE_PATH}/..)

mkdir build_docker

/bin/cp -f ${BASE_PATH}/build_insights.sh build_docker

echo `pwd`

docker run -v $INSIGHTS_PATH:/home/builder/clang -v `pwd`/build_docker/:/home/builder/build_docker --rm -it --security-opt seccomp=unconfined  cppinsights-buildenv2 /home/builder/build_docker/build_insights.sh

