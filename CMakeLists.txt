cmake_minimum_required(VERSION 3.4.3)

option(INSIGHTS_STRIP    "Strip insight after build" On )
option(INSIGHTS_TIDY     "Run clang-tidy"            Off)
option(INSIGHTS_STATIC   "Use static linking"        Off)
option(INSIGHTS_COVERAGE "Enable code coverage"      Off)
option(DEBUG             "Enable debug"              Off)

set(INSIGHTS_LLVM_CONFIG "llvm-config" CACHE STRING "LLVM config executable to use")

set(INSIGHTS_MIN_LLVM_MAJOR_VERSION 7)
set(INSIGHTS_MIN_LLVM_VERSION ${INSIGHTS_MIN_LLVM_MAJOR_VERSION}.0)

if(NOT DEFINED LLVM_VERSION_MAJOR)  # used when build inside the clang tool/extra folder
  set(BUILD_INSIGHTS_OUTSIDE_LLVM On)
endif()

if(INSIGHTS_COVERAGE OR DEBUG)
    set(INSIGHTS_STRIP Off)
endif()

if (BUILD_INSIGHTS_OUTSIDE_LLVM)
    project(cpp-insights)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(IS_GNU On)
else()
  set(IS_GNU Off)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(IS_CLANG On)
else()
    set(IS_CLANG Off)
endif()


