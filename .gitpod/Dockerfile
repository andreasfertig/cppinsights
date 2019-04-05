FROM ubuntu:18.04

LABEL maintainer "Andreas Fertig"

# Install compiler, python
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates gnupg wget ninja-build git vim

RUN echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main" >> /etc/apt/sources.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update && \
    apt-get install -y --no-install-recommends llvm-8-dev libclang-8-dev g++-8 cmake zlib1g-dev doxygen graphviz clang-format-8 clang-tidy-8 gdb && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/llvm-config-8 /usr/bin/llvm-config
RUN ln -fs /usr/bin/clang-tidy-8 /usr/bin/clang-tidy
RUN ln -fs /usr/bin/clang-format-8 /usr/bin/format-tidy

RUN ln -fs /usr/bin/g++-8 /usr/bin/g++
RUN ln -fs /usr/bin/g++-8 /usr/bin/c++
RUN ln -fs /usr/bin/gcc-8 /usr/bin/gcc
RUN ln -fs /usr/bin/gcc-8 /usr/bin/cc

### Gitpod user ###
# '-l': see https://docs.docker.com/develop/develop-images/dockerfile_best-practices/#user
RUN useradd -l -u 33333 -G sudo -md /home/gitpod -s /bin/bash -p gitpod gitpod
    # passwordless sudo for users in the 'sudo' group
#    && sed -i.bkp -e 's/%sudo\s\+ALL=(ALL\(:ALL\)\?)\s\+ALL/%sudo ALL=NOPASSWD:ALL/g' /etc/sudoers
ENV HOME=/home/gitpod
WORKDIR $HOME
# custom Bash prompt
RUN { echo && echo "PS1='\[\e]0;\u \w\a\]\[\033[01;32m\]\u\[\033[00m\] \[\033[01;34m\]\w\[\033[00m\] \\\$ '" ; } >> .bashrc

RUN mkdir /home/gitpod/build && chown -R gitpod:gitpod /home/gitpod/build

### Gitpod user (2) ###
USER gitpod
# use sudo so that user does not get sudo usage info on (the first) login
#RUN sudo echo "Running 'sudo' for Gitpod: success"

### checks ###
# no root-owned files in the home directory
RUN notOwnedFile=$(find . -not "(" -user gitpod -and -group gitpod ")" -print -quit) \
    && { [ -z "$notOwnedFile" ] \
|| { echo "Error: not all files/dirs in $HOME are owned by 'gitpod' user & group"; exit 1; } }
