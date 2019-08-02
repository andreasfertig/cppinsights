FROM andreasfertig/cppinsights-builder:latest

LABEL maintainer "Andreas Fertig"

# Install compiler, python
RUN apt-get update && \
    apt-get install -y --no-install-recommends clang-format-${CLANG_VERSION} clang-tidy-${CLANG_VERSION} gnupg git vim gdb && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/llvm-config-${CLANG_VERSION} /usr/bin/llvm-config
RUN ln -fs /usr/bin/clang-tidy-${CLANG_VERSION} /usr/bin/clang-tidy
RUN ln -fs /usr/bin/clang-format-${CLANG_VERSION} /usr/bin/format-tidy

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

### Gitpod user (2) ###
USER gitpod
# use sudo so that user does not get sudo usage info on (the first) login
#RUN sudo echo "Running 'sudo' for Gitpod: success"

### checks ###
# no root-owned files in the home directory
RUN notOwnedFile=$(find . -not "(" -user gitpod -and -group gitpod ")" -print -quit) \
    && { [ -z "$notOwnedFile" ] \
|| { echo "Error: not all files/dirs in $HOME are owned by 'gitpod' user & group"; exit 1; } }
