# C++ Insights - Tests

Here you find files for regression tests. They can be invoked either directly with ```runTest.py``` or with a build
target:

```
cmake --build . --target tests
```

the later one tries to setup all the stuff required (compiler, include paths, path to the insights executable...).

The system is, that for each *.cpp file a corresponding *.expect file exists. This expect file contains the transformed
output. In some, hopefully rare, cases an additional file *.cerr exists as well. This file then contains the compiler
error output. The idea is to track all changes as close as possible.

```runTest.py``` does invoke insights as first and transforms the *.cpp. It then compares the result to the expect. If
there is a difference it shows the output of diff. In any case it invokes gcc or clang afterwards and tries to compile
the transformed code.

Currently, it is hard to do this tests. One reason is, that the transformation result is different between glibc and
libstd++. This results in many difference, if tests are executed under Linux. As my main development environment is
currently macOS all the test results are based on that.

To run a single test you can use this:
```
./runTest.py --insights=PATH-TO-insights --cxx=PATH-TO-COMPILER TemplatesWithAutoAndLambdaTest.cpp
```
