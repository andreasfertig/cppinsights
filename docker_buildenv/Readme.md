# C++ Insights - See your source code with the eyes of a compiler.


## Contents

- [What](#what)
- [Why](#why)
- [Building](#building)


## What

This is a Dockerfile and helper script to setup a build environment for [C++ Insights](https://cppinsights.io).


## Building the docker

```
./build.sh
```

This creates a container `cppinsights-dev-env`


## Building C++ Insights in the docker

The `compile.sh` script creates two folders `build_docker` and `docker_clang`. It maps these folder into the machine.
The purpose is to have access to them outside the docker. One use-case is to copy the resulting binary to the
web front-end.

```
./compile.sh
```