if (BUILD_INSIGHTS_OUTSIDE_LLVM)
    function(check_compiler COMPILER version)
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "${COMPILER}")
            if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${version})
                message(FATAL_ERROR "${COMPILER} version must be at least ${version}!")
            endif()

            set(HAVE_COMPILER On PARENT_SCOPE)
        endif()
    endfunction(check_compiler)

    check_compiler("GNU" 7.3)
    check_compiler("Clang" 6.0)

    if(NOT HAVE_COMPILER)
        message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
    endif()


    if(NOT DEFINED INSIGHTS_VERSION_MAJOR)
      set(INSIGHTS_VERSION_MAJOR 0)
    endif()
    if(NOT DEFINED INSIGHTS_VERSION_MINOR)
      set(INSIGHTS_VERSION_MINOR 1)
    endif()
    if(NOT DEFINED INSIGHTS_VERSION_PATCH)
      set(INSIGHTS_VERSION_PATCH 0)
    endif()
    if(NOT DEFINED INSIGHTS_VERSION_SUFFIX)
      set(INSIGHTS_VERSION_SUFFIX git)
    endif()

    if (NOT INSIGHTS_PACKAGE_VERSION)
      set(INSIGHTS_PACKAGE_VERSION "${INSIGHTS_VERSION_MAJOR}.${INSIGHTS_VERSION_MINOR}.${INSIGHTS_VERSION_PATCH}${INSIGHTS_VERSION_SUFFIX}")
    endif()


    # Find LLVM Config binary for LLVM
    # If you're on debian, go to http://llvm.org/apt/ and install the development
    # release of LLVM (all packages).
    find_program(LLVM_CONFIG_PATH "${INSIGHTS_LLVM_CONFIG}")    
    if(NOT LLVM_CONFIG_PATH)
        message(FATAL_ERROR "llvm-config not found -- ${LLVM_CONFIG_PATH}: ${INSIGHTS_LLVM_CONFIG}")
    endif()    

    if(INSIGHTS_STATIC)
        if(APPLE)
            message(STATUS "Static linking may not be possible on OSX")
        else()
            set(LIB_TYPE "--link-static")
        endif()

        set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static" )
    endif()
    
    # This function saves the output of the llvm-config command with the given
    # switch to the variable named VARNAME.
    #
    # Example usage: llvm_config(LLVM_CXXFLAGS "--cxxflags")
    function(llvm_config VARNAME switch)
        set(CONFIG_COMMAND "${LLVM_CONFIG_PATH}" "${switch}")
        
        execute_process(
            COMMAND ${CONFIG_COMMAND} ${LIB_TYPE}
            RESULT_VARIABLE HAD_ERROR
            OUTPUT_VARIABLE CONFIG_OUTPUT
        )
        
        if (HAD_ERROR)
            string(REPLACE ";" " " CONFIG_COMMAND_STR "${CONFIG_COMMAND}")
            message(STATUS "${CONFIG_COMMAND_STR}")
            message(FATAL_ERROR "llvm-config failed with status ${HAD_ERROR}")
        endif()
        
        # replace linebreaks with semicolon
        string(REGEX REPLACE
            "[ \t]*[\r\n]+[ \t]*" ";"
            CONFIG_OUTPUT ${CONFIG_OUTPUT})

        # make all includes system include to prevent the compiler to warn about issues in LLVM/Clang
        string(REGEX REPLACE "-I" "-isystem" CONFIG_OUTPUT "${CONFIG_OUTPUT}")

        # remove certain options clang doesn't like
        if(IS_CLANG)
            string(REGEX REPLACE "-Wl,--no-keep-files-mapped" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-Wl,--no-map-whole-files" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-fuse-ld=gold" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
        elseif(IS_GNU)
            string(REGEX REPLACE "-Wcovered-switch-default" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-Wstring-conversion" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-Werror=unguarded-availability-new" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-Wno-unknown-warning-option" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-Wno-unused-command-line-argument" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
        endif()

        # make result available outside
        set(${VARNAME} ${CONFIG_OUTPUT} PARENT_SCOPE)
        
        # Optionally output the configured value
        message(STATUS "llvm_config(${VARNAME})=>${CONFIG_OUTPUT}")
        
        # cleanup
        unset(CONFIG_COMMAND)
    endfunction(llvm_config)
    
    llvm_config(LLVM_CXXFLAGS "--cxxflags")
    llvm_config(LLVM_LDFLAGS "--ldflags")
    llvm_config(LLVM_LIBS "--libs")
    llvm_config(LLVM_LIBDIR "--libdir")
    llvm_config(LLVM_INCLUDE_DIR "--includedir")
    llvm_config(LLVM_SYSTEM_LIBS "--system-libs")
    llvm_config(LLVM_PACKAGE_VERSION "--version")

    # always generate the compile commands
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    add_definitions(${LLVM_CXXFLAGS})
    add_definitions(-std=c++1z)
    add_definitions(-Wextra)
    add_definitions(-Wold-style-cast)
    add_definitions(-Werror)
    
    if(IS_GNU)
        add_definitions(-Wsuggest-override)
        #        add_definitions(-Wsuggest-final-types)
        add_definitions(-Wuseless-cast)
    elseif(IS_CLANG)
        # -Wno-maybe-uninitialized is a g++ option which is (currently) unknown to clang
        add_definitions(-Wno-unknown-warning-option)
        #add_definitions(-Wnounused-command-line-argument)

    endif()

    if(DEBUG)
        add_definitions(-D INSIGHTS_DEBUG)
        add_definitions(-O0)
    endif()


    include_directories(${LLVM_INCLUDE_DIR} .)
    link_directories(${LLVM_LIBDIR})    

    if(INSIGHTS_COVERAGE)
        message(STATUS "Code coveraging enabled")
        if(IS_GNU)
            set(GCC_COVERAGE_LINK_FLAGS    "-lgcov") 
        endif()

        add_definitions(-g) 
        add_definitions(-O0) 
        add_definitions(--coverage) 
        add_definitions(-fprofile-arcs)
        add_definitions(-ftest-coverage)

        set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} --coverage" )
    endif()

    # copied from: llvm/tools/clang/cmake/modules/AddClang.cmake
    macro(add_clang_tool name)
      add_executable( ${name} ${ARGN} )
    endmacro(add_clang_tool)

    # additional libs required when building insights outside llvm
    set(ADDITIONAL_LIBS         
        ${LLVM_LDFLAGS}
        clangFrontend
        clangDriver
        clangSerialization
        clangParse
        clangSema
        clangAnalysis
        clangEdit
        clangAST
        clangLex
        clangBasic
        clangRewrite
        ${LLVM_LIBS}
        ${LLVM_SYSTEM_LIBS}
    )


elseif(NOT DEFINED LLVM_VERSION_MAJOR)  # used when build inside the clang tool/extra folder
    message(FATAL_ERROR "Neither in LLVM directory nor BUILD_INSIGHTS_OUTSIDE_LLVM is set")    
elseif(NOT DEFINED LLVM_PACKAGE_VERSION)
    # clang seems not to set this variable. Do it ourselves, IF it is not already defined
    set(LLVM_PACKAGE_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}")
endif()

# minimum clang version
if (${LLVM_PACKAGE_VERSION} VERSION_LESS ${INSIGHTS_MIN_LLVM_VERSION})
    message(FATAL_ERROR "LLVM version ${INSIGHTS_MIN_LLVM_VERSION} or higher required. Current version is: ${LLVM_PACKAGE_VERSION}.")
endif()


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND git config --get remote.origin.url
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REPO_URL
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
else()
  set(GIT_REPO_URL "")
  set(GIT_COMMIT_HASH "")
endif()

message(STATUS "Generating version.h")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
)

include_directories(${CMAKE_BINARY_DIR}/generated)


# http://www.mariobadr.com/using-clang-tidy-with-cmake-36.html
find_program( 
  CLANG_TIDY_EXE
  NAMES "clang-tidy"
  DOC "Path to clang-tidy executable"
  )
if(NOT CLANG_TIDY_EXE)
  message(STATUS "clang-tidy not found.")
