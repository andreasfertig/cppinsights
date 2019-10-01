# C++ Insights - scripts {#readme_scripts}

This folder mostly hosts scripts for [Travis CI](https://travis-ci.org/) builds, with a few exceptions:

## docker-shell.sh

For local build you can use `docker-shell.sh`. It uses a pre-built Docker image from DockerHub 
[andreasfertig/cppinsights-builder-gitpod](https://cloud.docker.com/repository/docker/andreasfertig/cppinsights-builder-gitpod). 
This is also the same image which is used by [GitPod](https://www.gitpod.io) and a addition to the
[cppinsights-builder](https://cloud.docker.com/repository/docker/andreasfertig/cppinsights-builder) image which is used
by [Travis CI](https://travis-ci.org/) for the continuous integration builds. If you execute the script from within the
`script` folder it automatically mounts the C++ Insights repo in the Docker to `/workspace/cppinsights`. You also need
to specify a build folder which is inside the container mounted to `/workspace/build`.

The script can be invoked with two different modes:
* `shell` launches a `bash` in the Docker container. This allows to use `gdb` for example.
* `compile` does a complete C++ Insights build. When using compile you can provide two additional arguments. 
    * A string with CMake flags used to configure the build
    * and optional target to invoke. For example, `coverage` to run a code coverage build.

Some examples command line looks like this:

```
cd scripts
./docker-shell.sh /home/username/build/docker_insights compile "-DINSIGHTS_STATIC=Yes -DINSIGHTS_COVERAGE=Yes -DDEBUG=Yes" coverage
```

it triggers a CMake build with code coverage turned on and used as the `ninja` target.

```
cd scripts
./docker-shell.sh /home/username/build/docker_insights shell
```

Opens a shell into the Docker container.


### CMake and Docker

There are some CMake target which use Docker and the `docker-shell.sh` script. The results can be found in the binary folder you supplied plus
`docker_build`,

* `docker-build` to run a build in Docker.
* `docker-build-run` gives you a shell in Docker (same as invoking `docker-shell.sh` with `shell`).
* `docker-tests` to run the tests in Docker
* `docker-coverage` to run a code coverage build in Docker.


## `getinclude.py`

Helps for some corner cases to get the default includes from a compiler.
