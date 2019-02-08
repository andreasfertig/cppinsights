#! /bin/sh

cd /home/builder/build_docker

cmake -G Ninja -DINSIGHTS_STATIC=Yes -DINSIGHTS_COVERAGE=Yes -DINSIGHTS_LLVM_CONFIG=/usr/bin/llvm-config-8 ../clang/
ninja coverage

