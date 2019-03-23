FROM ubuntu:18.04

LABEL maintainer "Andreas Fertig"

# Install compiler, python
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates gnupg wget ninja-build

RUN echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main" >> /etc/apt/sources.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update && \
    apt-get install -y --no-install-recommends llvm-8-dev libclang-8-dev g++-8 cmake zlib1g-dev doxygen graphviz && \
    rm -rf /var/lib/apt/lists/*

RUN useradd builder \
    && mkdir /home/builder \
    && chown -R builder:builder /home/builder

RUN ln -s /usr/bin/llvm-config-8 /usr/bin/llvm-config

RUN ln -fs /usr/bin/g++-8 /usr/bin/g++
RUN ln -fs /usr/bin/g++-8 /usr/bin/c++
RUN ln -fs /usr/bin/gcc-8 /usr/bin/gcc
RUN ln -fs /usr/bin/gcc-8 /usr/bin/cc

RUN apt-get update && \
    apt-get install -y --no-install-recommends gdb && \
    rm -rf /var/lib/apt/lists/*

