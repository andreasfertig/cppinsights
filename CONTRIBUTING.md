# Contributing to C++ Insights {#contributing}

Thanks for considering contributing to **C++ Insights**.

**C++ Insights** follows a [Code of Conduct](CODE_OF_CONDUCT.md) which aims to foster an open and welcoming environment.

## In brief

C++ Insights is a clang tooling project. Therefore it requires libclang to be installed. You can compile it either with
g++ or clang++.

The current C++ language version used is C++17.


## Contribute

There is a Travis CI running which compiles the code on Linux and macOS. Currently, it is limited to a out-of-source
build. Windows is not supported yet. Please check the Travis CI status for each PR. The macOS build runs the unit tests.
Code coverage runs on one of the Linux images.

### Fixing an Issue

Create a test named: `Issue<ISSUE_NUMBER>.cpp` and a `Issue<ISSUE_NUMBER>.expect` with the transformed code after the fix.

The current style in the PR is:
```
Fixed #<ISSUE_NUMBER>: <SHORT EXPLANATION>

<DETAILD EXPLANATION>
```

### Contribute new functionality


