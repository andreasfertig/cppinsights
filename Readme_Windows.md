## Building on Windows {#building_on_windows}

### Tested with (supported compilers)

|         Name       | Version | Actual compiler |    Version    |        CMake command                      |
|--------------------|---------|-----------------|---------------|-------------------------------------------|
| Visual Studio 2017 | 15.9.3  |     cl.exe      | 19.16.27031.1 | -G "Visual Studio 15 2017" -A x64         |
| Visual Studio 2019 | 16.1.3  |     cl.exe      | 19.21.27702.2 | -G "Visual Studio 16 2019" -A x64         |
| Clang (VS 2017)    | 15.9.3  |   clang-cl.exe  | 8.0.0         | -G "Visual Studio 15 2017" -A x64 -T LLVM |
| Clang (VS 2019)    | 16.1.3  |   clang-cl.exe  | 8.0.0         | -G "Visual Studio 16 2019" -A x64 -T LLVM |

Note: supports only building **outside** LLVM.
There is no support for x86 since there is no LLVM/Clang libraries for x86.

For Clang with VS:

 + go to [LLVM Download Page](http://releases.llvm.org/download.html);
 + install "Windows (64-bit)" from "Pre-Built Binaries" section;

Installer will automatically add LLVM toolset to all Visual Studio instances you have.

### Build & install Clang from sources

Needed to have Clang libraries and `llvm-config.exe` to setup CMake.

Installs Clang/LLVM libraries to (for example) `C:\Programs\LLVM_local2`.

Note:

 * it's important to have install path with no spaces
 * better to have something different from %Program Files%
   since otherwise Administrator rights are needed to install files

```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
cmake -DLLVM_ENABLE_PROJECTS=clang ^
      -DCMAKE_INSTALL_PREFIX=C:\Programs\LLVM_local2 ^
      -G "Visual Studio 15 2017" ^
      -A x64 ^
      -Thost=x64 ^
      ..\llvm
cmake --build . --config Release --target install
```

You can also open build/LLVM.sln solution in Visual Studio and build everything
from there instead of using `cmake --build ...` command.

### Build insights

Assume:

 * cppinsights sources are in `C:\dev\cppinsights` and
 * LLVM/Clang built and installed into `C:\Programs\LLVM_local2` (see step above)
 

```
cd C:\dev\cppinsights\
mkdir build
cd build
set path=%path%;C:\Programs\LLVM_local2\bin
cmake -G "Visual Studio 15 2017 Win64" -T LLVM ..
cmake --build . --config Release --target insights
```

Instead of "Visual Studio 15 2017" generator with Clang,
you can choose whatever works for you.
See "Tested with (supported compilers)", *CMake command* column above.

Also, instead of building from command line, you can
open `build/cpp-insights.sln` and have fun with VS.


