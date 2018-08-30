FROM debian:testing

LABEL maintainer "Andreas Fertig"

# Install compiler, python
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates gnupg \
          wget cmake g++-8 python2.7 ninja-build

RUN echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch main" >> /etc/apt/sources.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update && \
    apt-get install -y --no-install-recommends llvm-8-dev libclang-8-dev libc++-dev zlib1g-dev bzip2 unzip && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential && \
    rm -rf /var/lib/apt/lists/*

RUN cd /tmp && \
    wget https://github.com/linux-test-project/lcov/archive/master.zip && \
    unzip master.zip && \
    cd lcov-master && \
    make install && \
    cd / && \
    rm -rf /tmp/*


RUN useradd builder \
    && mkdir /home/builder \
    && mkdir /home/builder/clang \
    && chown -R builder:builder /home/builder

RUN ln -s /usr/bin/llvm-config-8 /usr/bin/llvm-config

#RUN update-alternatives --install /usr/bin/g++ c++ /usr/bin/g++-8 10
##RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 10
#RUN update-alternatives --install /usr/bin/gcc cc  /usr/bin/gcc-8 10
#RUN update-alternatives --config cc
#RUN update-alternatives --config c++
RUN ln -fs /usr/bin/g++-8 /usr/bin/g++
RUN ln -fs /usr/bin/gcc-8 /usr/bin/gcc
RUN ln -fs /usr/bin/gcc-8 /usr/bin/cc
RUN ln -fs /usr/bin/g++-8 /usr/bin/c++
RUN ln -fs /usr/bin/python2.7 /usr/bin/python
RUN ln -fs /usr/bin/gcov-8 /usr/bin/gcov
