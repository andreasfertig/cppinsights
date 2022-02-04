# Docker Containers Overview {#docker_containers_overview}

Docker containers are used as they provide and excellent way to have a stable base system for deployment or use.
Over the time the number of Docker images has grown and there are now pre-built containers available on DockerHub.

Here is a brief overview of the containers with links to GitHub as well as DockerHub:

* cppinsights-docker-base: [GitHub](https://github.com/andreasfertig/cppinsights-docker-base)/[DockerHub](https://hub.docker.com/r/andreasfertig/cppinsights-docker-base) the base image for all other images. The idea is, that this image defines which LLVM/Clang version is used, installs the smallest common packages and export the version number. All other Docker containers derive from it and can add LLVM/Clang packages if necessary with the matching version number. This system helps to keep the containers and with that the environment stable against update to either Ubuntu or LLVM/Clang.
* cppinsights-builder: [GitHub](https://github.com/andreasfertig/cppinsights-builder)/[DockerHub](https://hub.docker.com/r/andreasfertig/cppinsights-builder) the image used by Travis CI to build C++ Insights. It contains only the minimal packages to achieve this task.
* cppinsights-builder-gitpod: [GitHub](https://github.com/andreasfertig/cppinsights-builder-gitpod)/[DockerHub](https://hub.docker.com/r/andreasfertig/cppinsights-builder-gitpod) the image is used by GitPod and can also be used for local development (see script/Readme.md). It derives from  [cppinsights-builder](https://github.com/andreasfertig/cppinsights-builder) and adds more packages suitable for development like `gdb`.

* cppinsights-webfrontend-container [GitHub](https://github.com/andreasfertig/cppinsights-webfrontend-container)/[DockerHub](https://hub.docker.com/r/andreasfertig/cppinsights-container) encapsulated the C++ Insights [web fronted](https://github.com/andreasfertig/cppinsights-web) and calls the [cppinsights-container](https://github.com/andreasfertig/cppinsights-container) to execute C++ Insights.
* cppinsights-container [GitHub](https://github.com/andreasfertig/cppinsights-container)/[DockerHub](https://hub.docker.com/r/andreasfertig/cppinsights-container) contains a pre-built C++ Insights binary including all the standard libraries required to use it.
 
