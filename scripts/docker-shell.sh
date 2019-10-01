#! /bin/sh

BUILD_FOLDER=$1

if [ "$#" -lt 2 ]; then
    echo "You need to specify the build folder location and operation type"
    echo "$0 /home/myuser/somewhere/build shell|compile"
    exit 1
fi

BASE_PATH=`pwd`
INSIGHTS_PATH=$(realpath ${BASE_PATH}/..)
BUILD_FOLDER=$(realpath ${BUILD_FOLDER})
OP_SHELL=!
DOCKER_COMMAND="/bin/bash -l"
CMAKE_CONFIG_PARAMS=$3
CMAKE_COMMAND=$4



if [ $2 != "shell" ]; then
    echo "Compile mode"
    OP_SHELL=0
else
    echo "Shell mode"
fi


if [ ! -d "$BUILD_FOLDER" ]; then
    echo "Creating build folder: ${BUILD_FOLDER}"
    mkdir -p ${BUILD_FOLDER}
fi

if [ 0 == ${OP_SHELL} ]; then
    DOCKER_COMMAND="/workspace/build/build_insights.sh"
    BUILD_SCRIPT="${BUILD_FOLDER}/build_insights.sh"

    if [ "${CMAKE_CONFIG_PARAMS}" == "" ]; then
        echo "Using pre-define CMAKE config parameters"
        CMAKE_CONFIG_PARAMS="-DINSIGHTS_STATIC=Yes -DDEBUG=Yes"
    fi

    echo "Creating build_insights.sh"
    echo "#! /bin/sh" > ${BUILD_SCRIPT}
    echo "" >> ${BUILD_SCRIPT}
    echo "cd /workspace/build" >> ${BUILD_SCRIPT}
    echo "" >> ${BUILD_SCRIPT}
    echo "cmake -G Ninja ${CMAKE_CONFIG_PARAMS} -DINSIGHTS_LLVM_CONFIG=/usr/bin/llvm-config /workspace/cppinsights/" >> ${BUILD_SCRIPT}
    echo "ninja ${CMAKE_COMMAND}" >> ${BUILD_SCRIPT}
    chmod 0755 ${BUILD_SCRIPT}
fi


docker run -v $INSIGHTS_PATH:/workspace/cppinsights -v ${BUILD_FOLDER}:/workspace/build --rm -it --security-opt seccomp=unconfined andreasfertig/cppinsights-builder-gitpod ${DOCKER_COMMAND}

