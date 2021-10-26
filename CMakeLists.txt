
cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

string(
        CONCAT
        PXTIDB_DESCRIPTION
        "px_tidb is a relational DBMS designed from tidb implemented in c++."
        "px_tidb is for study"
)

project(
        px_tidb
        VERSION 0.0.1
        DESCRIPTION "${PXTIDB_DESCRIPTION}"
        HOMEPAGE_URL "https://xxx.com/"
        # px_tidb is a C++ project.
        LANGUAGES CXX
)

# Create a compile_commands.json file that can be easily parsed by build tools, clang-tidy, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#######################################################################################################################
# HEADER Safety checks.
#######################################################################################################################

# People keep running CMake in the wrong folder, completely nuking their project or creating weird bugs.
# This checks if you're running CMake from a folder that already has CMakeLists.txt.
# Importantly, this catches the common case of running it from the root directory.
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" PATH_TO_CMAKELISTS_TXT)
if (EXISTS "${PATH_TO_CMAKELISTS_TXT}")
    message(FATAL_ERROR "Run CMake from a build subdirectory! \"mkdir build ; cd build ; cmake ..\" \
    Some junk files were created in this folder (CMakeCache.txt, CMakeFiles); you should delete those.")
endif ()

# Print a welcome message with the project's version number.
message(STATUS
        "Welcome to pxtidb!\n\
        Home Page: ${PROJECT_HOMEPAGE_URL}\n\
        Version: ${PROJECT_VERSION}\n")

# CMAKE_MODULE_PATH is the search path for the include() and find_package() CMake commands.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules")

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}")


function(print_sys_info QUERY_TARGET)
    cmake_host_system_information(RESULT SYS_INFO QUERY ${QUERY_TARGET})
    message(STATUS "  System ${QUERY_TARGET}: ${SYS_INFO}")
endfunction()
print_sys_info("NUMBER_OF_LOGICAL_CORES;NUMBER_OF_PHYSICAL_CORES")
print_sys_info("HOSTNAME;FQDN")
print_sys_info("AVAILABLE_VIRTUAL_MEMORY;TOTAL_VIRTUAL_MEMORY")
print_sys_info("AVAILABLE_PHYSICAL_MEMORY;TOTAL_PHYSICAL_MEMORY")
print_sys_info("IS_64BIT;HAS_IA64")
print_sys_info("HAS_FPU;HAS_MMX;HAS_MMX_PLUS")
print_sys_info("HAS_SSE;HAS_SSE2;HAS_SSE_FP;HAS_SSE_MMX")
print_sys_info("HAS_AMD_3DNOW;HAS_AMD_3DNOW_PLUS")
print_sys_info("HAS_SERIAL_NUMBER;PROCESSOR_SERIAL_NUMBER")
print_sys_info("PROCESSOR_NAME;PROCESSOR_DESCRIPTION")
print_sys_info("OS_NAME;OS_RELEASE;OS_VERSION;OS_PLATFORM")

# Default to DEBUG builds if -DCMAKE_BUILD_TYPE was not specified.
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

option(PXTIDB_BUILD_BENCHMARKS
        "Enable building benchmarks as part of the ALL target."
        OFF)

option(PXTIDB_BUILD_TESTS
        "Enable building tests as part of the ALL target."
        OFF)

option(PXTIDB_GENERATE_COVERAGE
        "Enable C++ code coverage."
        OFF)

set(PXTIDB_TEST_PARALLELISM
        "1"
        CACHE STRING "The maximum number of tests that can be run in parallel at a time. Warning: can cause weird bugs.")

option(PXTIDB_UNITTEST_OUTPUT_ON_FAILURE
        "Verbose output for unittests when they fail. Warning: on jumbo, this is VERY verbose!"
        OFF)

option(PXTIDB_UNITY_BUILD
        "Enable Unity builds for much faster compilation. https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html"
        OFF)

option(PXTIDB_USE_ASAN
        "Enable ASAN, a fast memory error detector. https://clang.llvm.org/docs/AddressSanitizer.html"
        OFF)

option(PXTIDB_USE_JEMALLOC
        "Link jemalloc instead of system malloc. https://github.com/jemalloc/jemalloc"
        OFF)

option(PXTIDB_USE_JUMBOTESTS
        "Enable jumbotests instead of unittest as part of ALL target."
        OFF)

option(PXTIDB_USE_LOGGING
        "Enable logging. When enabled, there is a performance hit for all logging calls even if nothing is logged."
        ON)

set(BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build-support")
set(BUILD_SUPPORT_DATA_DIR "${CMAKE_SOURCE_DIR}/build-support/data")

# Everything else in this section will populate the following global variables.
set(PXTIDB_COMPILE_OPTIONS "")
set(PXTIDB_LINK_LIBRARIES "")
set(PXTIDB_LINK_OPTIONS "")
set(PXTIDB_INCLUDE_DIRECTORIES "")


# Add compilation flags to PXTIDB_COMPILE_OPTIONS based on the current CMAKE_BUILD_TYPE.
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-ggdb" "-O0" "-fno-omit-frame-pointer" "-fno-optimize-sibling-calls")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "FASTDEBUG")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-ggdb" "-O1" "-fno-omit-frame-pointer" "-fno-optimize-sibling-calls")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    list(APPEND PXTIDB_COMPILE_DEFINITIONS "-DNDEBUG")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-O3")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RELWITHDEBINFO")
    list(APPEND PXTIDB_COMPILE_DEFINITIONS "-DNDEBUG")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-ggdb" "-O2")
