![cpp insights logo](artwork/logo_cppinsights.png)
# C++ Insights - See your source code with the eyes of a compiler.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT) [![download](https://img.shields.io/badge/latest-download-blue.svg)](https://github.com/andreasfertig/cppinsights/releases) [![Build Status](https://api.travis-ci.org/andreasfertig/cppinsights.svg?branch=master)](https://travis-ci.org/andreasfertig/cppinsights) 
[![codecov](https://codecov.io/gh/andreasfertig/cppinsights/branch/master/graph/badge.svg)](https://codecov.io/gh/andreasfertig/cppinsights)
[![Try online](https://img.shields.io/badge/try-online-blue.svg)](https://cppinsights.io)
[![Documentation](https://img.shields.io/badge/view-documentation-blue)](https://docs.cppinsights.io)
[![patreon](https://img.shields.io/badge/patreon-support-orange.svg)](https://www.patreon.com/cppinsights)

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io#https://github.com/andreasfertig/cppinsights)

## Contents

- [What](#what)
- [Why](#why)
- [Building](#building)
- [Usage](#usage)
- [Get Involved](#getinvolved)
- [Support](#support)


## What

[C++ Insights](https://cppinsights.io) is a [clang](https://clang.llvm.org)-based tool which does a source to source 
transformation. Its goal is it to make things visible which normally, and intentionally, happen behind the scenes. 
It's about the magic the compiler does for us to make things work.

Take this piece of code for example:

```.cpp
class Base {
};

class Derived : public Base {
};

int main() {
  Derived d;
  Base& b = d;
}
```

Nothing special and of course it compiles. This is the compilers view on it:

```.cpp
class Base {
/* public: inline constexpr Base() noexcept; */
/* public: inline ~Base(); */
/* public: inline constexpr Base(const Base &); */
/* public: inline constexpr Base(Base &&); */
};

class Derived : public Base {
/* public: inline constexpr Derived() noexcept; */
/* public: inline constexpr Derived(const Derived &); */
/* public: inline constexpr Derived(Derived &&); */
};

int main(){
  Derived d;
  Base& b = static_cast<Base&>(d);
}
```

You can see all the compiler provided functions. Also the downcast from `Derived` to `Base`.

I do not claim to get all the things right. This is just the initial version of insights I consider good enough to hand
it to the public. Keep also in mind that is solely based on clang and its understanding of the AST.

## Why

[C++ Insights](https://cppinsights.io) is a [clang](https://clang.llvm.org)-based tool which does a source to source transformation. 
Its goal is to make things visible which normally, and intentionally, happen behind the scenes. It's about the magic the compiler does 
for us to make things work. Or looking through the classes of a compiler.

Some time ago I started looking into some new things we got with C++11, C++14 and C++17. Amazing things like lambdas, range-based for-loops 
and structured bindings. I put it together in a talk. You can find the [slides](https://www.andreasfertig.info/talks_dl/afertig-ndcolo-2017-fast-and-small.pdf) 
and a [video](https://youtu.be/Bt7KzFxcbgc) online.

However, all that research and some of my training and teaching got me start thinking how it would be, if we could see with the eyes of the 
compiler. Sure, there is an AST-dump at least for clang. With tools like Compiler Explorer we can see what code the compiler generates 
from a C++ source snippet. However, what we see is assembler. Neither the AST nor the Compiler Explorer output is in the language I write 
code and therefore I'm most familiar with. Plus when teaching students C++ showing an AST and explaining that it is all there, was not 
quite satisfying for me.

I started to write a clang-based tool able to transform a range-based for-loop into the compiler-internal version. Then, I did the same 
for structured bindings and lambdas. In the end, I ended up with doing a lot more as initially planned. It shows where operators are 
invoked, places in which the compiler does some casting. C++ Insights is able to deduce the type behind `auto` or `decltype`. The goal 
is to produce compilable code. However, this is not possible in all places.

Still, there is work to do.

I do not claim to get all the things right. This is just the initial version of [C++ Insights](https://cppinsights.io) I consider good 
enough to hand it to the public. Also, keep in mind that it is solely based on clang and my understanding of C++ and the AST.

You can see, for example the transformation of a [lamda](https://cppinsights.io/lnk?code=aW50IG1haW4oKQp7CiAgaW50ICgqZnApKGludCwgY2hhcikgPSBbXShpbnQgYSwgY2hhciBiKXsgcmV0dXJuIGErYjt9Owp9&rev=1.0), [range-based for-loop](https://cppinsights.io/lnk?code=I2luY2x1ZGUgPGNzdGRpbz4KCmludCBtYWluKCkKewogICAgY29uc3QgY2hhciBhcnJbXXsyLDQsNiw4LDEwfTsKCiAgICBmb3IoY29uc3QgY2hhciYgYyA6IGFycikKICAgIHsKICAgICAgcHJpbnRmKCJjPSVjXG4iLCBjKTsKICAgIH0KfQ==&rev=1.0) or [auto](https://cppinsights.io/lnk?code=Y2xhc3MgQ1Rlc3QKewogICAgYXV0byBUZXN0KCkgeyByZXR1cm4gMjI7IH0KfTsKCmF1dG8gVGVzdCgpCnsKICAgIHJldHVybiAxOwp9CgphdXRvIEJlc3QoKSAtPiBpbnQKewogICAgcmV0dXJuIDE7Cn0KCmNvbnN0ZXhwciBhdXRvIENFQmVzdCgpIC0+IGludAp7CiAgICByZXR1cm4gMTsKfQoKZGVjbHR5cGUoYXV0bykgV2VzdCgpCnsKICAgIHJldHVybiAnYyc7Cn0KCmNvbnN0ZXhwciBkZWNsdHlwZShhdXRvKSBDRVdlc3QoKQp7CiAgICByZXR1cm4gJ2MnOwp9CgpbW21heWJlX3VudXNlZF1dIGlubGluZSBjb25zdGV4cHIgZGVjbHR5cGUoYXV0bykgTVVDRVdlc3QoKQp7CiAgICByZXR1cm4gJ2MnOwp9CgoKaW50IG1haW4oKQp7CiAgaW50IHggPSAyOwogIGNvbnN0IGNoYXIqIHA7CiAgY29uc3RleHByIGF1dG8gY2VpID0gMDsKICBhdXRvIGNvbnN0ZXhwciBjZWkyID0gMDsKICBhdXRvIGkgPSAwOwogIGRlY2x0eXBlKGF1dG8pIHhYID0gKGkpOwogIGF1dG8gaWkgPSAmaTsKICBhdXRvJiBpciA9IGk7CiAgYXV0byAqIGlwID0gJmk7CiAgY29uc3QgYXV0byAqIGNpcCA9ICZpOwogIGF1dG8gKiBwcCA9IHA7CiAgY29uc3QgYXV0byAqIGNwID0gcDsKICB2b2xhdGlsZSBjb25zdCBhdXRvICogdmNwID0gcDsKICBhdXRvIGYgPSAxLjBmOwogIGF1dG8gYyA9ICdjJzsKICBhdXRvIHUgPSAwdTsKICBkZWNsdHlwZSh1KSB1dSA9IHU7CgogIFtbbWF5YmVfdW51c2VkXV0gYXV0byBtdSA9IDB1OwogIFtbbWF5YmVfdW51c2VkXV0gZGVjbHR5cGUodSkgbXV1ID0gdTsKfQ==&rev=1.0). Of course, you can transform any other C++ snippet.

See yourself, C++ Insights is available online: [cppinsights.io](https://cppinsights.io).

## Building

C++ Insights can be build inside the clang-source tree or outside.

### Building on Windows

See [Readme_Windows.md](Readme_Windows.md)

### Building outside clang

You need to have a clang installation in the search path.

```
git clone https://github.com/andreasfertig/cppinsights.git
mkdir build && cd build
cmake -G"Ninja" ../cppinsights
ninja
```
The resulting binary (insights) can be found in the build-folder.

### Building inside clang

For building it inside the clang-source tree, assuming you have your source-tree already prepared:

```
cd llvm/tools/clang/tools/extra
git clone https://github.com/andreasfertig/cppinsights.git

echo "add_subdirectory(cppinsights)" >> CMakeLists.txt
```

Then build clang as you normally do.

### cmake options

There are a couple of options which can be enable with [cmake](https://cmake.org):

| Option              | Description                | Default |
|---------------------|:---------------------------| --------|
| INSIGHTS_STRIP      | Strip insight after build  | ON      |
| INSIGHTS_STATIC     | Use static linking         | OFF     |
| INSIGHTS_COVERAGE   | Enable code coverage       | OFF     |
| INSIGHTS_USE_LIBCPP | Use libc++ for tests       | OFF     |
| DEBUG               | Enable debug               | OFF     |


### Use it with [Cevelop](https://www.cevelop.com)

```
git clone https://github.com/andreasfertig/cppinsights.git
mkdir build_eclipse
cd build_eclipse
cmake -G"Eclipse CDT4 - Unix Makefiles" ../cppinsights/
```

Then in [Cevelop](https://www.cevelop.com) Import -> General -> Existing Project into Workspace. Select `build_eclipse`. Enjoy editing with
[Cevelop](https://www.cevelop.com).

## Usage

Using C++ Insights is fairly simple: 

```
insights <YOUR_CPP_FILE> -- -std=c++17
```

Things get complicated when it comes to the system include paths. There are path hard-coded in the binary which seem
to come from the compiler C++ Insights was build with. To help with that check out [scripts/getinclude.py](scripts/getinclude.py). It tries to
collect the system include paths from the compiler. Without an option it uses `g++`, you can also pass another compiler
as a first argument.

Here is an example:

```
./scripts/getinclude.py 
-isystem/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../include/c++/v1 -isystem/usr/local/include -isystem/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/7.3.0/include -isystem/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include -isystem/usr/include 
```

The script can be used together with C++ Insights:

```
insights <YOUR_CPP_FILE> -- -std=c++17 `./scripts/getinclude.py`
```


There is also another GitHub project which sets up a docker container with the latest C++ Insights version in it: [C++
Insights - Docker](https://github.com/andreasfertig/cppinsights-docker)


## Compatibility

Currently, the most recent stable version of clang is supported as well as the current development branch. 

## ToDo's

See [TODO](TODO.md).


## Get Involved
+ Report bugs/issues by submitting a [GitHub issue](https://github.com/andreasfertig/cppinsights/issues).
+ Submit contributions using [pull requests](https://github.com/andreasfertig/cppinsights/pulls). See [Contributing](CONTRIBUTING.md)

## Support

If you like to support the project consider [submitting](CONTRIBUTING.md) a patch. Another alternative is to become a [Patreon](https://www.patreon.com/cppinsights) supporter.

