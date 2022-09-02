cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
# 3.8* is required because of C++17 support

# For better control enable MSVC_RUNTIME_LIBRARY target property
# see https://cmake.org/cmake/help/latest/policy/CMP0091.html
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

include(CMakePrintHelpers)

# TODO: enable those options on Windows once tested
if(NOT WIN32)
    option(DEBUG                        "Enable debug"               Off)
    option(INSIGHTS_STRIP               "Strip insight after build"  On )
    option(INSIGHTS_TIDY                "Run clang-tidy"             Off)
    option(INSIGHTS_IWYU                "Run include-what-you-use"   Off)
    option(INSIGHTS_USE_LIBCPP          "Enable code coverage"       Off)
else()
    set(DEBUG                         Off)
    set(INSIGHTS_STRIP                Off)
    set(INSIGHTS_TIDY                 Off)
    set(INSIGHTS_IWYU                 Off)
    set(INSIGHTS_USE_LIBCPP           Off)
endif()

option(INSIGHTS_USE_SYSTEM_INCLUDES "Elevate to system includes" On )
option(INSIGHTS_COVERAGE            "Enable code coverage"       Off)
option(INSIGHTS_STATIC              "Use static linking"         Off)

set(INSIGHTS_LLVM_CONFIG "llvm-config" CACHE STRING "LLVM config executable to use")

function(check_llvm_version LLVMVERSION version)
    if(${LLVMVERSION} VERSION_LESS ${version})
        message(FATAL_ERROR "${LLVMVERSION} version must be at least ${version}!")
    endif()
endfunction(check_llvm_version)


set(INSIGHTS_MIN_LLVM_MAJOR_VERSION 14)
set(INSIGHTS_MIN_LLVM_VERSION ${INSIGHTS_MIN_LLVM_MAJOR_VERSION}.0)

if(NOT DEFINED LLVM_VERSION_MAJOR)  # used when build inside the clang tool/extra folder
  set(BUILD_INSIGHTS_OUTSIDE_LLVM On)
else()
    check_llvm_version(${LLVM_VERSION_MAJOR} ${INSIGHTS_MIN_LLVM_MAJOR_VERSION})
endif()

if(INSIGHTS_COVERAGE OR DEBUG)
    set(INSIGHTS_STRIP Off)
endif()

if (NOT BUILD_INSIGHTS_OUTSIDE_LLVM AND WIN32)
  message(FATAL_ERROR "Building inside LLVM on Windows was never tested")
endif ()

if (BUILD_INSIGHTS_OUTSIDE_LLVM)
    project(cpp-insights CXX)
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

if(IS_GNU AND WIN32)
    message(FATAL_ERROR "GCC on Windows was never tested")
endif()

# Clang On Windows mimics MSVC, but sometimes we need to know
# if we really have CL.exe (not clang-cl.exe)
if(NOT IS_CLANG AND MSVC)
    set(IS_MSVC_CL 1)
endif()

# cmake_print_variables(
#     CMAKE_CXX_COMPILER_ID
#     BUILD_INSIGHTS_OUTSIDE_LLVM
#     IS_CLANG
#     IS_GNU
#     IS_MSVC_CL
#     MSVC)

