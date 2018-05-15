#! /bin/sh

cd /home/builder/build_docker

cmake -G Ninja -DINSIGHTS_STATIC=Yes -DINSIGHTS_LLVM_CONFIG=/usr/bin/llvm-config-7 ../clang/
ninja


# packing include to save installing gcc in the runtime docker
tar -cjf include.tar.bz2 /usr/include
tar -cjf /home/builder/build_docker/include4.tar.bz2 /usr/lib/llvm-7/lib/clang/7.0.0/include

