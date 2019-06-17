## Building on Windows

Note: tested with Visual Studio 2017

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
cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_INSTALL_PREFIX=C:\Programs\LLVM_local2 -G "Visual Studio 15 2017" -A x64 -Thost=x64 ..\llvm
cmake --build . --config Release --target install
```

You can also open build/LLVM.sln solution in Visual Studio and build everything
from there instead of using `cmake --build ...` command.

### Build insights

Note: this is done with Clang toolset (-T LLVM).
You need to have Clang binaries installed and integrated to Visual Studio first.
Simply go to [LLVM Download Page](http://releases.llvm.org/download.html) and
install "Windows (64-bit)" from "Pre-Built Binaries" section.
This will add `LLVM` toolset to VS automatically.

Assume:

 * cppinsights sources are in `C:\dev\cppinsights` and
 * LLVM/Clang build and installed into `C:\Programs\LLVM_local2` (see step above)
 

```
cd C:\dev\cppinsights\
mkdir build
cd build
set path=%path%;C:\Programs\LLVM_local2\bin
cmake -G "Visual Studio 15 2017 Win64" -T LLVM ..
cmake --build . --config Release --target insights
```

