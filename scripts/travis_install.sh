#! /bin/sh

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
   brew install cmake || brew upgrade cmake
   brew install xz || brew upgrade xz

   mkdir ${TRAVIS_BUILD_DIR}/clang
   cd ${TRAVIS_BUILD_DIR}/clang
   wget https://releases.llvm.org/6.0.0/clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz

   xz -vd clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz
   tar -xf clang+llvm-6.0.0-x86_64-apple-darwin.tar
   mv clang+llvm-6.0.0-x86_64-apple-darwin current

   ls -l ${TRAVIS_BUILD_DIR}/clang/current/bin/clang
fi