else ()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif ()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Coverage.
if (${PXTIDB_GENERATE_COVERAGE})
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")                     # If coverage is required on non-DEBUG builds,
        message(FATAL_ERROR "Coverage requires a debug build type!")    # Then error out.
    endif ()
    list(APPEND PXTIDB_COMPILE_OPTIONS "--coverage")     # Compile with coverage (compilers alias this).
    list(APPEND PXTIDB_LINK_OPTIONS "--coverage")        # Link coverage libraries (compilers alias this).
endif ()
message(STATUS "Coverage: ${PXTIDB_GENERATE_COVERAGE}")

# ASAN, which includes LSAN.
if (${PXTIDB_USE_ASAN})
    set(PXTIDB_ASAN_FLAGS
            "-fsanitize=address"                # Enable ASAN.
            "-fno-omit-frame-pointer"           # Nicer stack traces in error messages.
            "-fno-optimize-sibling-calls"       # Disable tail call elimination (perfect stack traces if inlining off).
            )
    list(APPEND PXTIDB_COMPILE_OPTIONS ${PXTIDB_ASAN_FLAGS})
    list(APPEND PXTIDB_LINK_OPTIONS "-fsanitize=address")
    unset(PXTIDB_ASAN_FLAGS)
    # Unfortunately, gcc does not support -fsanitize-blacklist. We ask users to set it manually as an env var.
endif ()
message(STATUS "ASAN: ${PXTIDB_USE_ASAN}")
unset(PXTIDB_ASAN_MSG)

# jemalloc.
set(PXTIDB_JEMALLOC_MSG "${PXTIDB_USE_JEMALLOC}")
if (${PXTIDB_USE_JEMALLOC})
    # We find jemalloc from the system to avoid building jemalloc from scratch.
    find_path(JEMALLOC_INCLUDE_DIR NAMES jemalloc/jemalloc.h REQUIRED)
    find_library(JEMALLOC_LIBRARIES NAMES jemalloc libjemalloc.so.1 libjemalloc.so.2 libjemalloc.dylib REQUIRED)
    list(APPEND PXTIDB_LINK_LIBRARIES ${JEMALLOC_LIBRARIES})         # Add to pxtidb link libs.
    list(APPEND PXTIDB_INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE_DIR})  # Add to pxtidb includes.
    unset(JEMALLOC_INCLUDE_DIR)                                         # Variable hygiene.
    unset(JEMALLOC_LIBRARIES)                                           # Variable hygiene.
    set(PXTIDB_JEMALLOC_MSG "On (dir:${JEMALLOC_INCLUDE_DIR} lib:${JEMALLOC_LIBRARIES})")
endif ()
message(STATUS "jemalloc: ${PXTIDB_JEMALLOC_MSG}")
unset(PXTIDB_JEMALLOC_MSG)

# spdlog.
if (${PXTIDB_USE_LOGGING})
    list(APPEND PXTIDB_COMPILE_DEFINITIONS "-DPXTIDB_USE_LOGGING")
endif ()
message(STATUS "Logging: ${PXTIDB_USE_LOGGING}")

message(STATUS "Verbose unit tests (PXTIDB_UNITTEST_OUTPUT_ON_FAILURE): ${PXTIDB_UNITTEST_OUTPUT_ON_FAILURE}")
message(STATUS "Unity builds (PXTIDB_UNITY_BUILD): ${PXTIDB_UNITY_BUILD}")
message(STATUS "Test max parallelism: ${PXTIDB_TEST_PARALLELISM} tests at a time.")

# OS specific configuration.
if (APPLE)
    # On OSX, clang complains about this.
    list(APPEND PXTIDB_COMPILE_OPTIONS "-Wno-braced-scalar-init")
    message(STATUS "OSX hack, PXTIDB_COMPILE_OPTIONS: adding -Wno-braced-scalar-init")
    # On OSX, using lld causes a linking error because -lz (zlib) cannot be found.
    message(STATUS "OSX hack, Linker: Will use default system linker.")
    # On OSX, LLVM is compiled with -fvisibility=hidden.
    list(APPEND PXTIDB_COMPILE_OPTIONS "-fvisibility=hidden")
    list(APPEND PXTIDB_LINK_OPTIONS "-fvisibility=hidden")
else ()
    # lld is the fastest linker.
    list(APPEND PXTIDB_LINK_OPTIONS "-fuse-ld=lld")
    message(STATUS "Linker: Will pass \"-fuse-ld=lld\" to linker.")
endif ()

# Compiler specific configuration.
# ASAN_OPTIONS=suppressions=... does not work because we compile third party libraries ourselves for the most part.
# Since the issues tend to manifest only on clang builds, we can use the clang-specific -fsanitize-blacklist= option.
# GCC devs refuse to add the option, so eh. Fingers crossed.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Clang hack, PXTIDB_COMPILE_OPTIONS: adding -fsanitize-blacklist=${BUILD_SUPPORT_DATA_DIR}/asan_sanitize_blocklist.txt.")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-fsanitize-blacklist=${BUILD_SUPPORT_DATA_DIR}/asan_sanitize_blocklist.txt")
endif ()

