cmake_minimum_required(VERSION 3.4.3)

option(INSIGHTS_STRIP    "Strip insight after build" ON )
option(INSIGHTS_TIDY     "Run clang-tidy"            OFF)
option(INSIGHTS_STATIC   "Use static linking"        OFF)
option(INSIGHTS_COVERAGE "Enable code coverage"      OFF)
option(DEBUG             "Enable debug"              OFF)

set(INSIGHTS_LLVM_CONFIG "llvm-config" CACHE STRING "LLVM config executable to use")

if(NOT DEFINED LLVM_VERSION_MAJOR)  # used when build inside the clang tool/extra folder
  set(BUILD_INSIGHTS_OUTSIDE_LLVM 1)
endif()

if (BUILD_INSIGHTS_OUTSIDE_LLVM)
    project(cpp-insights)

    function(check_compiler COMPILER version)
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "${COMPILER}")
            if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${version})
                message(FATAL_ERROR "${COMPILER} version must be at least ${version}!")
            endif()

            set(HAVE_COMPILER 1 PARENT_SCOPE)
        endif()
    endfunction(check_compiler)

    check_compiler("GNU" 7.3)
    check_compiler("Clang" 6.0)
    check_compiler("AppleClang" 7.0)

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
    else()
        message(STATUS "Found LLVM_CONFIG_PATH: ${LLVM_CONFIG_PATH}")
    endif()    

    if(INSIGHTS_STATIC)
        if(APPLE)
            message(STATUS "Static linking may no be possible on OSX")
        else()
            set(LIB_TYPE "--link-static")
            message(STATUS "Use static linking")
        endif()
    endif(INSIGHTS_STATIC)
    
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
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            string(REGEX REPLACE "-Wl,--no-keep-files-mapped" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-Wl,--no-map-whole-files" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            string(REGEX REPLACE "-fuse-ld=gold" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
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


    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    
    if(INSIGHTS_COVERAGE)
        message(STATUS "Code coveraging enabled")
        add_definitions(-fprofile-arcs)
        add_definitions(-ftest-coverage)
    endif(INSIGHTS_COVERAGE)

    # always generate the compile commands
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    add_definitions(${LLVM_CXXFLAGS})
    add_definitions(-std=c++1z)
    add_definitions(-Wextra)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_definitions(-Wsuggest-override)
        #        add_definitions(-Wsuggest-final-types)
        add_definitions(-Wuseless-cast)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # -Wno-maybe-uninitialized is a g++ option which is (currently) unknown to clang
        add_definitions(-Wno-unknown-warning-option)
        #add_definitions(-Wnounused-command-line-argument)

    endif()
    add_definitions(-Wold-style-cast)
    add_definitions(-Werror)

    if(DEBUG)
        message(STATUS "Debug enabled")
        add_definitions(-D INSIGHTS_DEBUG)
        add_definitions(-O0)
    endif(DEBUG)

    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
      execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      execute_process(
        COMMAND git config --get remote.origin.url
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REPO_URL
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  
    else()
      set(GIT_REPO_URL "")
      set(GIT_COMMIT_HASH "")
    endif()

    message(STATUS "Git repo url: ${GIT_REPO_URL}")
    message(STATUS "Git commit hash: ${GIT_COMMIT_HASH}")
    message(STATUS "Generating version.h")

    configure_file(
      ${CMAKE_SOURCE_DIR}/version.h.in
      ${CMAKE_BINARY_DIR}/generated/version.h
    )

    include_directories(${CMAKE_BINARY_DIR}/generated)

    include_directories(${LLVM_INCLUDE_DIR} .)
    link_directories(${LLVM_LIBDIR})

    

    if(INSIGHTS_COVERAGE)
        set(INSIGHTS_STRIP Off)

        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            SET(GCC_COVERAGE_LINK_FLAGS    "-lgcov") 
        endif()

        add_definitions(-g -O0 --coverage -fprofile-arcs -ftest-coverage) 

        SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} --coverage" )
    endif(INSIGHTS_COVERAGE)

    if(INSIGHTS_STATIC)
        SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static" )
    endif()

    # copied from: llvm/tools/clang/cmake/modules/AddClang.cmake
    macro(add_clang_executable name)
      add_executable( ${name} ${ARGN} )
    endmacro(add_clang_executable)

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

set(INSIGHTS_MIN_LLVM_VERSION 6.0)

# minimum clang version
if (${LLVM_PACKAGE_VERSION} VERSION_LESS ${INSIGHTS_MIN_LLVM_VERSION})
    message(FATAL_ERROR "LLVM version ${INSIGHTS_MIN_LLVM_VERSION} or higher required. Current version is: ${LLVM_PACKAGE_VERSION}.")
endif()

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

# name the executable and all source files
add_clang_executable(insights
    AutoStmtHandler.cpp
    ClassOperatorHandler.cpp
    CodeGenerator.cpp
    CommaOperatorHandler.cpp
    CompilerGeneratedHandler.cpp
    DPrint.cpp
    IfStmtHandler.cpp
    ImplicitCastHandler.cpp
    Insights.cpp
    InsightsBase.cpp
    InsightsHelpers.cpp
    LambdaHandler.cpp
    NRVOHandler.cpp
    OutputFormatHelper.cpp
    RangeForStmtHandler.cpp
    StaticAssertHandler.cpp
    StaticHandler.cpp
    StdInitializerListHandler.cpp
    StructuredBindingsHandler.cpp
    TemplateHandler.cpp
    UserDefinedLiteralHandler.cpp
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

        message(STATUS "Stripping executable")
        ADD_CUSTOM_COMMAND(
            TARGET insights
            POST_BUILD
            COMMAND ${CMAKE_STRIP} ${STRIP_OPTION} insights
            COMMENT "Stripping ${TARGET}"
        )
    endif()

    install(TARGETS insights DESTINATION /home/afertig/tmp)

    #        ADD_CUSTOM_COMMAND(
    #            TARGET insights
    #            POST_BUILD
    #            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py ${CMAKE_CURRENT_BINARY_DIR}/insights
    #            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    #            COMMENT "tests ${TARGET}"
    #        )
    ## Prepare "Catch" library for other executables
    #set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/catch)
    #add_library(Catch INTERFACE)
    #target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})
    #
    #    add_definitions(-fexceptions)
    #    add_definitions(-DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS=1)


    ## Make test executable
    #set(TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/ca.cpp)
    #add_executable(tests ${TEST_SOURCES})
    #target_link_libraries(tests Catch)
    #
    #enable_testing(true)  # Enables unit-testing.
    #
    #add_test(NAME RunTests COMMAND tests)
    #
    #add_test(NAME ListTests COMMAND tests --list-tests)
    #set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")
    #
    #add_test(NAME ListTags COMMAND tests --list-tags)
    #set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")

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
endif(DOXYGEN_FOUND)

# add a target to generate run tests
add_custom_target(tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py --insights ${CMAKE_CURRENT_BINARY_DIR}/insights --cxx ${CMAKE_CXX_COMPILER}
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
    #    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    COMMENT "Bulding insights in docker" VERBATIM
)

add_custom_target(docker-build
    #    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    DEPENDS docker-build-container ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    #    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    COMMENT "Bulding insights in docker" VERBATIM
)

add_custom_target(docker-build-run
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/run.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
    DEPENDS docker-build-container ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/build.sh ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/Dockerfile ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv/compile.sh
    #    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docker_buildenv
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

    endif (NOT LCOV_FOUND)
endif()


add_custom_target(format
    COMMAND clang-format -i *.cpp
    COMMAND clang-format -i *.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running clang-format" VERBATIM
)

message(STATUS "Using CXX: ${CMAKE_CXX_COMPILER}")