if (MSVC)
  # For MSVC, CMake sets certain flags to defaults we want to override.
  # This replacement code is taken from sample in the CMake Wiki at
  # https://gitlab.kitware.com/cmake/community/wikis/FAQ#dynamic-replace.
  foreach (flag_var
         CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
         CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    string(REPLACE "/MD" "" ${flag_var} "${${flag_var}}")

    string(REPLACE "/EHsc" "" ${flag_var} "${${flag_var}}")

    string(REPLACE "/GR" "" ${flag_var} "${${flag_var}}")
  endforeach()
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

    check_compiler("GNU" 11.0)
    check_compiler("Clang" 14.0)
    check_compiler("MSVC" 19.30.0.0)

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

    if(NOT INSIGHTS_PACKAGE_VERSION)
        set(INSIGHTS_PACKAGE_VERSION "${INSIGHTS_VERSION_MAJOR}.${INSIGHTS_VERSION_MINOR}.${INSIGHTS_VERSION_PATCH}${INSIGHTS_VERSION_SUFFIX}")
    endif()


    # Find LLVM Config binary for LLVM
    # If you're on debian, go to http://llvm.org/apt/ and install the development
    # release of LLVM (all packages).
    # If you're on Windows, build LLVM from scratch,
    # see Readme_Windows.md instructions.
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

        if(WIN32)
            add_definitions(/MT)
        else()
            set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static")
        endif()
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

        if(INSIGHTS_USE_SYSTEM_INCLUDES)
            # make all includes system include to prevent the compiler to warn about issues in LLVM/Clang
            if(NOT WIN32)
                string(REGEX REPLACE "-I" "-isystem" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
            else()
                if(IS_MSVC_CL)
                    # See https://devblogs.microsoft.com/cppblog/broken-warnings-theory/
                    string(REGEX REPLACE "-I" "/external:I " CONFIG_OUTPUT "${CONFIG_OUTPUT}")
                endif()
            endif()
        endif()

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

        # Since Clang 10 of ziglang C++14 is set
        string(REGEX REPLACE "-std[:=]+c\\+\\+[0-9][0-9]" "" CONFIG_OUTPUT "${CONFIG_OUTPUT}")

        # make result available outside
        set(${VARNAME} ${CONFIG_OUTPUT} PARENT_SCOPE)

        # Optionally output the configured value
        message(STATUS "llvm_config(${VARNAME})=>${CONFIG_OUTPUT}")

        # cleanup
        unset(CONFIG_COMMAND)
    endfunction(llvm_config)

    llvm_config(LLVM_PACKAGE_VERSION "--version")
    string(REPLACE "git" "" LLVM_PACKAGE_VERSION_PLAIN "${LLVM_PACKAGE_VERSION}")
    check_llvm_version(${LLVM_PACKAGE_VERSION_PLAIN} ${INSIGHTS_MIN_LLVM_MAJOR_VERSION})

    llvm_config(LLVM_CXXFLAGS "--cxxflags")
    llvm_config(LLVM_LDFLAGS "--ldflags")

    llvm_config(LLVM_LIBS2 "--libs")
    if(WIN32)
        # libs list separated with " " (space).
        # CMake does not understand list with spaces,
        # but instead requires ; (semicolon).
        # Use ".lib " as string to replace instead of single " " (space)
        # since LLVM/Clang libraries can be installed into folder
        # with spaces (e.g., C:\Program Files (x86)\LLVM\bin)
        string(REPLACE ".lib " ".lib;" LLVM_LIBS "${LLVM_LIBS2}")
    else()
        set(LLVM_LIBS ${LLVM_LIBS2})
    endif()

    llvm_config(LLVM_LIBDIR "--libdir")
    llvm_config(LLVM_INCLUDE_DIR "--includedir")

    llvm_config(LLVM_SYSTEM_LIBS2 "--system-libs")
    if(WIN32)
        string(REPLACE ".lib " ".lib;" LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS2}")
        # For Win10 (?) some Win API things moved to Mincore.lib.
        # (VerQueryValue, GetFileVersionInfoSize, GetFileVersionInfo)
        # llvm-config does not contain it. Add manually.
        set(LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS};Mincore.lib")
    elseif(APPLE)
        string(REPLACE ".dylib" "" LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS2}")
        string(REPLACE "/usr/lib/lib" "-l" LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS}")
        set(LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS}")
    else()
        set(LLVM_SYSTEM_LIBS ${LLVM_SYSTEM_LIBS2})
    endif()

    # always generate the compile commands
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    if(WIN32)
        # Make compile_commands.json work on Windows.
        # Requires CMake > version 3.15 with fix of
        # https://gitlab.kitware.com/cmake/cmake/issues/17482
        set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
        set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
    endif()

    add_definitions(${LLVM_CXXFLAGS})

    if(NOT WIN32)
        # To many warnings/errors from Clang's internals
        add_definitions(-Wextra)
        add_definitions(-Wold-style-cast)
        add_definitions(-Werror)
    endif ()

    if(IS_GNU AND NOT INSIGHTS_TIDY)
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

    if(WIN32)
        # Ignore deprecated std::iterator<> usage from Clang's sources
        add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
    endif()

    if(IS_MSVC_CL)
        #add_definitions(/Wall)
        #add_definitions(/WX)     # Warnings as errors
        add_definitions(/wd4577) # 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify EHsc
        add_definitions(/wd5045) # spectre mitigation introduced
        add_definitions(/wd4514) # unreferenced inline function removed
        add_definitions(/wd4710) # inline function _not_ inlined
        add_definitions(/wd4711) # selected for automatic inline
        add_definitions(/wd4820) # padding introduced
        add_definitions(/wd4459) # hides global symbol disabled for now
        add_definitions(/wd4626 /wd5027 /wd4623 /wd4625 /wd5026) # implicitly deleted operators
        add_definitions(/wd4061) # enumerator 'clang::CK_BitCast' in switch of enum 'clang::CastKind' is not explicitly handled by a case label
        #add_definitions(/err)

        # To support /external:I:
        # - enables "system headers" and
        # - sets warning level 0 for them
        add_definitions(/experimental:external /external:W0)

        # Use sane and nice C++ for MSVC.
        # This makes alternative tokens (not, and, ...) as actual keywords and
        # enables more conformant C++ in general
        add_definitions(/permissive-)

        # Need to "__cplusplus preprocessor macro to report an updated value for recent C++ language standards
        # support" ...
        add_definitions(/Zc:__cplusplus)

        # llvm-config says /EHs-c-.
        # C++ exception handler used, but unwind semantics are not enabled
        add_definitions(/wd4530)
    endif()

    include_directories(${LLVM_INCLUDE_DIR} .)
    link_directories(${LLVM_LIBDIR})

    if(INSIGHTS_COVERAGE)
        message(STATUS "Code coveraging enabled")
        if(IS_GNU)
            set(COVERAGE_LINK_FLAGS    "-lgcov")
        endif()

	if(WIN32)
            add_definitions(-fprofile-instr-generate)
            set(COVERAGE_LINK_FLAGS "${COVERAGE_LINK_FLAGS} ${LLVM_LIBDIR}/clang/${LLVM_PACKAGE_VERSION_PLAIN}/lib/windows/clang_rt.profile-x86_64.lib /FORCE:MULTIPLE")
	else()
            add_definitions(-g)
            add_definitions(-O0)
#	    add_definitions(-fprofile-arcs)
            add_definitions(-ftest-coverage)
            set(COVERAGE_LINK_FLAGS "${COVERAGE_LINK_FLAGS} --coverage")
	endif()

        add_definitions(--coverage)

        set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_LINK_FLAGS}")
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


if(WIN32)
    # First for the generic no-config case (e.g. with mingw)
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
    # Second, for multi-config builds (e.g. msvc)
        foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
                string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
                set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
                set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
                set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
            endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif()


# clang 8 and later or better ld warn, if the visibility is not the same as for the libs.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

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

# Get the include dir
llvm_config(LLVM_INCLUDE_DIR "--includedir")


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
  message(STATUS "Could not find the program clang-tidy.")

else()
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
  set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" )
endif()


find_program(IWYU_EXE NAMES include-what-you-use iwyu)
if(IWYU_EXE)
    message(STATUS "Found include-what-you-use: ${IWYU_EXE}")
    set(DO_IWYU "${IWYU_EXE}" )
else()
    message(STATUS "Could not find the program include-what-you-use")
endif()

if (NOT WIN32)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        message(STATUS "Found ccache: ${CCACHE_FOUND}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)

        if(IS_CLANG)
            add_definitions(-Qunused-arguments)
        endif()
    endif()
endif()


# name the executable and all source files
add_clang_tool(insights
    CodeGenerator.cpp
    CoroutinesCodeGenerator.cpp
    DPrint.cpp
    FunctionDeclHandler.cpp
    GlobalVariableHandler.cpp
    Insights.cpp
    InsightsBase.cpp
    InsightsHelpers.cpp
    OutputFormatHelper.cpp
    RecordDeclHandler.cpp
    TemplateHandler.cpp
)

if(IS_MSVC_CL)
    # TODO figure out what llvm-config reports and use this configuration
    set_property(TARGET insights PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(CLANG_LINK_CLANG_DYLIB)
    if (NOT LLVM_LINK_LLVM_DYLIB)
        message(FATAL_ERROR "CLANG_LINK_CLANG_DYLIB and LLVM_LINK_LLVM_DYLIB must have the same value.")
    endif()
    target_link_libraries(insights PRIVATE clang-cpp)
else()
    # general include also provided by clang-build
    target_link_libraries(insights
        PRIVATE
        clangTooling
        clangASTMatchers
        ${ADDITIONAL_LIBS}
    )
endif()

if(LLVM_LINK_LLVM_DYLIB)
    if (NOT CLANG_LINK_CLANG_DYLIB)
        message(FATAL_ERROR "CLANG_LINK_CLANG_DYLIB and LLVM_LINK_LLVM_DYLIB must have the same value.")
    endif()
    target_link_libraries(insights PRIVATE LLVM)
endif()

if(CLANG_TIDY_EXE AND INSIGHTS_TIDY)
  set(RUN_CLANG_TIDY On)
  set_target_properties(insights PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
else()
  set(RUN_CLANG_TIDY Off)
endif()

if(IWYU_EXE AND INSIGHTS_IWYU)
    set(RUN_IWYU On)
  set_target_properties(insights PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "${DO_IWYU}")
else()
  set(RUN_IWYU Off)
endif()


install( TARGETS insights RUNTIME DESTINATION bin )

if (NOT WIN32)
    # Not ready for Windows yet
    #

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
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/docs/postProcessDoxygen.py ${CMAKE_CURRENT_BINARY_DIR}/html
        )

        add_dependencies(doc gen-options)
    endif()
endif ()

# XXX: hack to allow coverage build to run tests which fail
set(TEST_FAILURE_IS_OK "")
if(INSIGHTS_COVERAGE OR WIN32)
    set(TEST_FAILURE_IS_OK "--failure-is-ok")
endif()

set(TEST_USE_LIBCPP "")
if(INSIGHTS_USE_LIBCPP)
    set(TEST_USE_LIBCPP "--use-libcpp")
endif()

# Look for Python3, only relevant for target tests. As linux currently ships cmake 3.11.4 we need to workaround
if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    set(Python3_EXECUTABLE "/usr/bin/python")
    set(Python3_FOUND "Yes")
else()
    find_package(Python3 COMPONENTS Interpreter )
endif()

if (NOT Python3_FOUND)
    message(WARNING "Could not find the program Python3. Target tests disabled.")
else()
    message(STATUS "Found Python3: ${Python3_EXECUTABLE}. Target tests enabled.")

    # add a target to generate run tests
    add_custom_target(tests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py --insights ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:insights> --cxx ${CMAKE_CXX_COMPILER} ${TEST_FAILURE_IS_OK} ${TEST_USE_LIBCPP}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/testSTDIN.sh ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:insights>
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:insights> ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
        COMMENT "Running tests" VERBATIM
    )
endif()

if (NOT WIN32)
    add_custom_target(update-tests
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py --insights ${CMAKE_CURRENT_BINARY_DIR}/insights --cxx ${CMAKE_CXX_COMPILER} --update-tests ${TEST_FAILURE_IS_OK}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/testSTDIN.sh ${CMAKE_CURRENT_BINARY_DIR}/insights
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
        COMMENT "Running tests" VERBATIM
    )


    # run tests in a docker container
    add_custom_target(docker-tests
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh ${CMAKE_CURRENT_BINARY_DIR}/docker_build compile "-DINSIGHTS_STATIC=${INSIGHTS_STATIC} -DINSIGHTS_COVERAGE=${INSIGHTS_COVERAGE} -DDEBUG=${DEBUG} -DINSIGHTS_USE_LIBCPP=${INSIGHTS_USE_LIBCPP}" tests
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        COMMENT "Running tests in docker" VERBATIM
    )

    # run code coverage in docker container
    add_custom_target(docker-coverage
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh ${CMAKE_CURRENT_BINARY_DIR}/docker_build compile "-DINSIGHTS_STATIC=${INSIGHTS_STATIC} -DINSIGHTS_COVERAGE=Yes -DDEBUG=${DEBUG} -DINSIGHTS_USE_LIBCPP=${INSIGHTS_USE_LIBCPP}" coverage-html
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        COMMENT "Running tests in docker" VERBATIM
    )

    # run tests in a docker container
    add_custom_target(docker-build
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh ${CMAKE_CURRENT_BINARY_DIR}/docker_build compile "-DINSIGHTS_STATIC=${INSIGHTS_STATIC} -DINSIGHTS_COVERAGE=${INSIGHTS_COVERAGE} -DDEBUG=${DEBUG} -DINSIGHTS_TIDY=${INSIGHTS_TIDY} -DINSIGHTS_USE_LIBCPP=${INSIGHTS_USE_LIBCPP}"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        COMMENT "Bulding insights in docker" VERBATIM
    )

    # open a shell in the docker container
    add_custom_target(docker-build-run
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh ${CMAKE_CURRENT_BINARY_DIR}/docker_build shell
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker-shell.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        COMMENT "Open shell in docker" VERBATIM
    )

    if(CLANG_TIDY_EXE AND INSIGHTS_TIDY)
        add_custom_command(TARGET insights
            POST_BUILD
            COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/clang-tidy-check.sh ${CMAKE_CURRENT_BINARY_DIR}/output.txt ${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy-ignore
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Analyzing clang-tidy output..."
        )
    endif()

    add_subdirectory(docs)
endif()


# code coverage
if(INSIGHTS_COVERAGE)
    if(WIN32)
        find_program(GRCOV_BIN grcov REQUIRED)

        if(NOT GRCOV_BIN)
            message(FATAL_ERROR "grcov not found")
        else()
            message(STATUS "Found grcov: ${GRCOV_BIN}. Target coverage enabled.")

            llvm_config(LLVM_BIN_DIR "--bindir")
            get_filename_component(CLANG_ABS_DIR "../" ABSOLUTE BASE_DIR "${LLVM_BIN_DIR}")
            message(STATUS "${CLANG_ABS_DIR}")

            add_custom_target(coverage
                COMMAND ${GRCOV_BIN} -s ${CMAKE_CURRENT_SOURCE_DIR} --llvm --ignore "C:\\Program*" --ignore "${CLANG_ABS_DIR}/*" --ignore-not-existing ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/insights.dir -t lcov -o ${CMAKE_CURRENT_BINARY_DIR}/filtered.info
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
                COMMENT "Running code coverage analysis" VERBATIM
            )

            add_custom_target(coverage-html
                COMMAND ${GRCOV_BIN} -s ${CMAKE_CURRENT_SOURCE_DIR} --llvm --ignore "${CMAKE_CURRENT_BINARY_DIR}/../current/*" --ignore-not-existing ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/insights.dir -t html -o ${CMAKE_CURRENT_BINARY_DIR}/out
                COMMENT "Generating code coverage HTML" VERBATIM
            )
        endif()
    else()
        find_program(LCOV_BIN lcov)
        find_program(GENHTML_BIN genhtml)
        find_package_handle_standard_args(lcov
            REQUIRED_VARS LCOV_BIN GENHTML_BIN
        )


        if (NOT LCOV_FOUND)
            message(FATAL_ERROR "Lcov not found")
        else()
            message(STATUS "Target coverage available")
            message(STATUS "lcov                     : ${LCOV_BIN}")

            set(GCOV_TOOL "")
            if(IS_GNU)
                # Find the major version of the CXX compiler to use the corresponding gcov with it
                string(REPLACE "." ";" CXX_VERSION_LIST "${CMAKE_CXX_COMPILER_VERSION}")
                list(GET CXX_VERSION_LIST 0 CXX_MAJOR_VERSION)
                set(GCOV_TOOL --gcov-tool /usr/bin/gcov-${CXX_MAJOR_VERSION})
            else()
                #find_program(GCOV_BIN gcov PATHS /usr/local/bin NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_DEFAULT_PATH)
                #find_package_handle_standard_args(gcov REQUIRED_VARS GCOV_BIN)

                #if (NOT GCOV_FOUND)
                #    message(FATAL_ERROR "gcov not found")
                #endif()

                #                set(GCOV_TOOL "--gcov-tool ${GCOV_BIN}")
                set(GCOV_TOOL --gcov-tool /usr/local/bin/gcov-10)
            endif()

            message(STATUS "gcov_tool                : ${GCOV_TOOL}")


            add_custom_target(coverage
                COMMAND lcov --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/insights.dir ${GCOV_TOOL} --base-directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/insights.dir --capture --output-file ${CMAKE_CURRENT_BINARY_DIR}/coverage.info
                COMMAND lcov --remove ${CMAKE_CURRENT_BINARY_DIR}/coverage.info "/Applications/*" "/usr/*" "/Users/runner/work/cppinsights/cppinsights/current/*" "./current/*" "./build/*" "./cmake*/*" "${CMAKE_CURRENT_BINARY_DIR}" "./cppinsights/docs/*" "./cppinsights/tests/*" -o ${CMAKE_CURRENT_BINARY_DIR}/filtered.info
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/insights ${CMAKE_CURRENT_SOURCE_DIR}/tests/runTest.py
                COMMENT "Running code coverage analysis" VERBATIM
            )

            add_custom_target(coverage-html
                COMMAND genhtml ${CMAKE_CURRENT_BINARY_DIR}/filtered.info --demangle-cpp --output-directory ${CMAKE_CURRENT_BINARY_DIR}/out
                COMMENT "Generating code coverage HTML" VERBATIM
            )

        endif()
    endif()

    add_dependencies(coverage tests)
    add_dependencies(coverage-html coverage)

endif()

message(STATUS "")
message(STATUS "[ Build summary ]")
message(STATUS "CMAKE_GENERATOR       : ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_LINKER          : ${CMAKE_LINKER}")
message(STATUS "Compiler ID           : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version      : ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Compiler path         : ${CMAKE_CXX_COMPILER}")
message(STATUS "llvm-config           : ${LLVM_CONFIG_PATH}")
message(STATUS "Min LLVM major version: ${INSIGHTS_MIN_LLVM_MAJOR_VERSION}")
message(STATUS "Install path          : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Clang resource dir    : ${INSIGHTS_CLANG_RESOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR      : ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR      : ${CMAKE_BINARY_DIR}")
message(STATUS "Git repo url          : ${GIT_REPO_URL}")
message(STATUS "Git commit hash       : ${GIT_COMMIT_HASH}")
message(STATUS "Debug                 : ${DEBUG}")
message(STATUS "Code Coverage         : ${INSIGHTS_COVERAGE}")
message(STATUS "Static linking        : ${INSIGHTS_STATIC}")
message(STATUS "Strip executable      : ${INSIGHTS_STRIP}")
message(STATUS "Elevate includes:     : ${INSIGHTS_USE_SYSTEM_INCLUDES}")
message(STATUS "clang-tidy            : ${RUN_CLANG_TIDY}")
message(STATUS "include-what-you-use  : ${RUN_IWYU}")
message(STATUS "")