# The goal is to dynamically link tests to reduce filesize. However,
#   1. The OSX linker is a huge pain.
#   2. Clang has a bug with dynamically linked ASAN https://github.com/google/sanitizers/issues/1017
#      and the workaround no longer works.
#   3. Coverage builds are very slow and every little bit of speed helps.
# In those scenarios, we disable dynamic linking entirely and rely on static linking.
if (APPLE OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR ${PXTIDB_GENERATE_COVERAGE})
    set(PXTIDB_ENABLE_SHARED OFF)
    message(STATUS "Shared targets: Removed.")
    message(STATUS "Tests: Will use static linking.")
else ()
    set(PXTIDB_ENABLE_SHARED ON)
    message(STATUS "Shared targets: Available.")
    message(STATUS "Tests: Will use dynamic linking.")
endif ()

# From pmenon: since Ninja buffers output from the compiler, GCC/Clang won't generate colored text.
# Explicitly request colorized compiler output.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-fdiagnostics-color=always")
    message(STATUS "Colorized output: ON (-fdiagnostics-color=always)")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND PXTIDB_COMPILE_OPTIONS "-fcolor-diagnostics")
    message(STATUS "Colorized output: ON (-fcolor-diagnostics)")
else ()
    message(STATUS "Colorized output: OFF (unknown compiler ${CMAKE_CXX_COMPILER_ID})")
endif ()

if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
    # Num Logical Cores = Compile Pool + Link Pool (1) + 1
    # The extra 1 at the end is there for good luck.
    # A pool controls the _maximum_ parallelism of the jobs assigned to the pool.

    # Force single-threaded linking because linking can consume a ton of memory.
    cmake_host_system_information(RESULT NINJA_COMPILE_POOL_SIZE QUERY "NUMBER_OF_LOGICAL_CORES")
    set(NINJA_LINK_POOL_SIZE 1)
    math(EXPR NINJA_COMPILE_POOL_SIZE "${NINJA_COMPILE_POOL_SIZE} - ${NINJA_LINK_POOL_SIZE} - 1")
    set_property(GLOBAL PROPERTY JOB_POOLS link_pool=${NINJA_LINK_POOL_SIZE} compile_pool=${NINJA_COMPILE_POOL_SIZE})
    # Set linking and compiling limits globally for all targets by modifying the default pool options.
    set(CMAKE_JOB_POOL_COMPILE compile_pool)
    set(CMAKE_JOB_POOL_LINK link_pool)
    message(STATUS "Ninja detected. Global pools: link=${NINJA_LINK_POOL_SIZE} compile=${NINJA_COMPILE_POOL_SIZE}")
endif()

include(FetchContent)                   # FetchContent_ functions.
find_package(PkgConfig REQUIRED)        # pkg_search_module function.

# This pair of VARS_BEFORE() and VARS_AFTER() macros helps you to figure out what variables external code has defined.
# For example, suppose you want to include the nlohmann_json library with
#       find_package(nlohmann_json REQUIRED).
# What variables did it define? Is there a version number in there? Who knows?
#       VARS_BEFORE()
#       find_package(nlohmann_json REQUIRED).
#       VARS_AFTER()
# And now you know! This is useful if you're adding new libraries and want to print useful messages.

macro(VARS_BEFORE)
    get_directory_property(_vars_before VARIABLES)              # Save the current variables to _vars_before.
endmacro()

macro(VARS_AFTER)
    get_directory_property(_vars_after VARIABLES)               # Save the current variables to _vars_after.
    list(REMOVE_ITEM _vars_after _vars_before ${_vars_before})  # Diff _vars_after and _vars_before.
    foreach (_var IN LISTS _vars_after)                         # Print out all the variables that remain.
        message(STATUS "${_var} = ${${_var}}")
    endforeach ()
endmacro()

# Some libraries just won't shut up, so this redefines the message function.
set(MESSAGE_QUIET OFF)
function(message)
    list(GET ARGV 0 MessageType)
    if (NOT MESSAGE_QUIET OR
            MessageType STREQUAL FATAL_ERROR OR
            MessageType STREQUAL SEND_ERROR OR
            MessageType STREQUAL WARNING OR
            MessageType STREQUAL AUTHOR_WARNING)
        list(GET ARGV 1 Message)

        # Special-casing for messages to ignore.
        set(IGNORED_MESSAGES
                ""
                # Google Benchmark.
                "CMake's FindThreads.cmake did not fail, but CMAKE_THREAD_LIBS_INIT ended up being empty. This was fixed in https://github.com/Kitware/CMake/commit/d53317130e84898c5328c237186dbd995aaf1c12 Let's guess that -pthread is sufficient."
                )
        if (Message IN_LIST IGNORED_MESSAGES)
            return()
        endif ()

        list(REMOVE_AT ARGV 0)
        _message(${MessageType} "${ARGV}")
    endif ()
endfunction()

