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

if(NOT BUILD_SHARED_LIBS)
  message(
    FATAL_ERROR
      "OPENFST_ENABLE_PYTHON requires BUILD_SHARED_LIBS=ON. "
      "Static builds drop or duplicate OpenFst and Abseil registries across "
      "dynamically loaded FST plugins and downstream Python extensions. "
      "Please enable shared libraries (-DBUILD_SHARED_LIBS=ON) or disable "
      "the Python extension (-DOPENFST_ENABLE_PYTHON=OFF)."
  )
endif()

find_package(Python3 COMPONENTS Interpreter Development.Module)
find_program(CYTHON_EXECUTABLE NAMES cython3 cython)

if(Python3_FOUND AND CYTHON_EXECUTABLE)
  message(STATUS "Found Python3: ${Python3_EXECUTABLE}")
  message(STATUS "Found Cython: ${CYTHON_EXECUTABLE}")

  # Check for absl-py if testing is enabled.
  if(OPENFST_BUILD_TESTS)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -c "import absl"
      RESULT_VARIABLE ABSL_PY_EXIT_CODE
      OUTPUT_QUIET ERROR_QUIET
    )
    if(ABSL_PY_EXIT_CODE)
      message(
        FATAL_ERROR
          "Python package 'absl-py' is required for testing but was not found. "
          "Please install it (e.g., `pip install absl-py`) or disable tests "
          "(-DOPENFST_BUILD_TESTS=OFF)."
      )
    endif()
  endif()

  set(PYWRAPFST_CC ${CMAKE_CURRENT_BINARY_DIR}/pywrapfst.cc)

  # Cython command
  add_custom_command(
    OUTPUT ${PYWRAPFST_CC}
    COMMAND ${CYTHON_EXECUTABLE} -3 --cplus -I ${PROJECT_SOURCE_DIR} -o
            ${PYWRAPFST_CC} ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst.pyx
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst.pyx
            ${CMAKE_CURRENT_SOURCE_DIR}/cios.pxd
            ${CMAKE_CURRENT_SOURCE_DIR}/cmemory.pxd
            ${CMAKE_CURRENT_SOURCE_DIR}/cpywrapfst.pxd
            ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst.pxd
    COMMENT "Generating C++ source from Cython file"
  )

  # Python module. When built as a PEP 517 wheel via `scikit-build-core` (e.g.,
  # as part of Pynini wheels), prefer its `SKBUILD_SOABI` over CMake's detected
  # `Python3_SOABI` for PyPy and cross-compilation compatibility.
  if(SKBUILD_SOABI)
    set(Python3_SOABI "${SKBUILD_SOABI}")
  endif()
  Python3_add_library(pywrapfst MODULE WITH_SOABI ${PYWRAPFST_CC})

  target_include_directories(pywrapfst PRIVATE ${PROJECT_SOURCE_DIR})

  target_link_libraries(
    pywrapfst
    PRIVATE fst
            fstscript
            fstfarscript
            absl::flags
  )

  # Prepare package structure for testing
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/openfst)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/openfst/__init__.py
       "from .pywrapfst import *\n"
  )

  add_custom_command(
    TARGET pywrapfst
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pywrapfst>
            ${CMAKE_CURRENT_BINARY_DIR}/openfst/
    COMMENT "Copying pywrapfst to openfst/ package directory"
  )

  # Use relative path to respect CMAKE_INSTALL_PREFIX
  set(PYTHON_INSTALL_DIR
      lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages
  )

  openfst_install(TARGETS pywrapfst DESTINATION ${PYTHON_INSTALL_DIR}/openfst)
  openfst_install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openfst/__init__.py
                  DESTINATION ${PYTHON_INSTALL_DIR}/openfst
  )

  if(OPENFST_BUILD_TESTS)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tmp)
    # `pywrapfst_test.py` resolves `testdata` relative to `.`, so run from
    # `${PROJECT_SOURCE_DIR}` where `openfst/*/testdata` lives.
    add_test(
      NAME pywrapfst_test
      COMMAND
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst_test.py
        --test_tmpdir=${CMAKE_CURRENT_BINARY_DIR}/tmp
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_test(
      NAME gil_test
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gil_test.py
    )

    # Prepend the Python module and library build directories to the test
    # environment without overwriting caller paths.
    if(APPLE)
      set(ENV_VAR_NAME "DYLD_LIBRARY_PATH")
    elseif(UNIX)
      set(ENV_VAR_NAME "LD_LIBRARY_PATH")
    else()
      set(ENV_VAR_NAME "PATH")
    endif()

    set(TEST_ENV_MODS
        "PYTHONPATH=path_list_prepend:${CMAKE_CURRENT_BINARY_DIR}"
        "${ENV_VAR_NAME}=path_list_prepend:$<TARGET_FILE_DIR:fstfar>"
        "${ENV_VAR_NAME}=path_list_prepend:$<TARGET_FILE_DIR:fstfarscript>"
        "${ENV_VAR_NAME}=path_list_prepend:$<TARGET_FILE_DIR:fstscript>"
        "${ENV_VAR_NAME}=path_list_prepend:$<TARGET_FILE_DIR:fst>"
    )
    set_tests_properties(
      pywrapfst_test gil_test
      PROPERTIES ENVIRONMENT_MODIFICATION "${TEST_ENV_MODS}"
    )
  endif()

else()
  message(
    STATUS "Python3 or Cython not found. Skipping Python extension build."
  )
endif()
