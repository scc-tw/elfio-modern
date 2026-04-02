cmake_minimum_required(VERSION 3.20)

project(elfio-modern VERSION 1.1.3 LANGUAGES C CXX)

# --- Options ---
option(ELFIO_BUILD_TESTS    "Build elfio-modern tests"              ON)
option(ELFIO_BUILD_EXAMPLES "Build elfio-modern examples"           ON)
option(ELFIO_FETCH_GTEST    "Fetch Google Test via FetchContent"   OFF)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Build Tools ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# --- Compiler Warnings ---
set(ELFIO_WARNINGS
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Woverloaded-virtual -Wunused
        -Wnull-dereference -Wdouble-promotion
        -Wformat=2 -Wimplicit-fallthrough>
    $<$<CXX_COMPILER_ID:GNU>:
        -Wduplicated-cond -Wduplicated-branches -Wlogical-op>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX>
)

# --- Library Target ---
add_library(elfio-modern INTERFACE)
add_library(elfio::elfio-modern ALIAS elfio-modern)

target_include_directories(
    elfio-modern
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(elfio-modern INTERFACE ${ELFIO_WARNINGS})

# --- Dependencies (Google Test) ---
if(ELFIO_BUILD_TESTS)
    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        if(ELFIO_FETCH_GTEST)
            message(STATUS "GTest not found — fetching via FetchContent")
            include(FetchContent)
            FetchContent_Declare(
              googletest
              URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
            )
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(googletest)
        else()
            message(WARNING
                "GTest not found and ELFIO_FETCH_GTEST is OFF — "
                "tests disabled. Pass -DELFIO_FETCH_GTEST=ON to auto-download, "
                "or install GTest system-wide.")
            set(ELFIO_BUILD_TESTS OFF)
        endif()
    endif()

    if(ELFIO_BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

if(ELFIO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# --- Install ---
include(GNUInstallDirs)
install(TARGETS elfio-modern EXPORT elfio-modern-targets)
install(DIRECTORY include/elfio DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT elfio-modern-targets
    FILE elfio-modern-config.cmake
    NAMESPACE elfio::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/elfio-modern)
