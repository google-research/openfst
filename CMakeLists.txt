# Copyright 2026 The OpenFst Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.22)

# Set project version
project(
  openfst
  VERSION 1.8.4
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Shared library versioning
set(OPENFST_SOVERSION 26)

# Options
option(OPENFST_BUILD_TESTS "Enable OpenFst tests" ON)
option(OPENFST_ENABLE_INSTALL "Enable installation of OpenFst" OFF)
option(OPENFST_USE_SYSTEM_ABSEIL
       "Use installed Abseil-cpp instead of downloading" OFF
)
option(OPENFST_USE_SYSTEM_GTEST
       "Use installed GoogleTest instead of downloading" OFF
)
option(OPENFST_USE_SYSTEM_BENCHMARK
       "Use installed Google Benchmark instead of downloading" OFF
)

# Component Enable/Disable Options
option(OPENFST_ENABLE_FSTS "Enable all FST extensions" OFF)
option(OPENFST_ENABLE_GRM "Enable all dependencies of OpenGrm" OFF)

# Helper defaults based on meta-flags
set(_def_fsts OFF)
if(OPENFST_ENABLE_FSTS)
  set(_def_fsts ON)
endif()

set(_def_grm OFF)
if(OPENFST_ENABLE_GRM)
  set(_def_grm ON)
endif()

set(_def_ngram OFF)
if(OPENFST_ENABLE_FSTS OR OPENFST_ENABLE_GRM)
  set(_def_ngram ON)
endif()

option(OPENFST_ENABLE_BIN "Enable fst::script and executables" ON)
option(OPENFST_ENABLE_COMPACT_FSTS "Enable CompactFst extensions" ${_def_fsts})
option(OPENFST_ENABLE_COMPRESS "Enable compression extension" ${_def_fsts})
option(OPENFST_ENABLE_CONST_FSTS "Enable ConstFst extensions" ${_def_fsts})
option(OPENFST_ENABLE_FAR "Enable FAR extensions" ${_def_grm})
option(OPENFST_ENABLE_LINEAR_FSTS
       "Enable LinearTagger/ClassifierFst extensions" ${_def_fsts}
)
option(OPENFST_ENABLE_LOOKAHEAD_FSTS "Enable LookAheadFst extensions"
       ${_def_fsts}
)
option(OPENFST_ENABLE_MPDT "Enable MPDT extensions" ${_def_grm})
option(OPENFST_ENABLE_NGRAM_FSTS "Enable NGramFst extensions" ${_def_ngram})
option(OPENFST_ENABLE_PDT "Enable PDT extensions" ${_def_grm})
option(OPENFST_ENABLE_PYTHON "Enable Python extension" OFF)
option(OPENFST_ENABLE_SPECIAL "Enable special-matcher extensions" ${_def_fsts})

# Installation requires System Abseil to avoid vendoring/hijacking conflicts.
if(OPENFST_ENABLE_INSTALL AND NOT OPENFST_USE_SYSTEM_ABSEIL)
  message(
    FATAL_ERROR
      "OPENFST_ENABLE_INSTALL requires OPENFST_USE_SYSTEM_ABSEIL to be ON."
      "OpenFst cannot install its own Abseil copy. Please install Abseil"
      "separately or disable installation (-DOPENFST_ENABLE_INSTALL=OFF)."
  )
endif()

# Python extension depends on FAR
if(OPENFST_ENABLE_PYTHON)
  set(OPENFST_ENABLE_FAR ON CACHE BOOL "Enable FAR extensions" FORCE)
endif()

# Compiler Warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(
    -Wall
    -Wno-deprecated-declarations
    # TODO: b/476056201 - This warning is enabled in Google, so unclear why it
    # is failing here.  Investigate, fix, and re-enable.
    -Wno-overloaded-virtual
    -Wno-sign-compare
    # Temporarily disabled warnings that we would like to turn on.
    -Wno-defaulted-function-deleted
    -Wno-nullability-completeness
  )
endif()

# Linker options.
if(APPLE)
  # There is a dependency cycle that the macOS linker cannot resolve regarding
  # where the abseil flags are defined versus where they are used. The macOS
  # linker is strict and does not allow undefined symbols in shared libraries
  # by default. Below we relax this requirement.
  # Append the flag ONLY for shared library targets.
  set(
    CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup"
  )
endif()

include(FetchContent)
include(GNUInstallDirs)

# Helper macro for conditional installation
function(openfst_install)
  if(OPENFST_ENABLE_INSTALL)
    install(${ARGV})
  endif()
endfunction()

# Abseil
if(OPENFST_USE_SYSTEM_ABSEIL)
  find_package(absl REQUIRED)
else()
  # Bundled Abseil: Only used when NOT installing OpenFst (e.g. build-only or
  # subproject). We disable installation and tests for the bundled copy.
  set(ABSL_ENABLE_INSTALL OFF)
  set(ABSL_PROPAGATE_CXX_STD ON)
  set(ABSL_BUILD_TESTING OFF)

  FetchContent_Declare(
    abseil-cpp
    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20260107.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(abseil-cpp)
endif()

if(OPENFST_BUILD_TESTS)
  enable_testing()

  # GoogleTest
  if(OPENFST_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
  else()
    set(INSTALL_GTEST OFF)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
  endif()

  # Google Benchmark
  if(OPENFST_USE_SYSTEM_BENCHMARK)
    find_package(benchmark REQUIRED)
  else()
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    FetchContent_Declare(
      google_benchmark
      URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(google_benchmark)
  endif()
endif()

# OpenFst
add_subdirectory(openfst)

# Installation of the export set
openfst_install(
  EXPORT OpenFstTargets
  FILE OpenFstTargets.cmake
  NAMESPACE OpenFst::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenFst
)

# License
openfst_install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  OpenFstConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenFstConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/OpenFstConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenFst
)

openfst_install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenFstConfig.cmake
                      ${CMAKE_CURRENT_BINARY_DIR}/OpenFstConfigVersion.cmake
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenFst
)