else()
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
  set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" )
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message(STATUS "Found ccache: ${CCACHE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)

    if(IS_CLANG)
        add_definitions(-Qunused-arguments)        
    endif()
endif()


# name the executable and all source files
add_clang_tool(insights
    AutoStmtHandler.cpp
    CodeGenerator.cpp
    CompilerGeneratedHandler.cpp
    DPrint.cpp
    FunctionDeclHandler.cpp
    GlobalVariableHandler.cpp
    ImplicitCastHandler.cpp
    Insights.cpp
    InsightsBase.cpp
    InsightsHelpers.cpp
    OutputFormatHelper.cpp
    StaticAssertHandler.cpp
    TemplateHandler.cpp
)

# general include also provided by clang-build
target_link_libraries(insights
  PRIVATE
  clangTooling
  clangASTMatchers
  ${ADDITIONAL_LIBS}
)

if(CLANG_TIDY_EXE AND INSIGHTS_TIDY)
  set_target_properties(insights PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif()



# additional includes we need when building outside the llvm-folder
if (BUILD_INSIGHTS_OUTSIDE_LLVM)
    if(INSIGHTS_STRIP)
        if(APPLE)
            # Apple's strip seems to dislike -s even as it is given at strip --help
            set(STRIP_OPTION "")
        else()
            set(STRIP_OPTION "-s")
        endif()

        ADD_CUSTOM_COMMAND(
            TARGET insights
            POST_BUILD
            COMMAND ${CMAKE_STRIP} ${STRIP_OPTION} insights
            COMMENT "Stripping ${TARGET}"
        )
    endif()
endif()



# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif()

# XXX: hack to allow coverage build to run tests which fail
set(TEST_FAILURE_IS_OK "")
if(INSIGHTS_COVERAGE)
    set(TEST_FAILURE_IS_OK "--failure-is-ok")
endif()

# add a target to generate run tests
add_custom_target(tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py --insights ${CMAKE_CURRENT_BINARY_DIR}/insights --cxx ${CMAKE_CXX_COMPILER} ${TEST_FAILURE_IS_OK}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/testSTDIN.sh ${CMAKE_CURRENT_BINARY_DIR}/insights
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    COMMENT "Running tests" VERBATIM
)

# run tests in a docker container
add_custom_target(tests-docker
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py --insights /usr/bin/insights --cxx ${CMAKE_CXX_COMPILER} --docker
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    COMMENT "Running tests in docker" VERBATIM
)

# run tests in a docker container
add_custom_target(docker-build-container
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    COMMENT "Bulding insights in docker" VERBATIM
)

add_custom_target(docker-build
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    DEPENDS docker-build-container ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    COMMENT "Bulding insights in docker" VERBATIM
)

add_custom_target(docker-build-run
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/run.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    DEPENDS docker-build-container ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    COMMENT "Bulding insights in docker" VERBATIM
)

# code coverage
if(INSIGHTS_COVERAGE)
    find_program(LCOV_BIN lcov)
    find_program(GENHTML_BIN genhtml)
    find_package_handle_standard_args(lcov
        REQUIRED_VARS LCOV_BIN GENHTML_BIN
    )
    
    if (NOT LCOV_FOUND)
        message(WARNING "Lcov not found")
    else()
        message(STATUS "Target coverage available")

        add_custom_target(coverage
            COMMAND lcov --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/insights.dir --base-directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/insights.dir --capture --output-file ${CMAKE_CURRENT_BINARY_DIR}/coverage.info
            COMMAND lcov --remove ${CMAKE_CURRENT_BINARY_DIR}/coverage.info "/Applications/*" "/usr/*"  -o ${CMAKE_CURRENT_BINARY_DIR}/filtered.info
            COMMAND genhtml ${CMAKE_CURRENT_BINARY_DIR}/filtered.info --demangle-cpp --output-directory ${CMAKE_CURRENT_BINARY_DIR}/out
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
            COMMENT "Running code coverage analysis" VERBATIM
        )

        add_dependencies(coverage tests)

    endif()
endif()

message(STATUS "")
message(STATUS "[ Build summary ]")
message(STATUS "CMAKE_GENERATOR       : ${CMAKE_GENERATOR}")
message(STATUS "Compiler ID           : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version      : ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Compiler path         : ${CMAKE_CXX_COMPILER}")
message(STATUS "llvm-config           : ${LLVM_CONFIG_PATH}")
message(STATUS "Min LLVM major version: ${INSIGHTS_MIN_LLVM_MAJOR_VERSION}")
message(STATUS "Install path          : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_SOURCE_DIR      : ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR      : ${CMAKE_BINARY_DIR}")
message(STATUS "Git repo url          : ${GIT_REPO_URL}")
message(STATUS "Git commit hash       : ${GIT_COMMIT_HASH}")
message(STATUS "Debug                 : ${DEBUG}")
message(STATUS "Code Coverage         : ${INSIGHTS_COVERAGE}")
message(STATUS "Static linking        : ${INSIGHTS_STATIC}")
message(STATUS "Strip executable      : ${INSIGHTS_STRIP}")
message(STATUS "")


