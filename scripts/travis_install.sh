#! /bin/sh

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
   export HOMEBREW_NO_AUTO_UPDATE=1
   brew update > /dev/null
   brew install cmake || brew upgrade cmake
   brew install xz || brew upgrade xz

   brew install ccache
   export PATH="/usr/local/opt/ccache/libexec:$PATH"

   mkdir ${TRAVIS_BUILD_DIR}/clang
   cd ${TRAVIS_BUILD_DIR}/clang
   wget -q --continue --directory-prefix=$HOME/Library/Caches/Homebrew https://releases.llvm.org/6.0.0/clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz
   cp $HOME/Library/Caches/Homebrew/clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz

   xz -d clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz
   tar -xf clang+llvm-6.0.0-x86_64-apple-darwin.tar
   mv clang+llvm-6.0.0-x86_64-apple-darwin current
fi
