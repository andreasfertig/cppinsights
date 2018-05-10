#! /bin/sh

BASE_PATH=$1

if [ "$#" -ne 1 ]; then
    BASE_PATH=`pwd`
fi

INSIGHTS_PATH=$(realpath ${BASE_PATH}/..)

mkdir build_docker

/bin/cp -f ${BASE_PATH}/build_insights.sh build_docker

echo `pwd`

docker run -v $INSIGHTS_PATH:/home/builder/clang -v `pwd`/build_docker/:/home/builder/build_docker --rm -it --security-opt seccomp=unconfined  cppinsights-buildenv /home/builder/build_docker/build_insights.sh


INSIGHTS_RUNTIME_PATH=$(realpath `pwd`/../docker_runtime)
#cp build_docker/insights ${INSIGHTS_RUNTIME_PATH}/
#cp build_docker/include.tar.bz2 ${INSIGHTS_RUNTIME_PATH}/
#cp build_docker/include4.tar.bz2 ${INSIGHTS_RUNTIME_PATH}/

scp build_docker/insights insights:~/
