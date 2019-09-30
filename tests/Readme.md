# C++ Insights - Tests {#tests}

## Running tests
Here you find files for regression tests. They can be invoked either directly with `runTest.py` or with a build
target:

```
cmake --build . --target tests
```

the later one tries to setup all the stuff required (compiler, include paths, path to the insights executable...).

To run a single test you can use this:
```
./runTest.py --insights=PATH-TO-insights --cxx=PATH-TO-COMPILER TemplatesWithAutoAndLambdaTest.cpp
```

## What kind of tests

In general this is a end-to-end verification system. There are no unit tests. There are only checks, if for a known input
a expected output is generated. C++ code level unit test would require create AST nodes. This is possible, but has still 
the issue of beeing not as narrow as wanted. The end-to-end test seem best and worked out good so far.

## The test system

The system is, that for each *.cpp file a corresponding *.expect file exists. This expect file contains the transformed
output. In some, hopefully rare, cases an additional file *.cerr exists as well. This file then contains the compiler
error output. The idea is to track all changes as close as possible.

`runTest.py` does invoke insights as first and transforms the `*.cpp`. It then compares the result to the corresponding 
`.expect` files contents. If there is a difference it shows the output of the diff. In any case it invokes gcc or clang 
afterwards and tries to compile the transformed code.

Currently, it is hard to do this tests. One reason is, that the transformation result is different between [libstdc++](https://gcc.gnu.org/onlinedocs/libstdc++/) and
[libc++](https://libcxx.llvm.org/). This results in many difference, if tests are executed under Linux vs. macOS. As my main development environment 
is currently macOS all the test results are based on that. 

## Test creation rules

For each issue a dedicated `Issue#ISSUE_NUMBER.cpp` file is created with a matching `.expect` file. Both should be
checked in together with the actual fix.

Other tests are named `NAME_OF_THE_TESTTest.cpp`. In case, there are more than one test for the same subject the scheme
is `NAME_OF_THE_TESTNUMBERTest.cpp`. For example:

```
StructuredBindingsHandlerTest.cpp
StructuredBindingsHandler2Test.cpp
```

## Updating tests

There is a dedicated build target for cases where a change causes the need to update a lot of tests 

```
make update-tests
```

Does update all failed tests as well as existing `.cerr` files. Be sure to check, whether the updated tests are in fact
correct.