function(add_pxtidb_dep NAME GIT_URL GIT_TAG)
    string(TOLOWER "${NAME}" NAME_LOWER)    # The automatically created variables are lowercase. Footgun!
    FetchContent_Declare(${NAME})           # Declare the resource to be fetched.
    if (NOT ${NAME}_POPULATED)
        # The long form of FetchContent_Populate is used since pxtidb requires control of the source folder name.
        # This is to enable includes like #include "NAME/their_files.h".
        FetchContent_Populate(${NAME}
                QUIET                                                   # Don't print verbose output while populating.
                SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/src/${NAME}        # Source files go here.
                BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/build/${NAME}      # Build files go here.
                SUBBUILD_DIR ${CMAKE_BINARY_DIR}/_deps/sub/${NAME}      # Sub-build files go here.
                GIT_REPOSITORY ${GIT_URL}                               # Download from GIT_URL.
                GIT_TAG ${GIT_TAG}                                      # Checkout the specific GIT_TAG version.
                )

        # Whenever custom behavior is required, override with pxtidb third_party/deps_override/NAME_CMakeLists.txt.
        set(DEP_CUSTOM_CMAKELISTS ${PROJECT_SOURCE_DIR}/third_party/deps_override/${NAME}_CMakeLists.txt)
        if (EXISTS ${DEP_CUSTOM_CMAKELISTS})
            file(COPY ${DEP_CUSTOM_CMAKELISTS} DESTINATION ${${NAME_LOWER}_SOURCE_DIR})
            file(RENAME ${${NAME_LOWER}_SOURCE_DIR}/${NAME}_CMakeLists.txt ${${NAME_LOWER}_SOURCE_DIR}/CMakeLists.txt)
            message(STATUS "Copied ${DEP_CUSTOM_CMAKELISTS} -> ${${NAME_LOWER}_SOURCE_DIR}/CMakeLists.txt.")
        endif ()
        unset(DEP_CUSTOM_CMAKELISTS)

        set(MESSAGE_QUIET ON)                                                           # Silence third party includes.
        add_subdirectory(${${NAME_LOWER}_SOURCE_DIR} ${${NAME_LOWER}_BINARY_DIR})       # Include the dependency.
        unset(MESSAGE_QUIET)                                                            # Undo silence.
    endif ()

    # Get the date of the last git commit for the GIT_TAG branch.
    execute_process(COMMAND git log -1 --format=%cd --date=short                        # Get the last commit's date.
            WORKING_DIRECTORY ${${NAME_LOWER}_SOURCE_DIR}                               # From the specified git dir.
            OUTPUT_VARIABLE GIT_LAST_COMMIT)                                            # Save to GIT_LAST_COMMIT.
    string(STRIP "${GIT_LAST_COMMIT}" GIT_LAST_COMMIT)                                  # Remove any trailing newlines.

    message(STATUS "[FOUND] ${NAME} (${GIT_URL} ${GIT_TAG} ${GIT_LAST_COMMIT})")        # Print a success message.
endfunction()

function(add_pxtidb_dep_singlefile NAME FILE_URL)
    string(TOLOWER "${NAME}" NAME_LOWER)    # For consistency with add_pxtidb_dep above, also use lowercase.
    get_filename_component(FILE_NAME ${FILE_URL} NAME)                                  # Get the filename.
    set(DOWNLOAD_FOLDER ${CMAKE_BINARY_DIR}/_deps/src/${NAME})                          # Set download folder.
    if (EXISTS ${DOWNLOAD_FOLDER}/${FILE_NAME})                                         # Only download if required.
        message(STATUS "[FOUND] ${NAME} (already in ${DOWNLOAD_FOLDER}, skipping download)")
    else ()
        file(DOWNLOAD ${FILE_URL} ${DOWNLOAD_FOLDER}/${FILE_NAME})                      # Download the file.
        message(STATUS "[FOUND] ${NAME} (${FILE_URL})")                                 # Print a success message.
    endif ()
    set(${NAME_LOWER}_SOURCE_DIR ${DOWNLOAD_FOLDER} PARENT_SCOPE)                       # Set SOURCE_DIR in parent.
endfunction()

#######################################################################################################################
# HEADER Dependencies.
#######################################################################################################################

message(STATUS "\nDependencies: Locating...\n==========================================\n")

# DISGUSTING HACKS:
# - -fPIC is required on pretty much every dependency.
# - fsanitize=address is required on OSX.
set(OLD_CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")     # Save the current CMAKE_BUILD_TYPE.
set(OLD_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")           # Save the current CMAKE_C_FLAGS.
set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")       # Save the current CMAKE_CXX_FLAGS.
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_C_FLAGS "-fPIC")
set(CMAKE_CXX_FLAGS "-fPIC")
if (${PXTIDB_USE_ASAN})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()

message(STATUS "Third-party CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "Third-party CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "Third-party CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# Google Benchmark build settings.
set(BENCHMARK_ENABLE_TESTING OFF)                   # Disable testing of the benchmark library.
set(BENCHMARK_ENABLE_EXCEPTIONS OFF)                # Disable the use of exceptions in the benchmark library.
set(BENCHMARK_ENABLE_LTO OFF)                       # Disable link time optimisation of the benchmark library.
set(BENCHMARK_USE_LIBCXX OFF)                       # Disable building and testing using libc++ as the standard library.
set(BENCHMARK_BUILD_32_BITS OFF)                    # Disable building a 32 bit version of the library.
set(BENCHMARK_ENABLE_INSTALL OFF)                   # Disable installation of benchmark.
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)            # Disable downloading and building google benchmark dependencies.
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)               # Disable building benchmark unit tests.

# gflags build settings.
set(GFLAGS_BUILD_SHARED_LIBS OFF)                   # Disable building gflags shared library.
set(GFLAGS_BUILD_STATIC_LIBS ON)                    # Enable building gflags static library.
set(GFLAGS_BUILD_gflags_LIB ON)                     # Enable building gflags as a library.
set(GFLAGS_BUILD_gflags_nothreads_LIB OFF)          # Disable building the nothreads version of gflags.
set(GFLAGS_BUILD_TESTING OFF)                       # Disable building gflags tests.
set(GFLAGS_BUILD_PACKAGING OFF)                     # Disable building gflags packaging.

# ZeroMQ build settings.
set(CPPZMQ_BUILD_TESTS OFF CACHE INTERNAL "Disable building CPPZMQ tests.")

# Include the source of the dependencies as sources that pxtidb can include.
list(APPEND PXTIDB_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/_deps/src/)

# Fetch single-file dependencies.
add_pxtidb_dep_singlefile(csv https://raw.githubusercontent.com/vincentlaucsb/csv-parser/92694230ba4863a436b6533788e590fce70b5f44/single_include/csv.hpp)
add_pxtidb_dep_singlefile(portable_endian https://gist.githubusercontent.com/panzi/6856583/raw/1eca2ab34f2301b9641aa73d1016b951fff3fc39/portable_endian.h)

# Fetch project dependencies.
add_pxtidb_dep(count https://github.com/pmenon/libcount.git 6eef9d048d4577f144506ffc076c1913f8faf3ef)
add_pxtidb_dep(cppzmq https://github.com/zeromq/cppzmq.git v4.7.1)
add_pxtidb_dep(gflags https://github.com/gflags/gflags.git v2.2.2)
add_pxtidb_dep(googlebenchmark https://github.com/google/benchmark.git v1.5.2)
add_pxtidb_dep(googletest https://github.com/google/googletest.git release-1.10.0)
add_pxtidb_dep(ips4o https://github.com/ips4o/ips4o.git 2fb65ca11ac1898faee2f146610e6409489d2105)
add_pxtidb_dep(madoka https://github.com/s-yata/madoka.git 66783ee5b84a432f934517ad65452d54b19230bb)
add_pxtidb_dep(nlohmann_json https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent.git v3.7.3)
add_pxtidb_dep(spdlog https://github.com/gabime/spdlog.git v1.8.1)
add_pxtidb_dep(xbyak https://github.com/herumi/xbyak.git v5.77)
add_pxtidb_dep(xxHash https://github.com/Cyan4973/xxHash.git v0.8.0)
add_pxtidb_dep(fast_float https://github.com/lemire/fast_float.git v1.0.0)

# Handle other dependencies.

# TODO(WAN): libpg_query is CURSED. Someone else is welcome to fix it. Or I may retry in the future.
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/libpg_query/ EXCLUDE_FROM_ALL)

# libpqxx.
find_library(PQXX_LIBRARIES NAMES pqxx libpqxx REQUIRED)
find_path(PQXX_INCLUDE_DIRECTORIES NAMES pqxx/pqxx REQUIRED)
list(APPEND PXTIDB_LINK_LIBRARIES ${PQXX_LIBRARIES})
list(APPEND PXTIDB_INCLUDE_DIRECTORIES ${PQXX_INCLUDE_DIRECTORIES})
message(STATUS "[FOUND] pqxx (dir:${PQXX_INCLUDE_DIRECTORIES} lib:${PQXX_LIBRARIES})")

# libevent.
pkg_search_module(EVENT REQUIRED libevent)
pkg_search_module(EVENT_PTHREADS REQUIRED libevent_pthreads)

# LLVM 8.0.
#find_package(LLVM 8.0 PATHS /usr/local/opt/llvm/ REQUIRED CONFIG)
#find_package(LLVM  REQUIRED CONFIG)
#message(STATUS "[FOUND] LLVM ${LLVM_PACKAGE_VERSION}")
# Explicitly request all of the LLVM components that we want.
#llvm_map_components_to_libnames(LLVM_LIBRARIES core ipo mcjit nativecodegen native support)

# TBB.
find_package(TBB REQUIRED)
#find_package(TBB PATHS /Users/pxcai/opt/tbb NO_DEFAULT_PATH REQUIRED)
message(STATUS "[FOUND] TBB ${TBB_VERSION}")

# DISGUSTING HACK: Restore the old CMAKE_BUILD_TYPE, CMAKE_C_FLAGS, and CMAKE_CXX_FLAGS.
set(CMAKE_BUILD_TYPE "${OLD_CMAKE_BUILD_TYPE}")     # Restore the old CMAKE_BUILD_TYPE.
set(CMAKE_C_FLAGS "${OLD_CMAKE_C_FLAGS}")           # Restore the old CMAKE_C_FLAGS.
set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")       # Restore the old CMAKE_CXX_FLAGS.
unset(OLD_CMAKE_C_FLAGS)                            # Variable hygiene.
unset(OLD_CMAKE_CXX_FLAGS)                          # Variable hygiene.
unset(OLD_CMAKE_BUILD_TYPE)                         # Variable hygiene.

message(STATUS "\n==========================================\nDependencies: All found!\n")

#######################################################################################################################
# HEADER pxtidb libraries.
# pxtidb_objlib      :   pxtidb object library, built once and linked into both static and shared targets.
# pxtidb_static      :   All of pxtidb functionality exposed as a static library.
# pxtidb_shared      :   All of pxtidb functionality exposed as a shared library.
#######################################################################################################################

# Get the list of all pxtidb sources.
file(GLOB_RECURSE
        PXTIDB_SRCS                  # Store the list of files into the variable ${PXTIDB_SRCS}.
        CONFIGURE_DEPENDS               # See above. Ask CMake to regenerate the build system if these files change.
        ${PROJECT_SOURCE_DIR}/src/*.cc
        ${PROJECT_SOURCE_DIR}/src/include/*.h
#        ${PROJECT_SOURCE_DIR}/third_party/bwtree/*.cpp
#        ${PROJECT_SOURCE_DIR}/third_party/bwtree/*.h
        )
# Remove the main program from pxtidb sources.
list(REMOVE_ITEM PXTIDB_SRCS ${PROJECT_SOURCE_DIR}/src/main/main.cc)

# Build pxtidb as an OBJECT library first, i.e., a .o file per corresponding .cpp file.
# The OBJECT library is built first so that the same .o files can be linked into static and shared libraries.
# This allows both pxtidb_static and pxtidb_shared to be built with a single compilation of translation units.
add_library(pxtidb_objlib OBJECT ${PXTIDB_SRCS})

set_target_properties(pxtidb_objlib PROPERTIES
        POSITION_INDEPENDENT_CODE ON                # Required for static linking into other shared libraries.
        CXX_EXTENSIONS OFF                          # Disable compiler extensions (e.g., use c++20 not gnu17).
        UNITY_BUILD ${PXTIDB_UNITY_BUILD}        # Build multiple cpp files as a single cpp file.
        )
target_compile_definitions(pxtidb_objlib PUBLIC  # PUBLIC: all consumers of the library inherit the following.
        ${PXTIDB_COMPILE_DEFINITIONS}
        )
target_compile_options(pxtidb_objlib PRIVATE     # PRIVATE: only pxtidb_objlib uses the following.
        "-Werror"                                   # Treat warnings as errors.
        "-Wall"                                     # Enable "all" warnings. (Not actually all warnings.)
        )
target_compile_options(pxtidb_objlib PUBLIC      # PUBLIC: all consumers of the library inherit the following.
        "-march=native"                             # Enable machine-specific instruction sets and optimizations.
        "-mcx16"                                    # Allow CMPXCHG16B (16-byte compare and exchange).
        ${PXTIDB_COMPILE_OPTIONS}
        )
target_compile_features(pxtidb_objlib PUBLIC     # PUBLIC: all consumers of the library inherit the following.
        cxx_std_20                                  # Require support for C++20.
        )
target_include_directories(pxtidb_objlib PUBLIC  # PUBLIC: all consumers of the library inherit the following.
        ${PROJECT_SOURCE_DIR}/src/include           # Include NoisePage src/include/ headers.
        )
target_include_directories(pxtidb_objlib SYSTEM PUBLIC   # SYSTEM PUBLIC: inherit without error checking.
        ${LLVM_INCLUDE_DIRS}                                # Include LLVM headers.
        ${PXTIDB_INCLUDE_DIRECTORIES}                    # Third-party includes.
        ${CMAKE_BINARY_DIR}/_deps/src/spdlog/include/       # Hack: spdlog.
        )
target_link_options(pxtidb_objlib PUBLIC         # PUBLIC: all consumers of the library inherit the following.
        ${PXTIDB_LINK_OPTIONS}
        )

# Unfortunately, some libraries do not have clean target_link_libraries hygiene.
# The symptoms are typically weird -Werror warnings because the NoisePage Werror flags get applied to library code.
# The hacky solution is to manually specify their target_include_directories as SYSTEM.
target_include_directories(pxtidb_objlib SYSTEM PUBLIC ${CMAKE_BINARY_DIR}/_deps/src/fast_float/include/)

target_link_libraries(pxtidb_objlib PUBLIC       # PUBLIC: all consumers of the library inherit the following.
        count
        cppzmq
        fast_float
        gflags
        ips4o::ips4o
        madoka::madoka
        nlohmann_json::nlohmann_json
#        pg_query::pg_query
        xbyak::xbyak
        xxHash::xxhash
        ${CMAKE_BINARY_DIR}/_deps/build/spdlog/libspdlog.a
        ${EVENT_LINK_LIBRARIES}
        ${EVENT_PTHREADS_LINK_LIBRARIES}
        ${PXTIDB_LINK_LIBRARIES}
        ${LLVM_LIBRARIES}
        ${TBB_LIBRARIES_RELEASE}
        )

# Create the pxtidb_static and pxtidb_shared libraries using the objects from pxtidb_objlib.
add_library(pxtidb_static STATIC $<TARGET_OBJECTS:pxtidb_objlib>)     # Bundle up these objects into static lib.
target_link_libraries(pxtidb_static PUBLIC pxtidb_objlib)             # Consumers will inherit this link.
target_compile_options(pxtidb_static PUBLIC      # PUBLIC: all consumers of the library inherit the following.
        "-fvisibility=hidden"                       # Hide symbols by default.
        )
target_link_options(pxtidb_static PUBLIC         # PUBLIC: all consumers of the library inherit the following.
        "-fvisibility=hidden"                       # Hide symbols by default.
        )

# Dependencies are built in release because the debug versions have unacceptable performance for coverage builds.
# An example is spdlog. spdlog is not added as a true dependency because the library name changes between
# debug mode and release mode, namely libspdlogd.a versus libspdlog.a.
# Due to the different names, the build system (especially make) gets confused.
# Therefore the dependencies are manually built here.
# However, make is too stupid and ninja is too smart.
# make can't understand that TARGET DEPENDS on something.
# ninja looks for the files in DEPENDS and complains if those can't be found.
if (${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
    add_dependencies(pxtidb_static spdlog)
else()
    add_custom_command(TARGET pxtidb_static DEPENDS spdlog)
endif()

if (${PXTIDB_ENABLE_SHARED})
    add_library(pxtidb_shared SHARED $<TARGET_OBJECTS:pxtidb_objlib>)   # Bundle up these objects into shared lib.
    target_link_libraries(pxtidb_shared PUBLIC pxtidb_objlib)           # Consumers will inherit this link.
endif ()


#######################################################################################################################
# HEADER pxtidb binary.
# pxtidb             :   The main DBMS binary.
#######################################################################################################################

add_executable(pxtidb src/main/main.cc)
target_compile_options(pxtidb PRIVATE "-Werror" "-Wall")
target_link_libraries(pxtidb pxtidb_static)
set_target_properties(pxtidb PROPERTIES CXX_EXTENSIONS OFF ENABLE_EXPORTS ON)

#######################################################################################################################
# HEADER pxtidb miscellaneous files needed for operation.
# replication.config    :   The identities, hostnames, and ports, of all primaries and replicas.
#######################################################################################################################

# replication.config specifies the identities, hostnames, and ports, and is needed when replication is enabled.
add_custom_command(
        TARGET pxtidb
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${BUILD_SUPPORT_DIR}/data/replication.config" "${CMAKE_BINARY_DIR}/bin/replication.config"
)


#######################################################################################################################
# HEADER Tests.
#######################################################################################################################

include(CTest)          # CTest support is built into CMake.
enable_testing()        # CTest support is built into CMake.
include(GoogleTest)     # We rely on GoogleTest for all our tests.
#configure_file(test/CTestCustom.cmake ${PROJECT_BINARY_DIR}/CTestCustom.cmake COPYONLY) # Configure CTest output.

set(UNITTEST_OUTPUT_ON_FAILURE "")
if (${PXTIDB_UNITTEST_OUTPUT_ON_FAILURE})
    set(UNITTEST_OUTPUT_ON_FAILURE "--output-on-failure")
endif ()


#######################################################################################################################
# HEADER Generated file destinations.
#######################################################################################################################

set_target_properties(
        pxtidb_static
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


if (${PXTIDB_ENABLE_SHARED})
    set_target_properties(
            pxtidb_shared
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif ()


#######################################################################################################################
# HEADER Miscellaneous.
# This is where helper scripts and other random CMake stuff goes.
#######################################################################################################################

function(print_property TARGET_NAME PROPERTY_NAME)
    get_target_property(TARGET_NAME_PROPERTY ${TARGET_NAME} ${PROPERTY_NAME})
    if (NOT (TARGET_NAME_PROPERTY STREQUAL "TARGET_NAME_PROPERTY-NOTFOUND"))
        message(STATUS "    ${TARGET_NAME} ${PROPERTY_NAME}: ${TARGET_NAME_PROPERTY}")
    endif ()
endfunction()

function(print_properties TARGET_NAME)
    print_property(${TARGET_NAME} COMPILE_DEFINITIONS)
    print_property(${TARGET_NAME} COMPILE_FEATURES)
    print_property(${TARGET_NAME} COMPILE_OPTIONS)
    print_property(${TARGET_NAME} LINK_OPTIONS)
    print_property(${TARGET_NAME} LINK_LIBRARIES)
endfunction()

message(STATUS "Note that following NoisePage options are just a sample and may not be the full list. Check compile_commands.json to be sure.")

print_properties(pxtidb_objlib)
print_properties(pxtidb)


#######################################################################################################################
# Global variables for helper scripts.
# LINT_FILES    :   All the files that are to be linted. Exposed since other tools may want to use them.
# CLANG_TOOLS_SEARCH_PATH   :   Where to look for clang programs like clang-format and clang-tidy.
#######################################################################################################################

file(GLOB_RECURSE LINT_FILES
        "${PROJECT_SOURCE_DIR}/src/*.hh"
        "${PROJECT_SOURCE_DIR}/src/*.cc"
#        "${PROJECT_SOURCE_DIR}/test/*.hh"
#        "${PROJECT_SOURCE_DIR}/test/*.cc"
        )

set(CLANG_TOOLS_SEARCH_PATH
        "/usr/local/bin"
        "/usr/bin"
        "/usr/local/opt/llvm/bin"
        "/usr/local/opt/llvm@8/bin"
        "/usr/local/Cellar/llvm/8.0.1/bin"
        )

#######################################################################################################################
# check-censored    :   Grep the repository for some bad keywords. Brittle, but cheap and useful.
#######################################################################################################################

# If allowing entire files becomes necessary, do it here. Make it harder so that people try not to do it.
# file(GLOB_RECURSE UNCENSORED_FILES "${PROJECT_SOURCE_DIR}/src/...")
# list(REMOVE_ITEM CENSOR_FILES ${UNCENSORED_FILES})

set(BAD_WORDS_FILE ${BUILD_SUPPORT_DATA_DIR}/bad_words.txt)
if (NOT EXISTS ${BAD_WORDS_FILE})
    message(STATUS "[MISSING] bad words at ${BUILD_SUPPORT_DATA_DIR}/bad_words.txt, no check-censored.")
else ()
    add_custom_target(check-censored                                        # Make the "check-censored" target.
            grep --invert-match -n -e '^ *//' -e '^ *[*]' ${LINT_FILES}     # Check all uncommented lines with line num.
            | grep -f ${BAD_WORDS_FILE}                                     # For bad words, case sensitive.
            | grep --invert-match -e 'NOLINT'                               # And that are not marked as NOLINT.

            # Start of allowlist.

            # Description: allow usage of 'inline' in execution engine.
            # Reason: pmenon 2019/08/13
            # All the bytecode handler functions have to be inline to ensure one definition.
            # gen_opt_bc also cleans up the modules to add the odr to all functions.
            # On top of that, many are marked ALWAYS_INLINE to force physical inlining into the VM.
            # These are hand-selected by Prashanth based on profiling.
#            | grep --invert-match -e 'src/include/execution/.*:.*inline\\b.*'
#            | grep --invert-match -e 'src/execution/ast/context.cpp.*:.*inline\\b.*'
#            | grep --invert-match -e 'src/execution/vm/vm.cpp.*:.*inline\\b.*'

            # Description: Regions, memory pool chunks, and vm stacks are managed using malloc and free.
#            | grep --invert-match -e 'src/execution/sql/memory_pool.cpp:.*malloc\(.*'
#            | grep --invert-match -e 'src/execution/sql/memory_pool.cpp:.*calloc\(.*'
#            | grep --invert-match -e 'src/execution/sql/memory_pool.cpp:.*free\(.*'
#            | grep --invert-match -e 'src/execution/util/region.cpp:.*malloc\(.*'
#            | grep --invert-match -e 'src/execution/util/region.cpp:.*free\(.*'
#            | grep --invert-match -e 'src/execution/vm/vm.cpp:.*free\(.*'

            # End of allowlist.

            || exit 0                                                     # If nothing found, return 0.
            && exit 1                                                     # Else return 1, note || && left-associative.
            USES_TERMINAL
            )
    message(STATUS "[ADDED] check-censored")
endif ()
unset(BAD_WORDS_FILE)

#######################################################################################################################
# check-lint        :   Run the cpplint python script.
#######################################################################################################################

add_pxtidb_dep_singlefile(cpplint https://raw.githubusercontent.com/cpplint/cpplint/5b4259ef4c94d34e98192f53466c8af5e9d1c259/cpplint.py)
execute_process(COMMAND chmod +x "${cpplint_SOURCE_DIR}")
find_program(CPPLINT_BIN NAMES cpplint.py HINTS ${cpplint_SOURCE_DIR})

if ("${CPPLINT_BIN}" STREQUAL "CPPLINT_BIN-NOTFOUND")
    message(STATUS "[MISSING] cpplint at ${cpplint_SOURCE_DIR}/cpplint.py, no check-lint.")
else ()
    # Balancing act: cpplint.py takes a non-trivial time to launch, so process 12 files per invocation with parallelism.
    add_custom_target(check-lint
            COMMENT "Running: echo LINT_FILES | xargs -n12 -P8 python3 ${CPPLINT_BIN} --verbose=2 --linelength=120 --quiet --filter=legal/copyright,-build/header_guard"
            COMMAND echo '${LINT_FILES}' | xargs -n12 -P8
            python3 ${CPPLINT_BIN}
            --verbose=2 --linelength=120 --quiet
            --filter=-legal/copyright,-build/header_guard
            USES_TERMINAL
            )
    message(STATUS "[ADDED] check-lint (${CPPLINT_BIN})")
endif ()
unset(${CPPLINT_BIN})


#######################################################################################################################
# format            :   Reformat the codebase according to standards.
# check-format      :   Check if the codebase is formatted according to standards.
#######################################################################################################################

find_program(CLANG_FORMAT_BIN NAMES clang-format-8 clang-format HINTS ${CLANG_TOOLS_SEARCH_PATH})
if ("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
    message(STATUS "[MISSING] clang-format not found, no format and no check-format.")
else ()
    # The directories to be formatted. Note that we modified the format script to take in multiple arguments.
    string(CONCAT FORMAT_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/benchmark,"
            "${CMAKE_CURRENT_SOURCE_DIR}/src,"
            "${CMAKE_CURRENT_SOURCE_DIR}/test,"
            )

    # Run clang-format and update files in place.
    add_custom_target(format
            ${BUILD_SUPPORT_DIR}/run_clang_format.py
            ${CLANG_FORMAT_BIN}
            ${BUILD_SUPPORT_DATA_DIR}/clangformat_suppressions.txt
            --source_dirs
            ${FORMAT_DIRS}
            --fix
            --quiet
            USES_TERMINAL
            )

    # Run clang-format and exit with a non-zero exit code if any files need to be reformatted.
    add_custom_target(check-format
            ${BUILD_SUPPORT_DIR}/run_clang_format.py
            ${CLANG_FORMAT_BIN}
            ${BUILD_SUPPORT_DATA_DIR}/clangformat_suppressions.txt
            --source_dirs
            ${FORMAT_DIRS}
            --quiet
            USES_TERMINAL
            )

    message(STATUS "[ADDED] clang-format and check-clang-format (${CLANG_FORMAT_BIN})")

    unset(FORMAT_DIRS)
endif ()
unset(CLANG_FORMAT_BIN)


#######################################################################################################################
# Apply +x permissions to all scripts in the build-support folder.
#######################################################################################################################

# CLion's remote toolchain functionality seems to lose the +x permissions when the files are rsync'd to the remote box.
# This creates a list of the executable scripts in the build-support folder and applies +x to them to ensure building
# dependent CMake targets doesn't fail.

file(GLOB_RECURSE
        BUILD_SUPPORT_SCRIPTS
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/build-support/*.pl
        ${PROJECT_SOURCE_DIR}/build-support/*.py
        ${PROJECT_SOURCE_DIR}/build-support/*.sh
        )

foreach (_var IN LISTS BUILD_SUPPORT_SCRIPTS)
    execute_process(COMMAND chmod +x "${_var}")
endforeach ()

