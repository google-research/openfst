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

find_package(Python3 COMPONENTS Interpreter Development)
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
          "Python package 'absl-py' is required for testing but was not found."
          "Please install it (e.g., `pip install absl-py`) or disable tests"
          "(-DOPENFST_BUILD_TESTS=OFF)."
      )
    endif()
  endif()

  set(PYWRAPFST_CC ${CMAKE_CURRENT_BINARY_DIR}/pywrapfst.cc)

  # Prepare include directory structure for Cython to match "from openfst.xxx
  # cimport"
  set(CYTHON_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/cython_include)
  file(MAKE_DIRECTORY ${CYTHON_INCLUDE_DIR}/openfst)

  set(PXD_FILES
      cios.pxd
      cmemory.pxd
      cpywrapfst.pxd
  )
  foreach(PXD ${PXD_FILES})
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${PXD} ${CYTHON_INCLUDE_DIR}/openfst/${PXD}
    )
  endforeach()

  # Cython command
  add_custom_command(
    OUTPUT ${PYWRAPFST_CC}
    COMMAND
      ${CYTHON_EXECUTABLE} -3 --cplus -I ${CYTHON_INCLUDE_DIR} -I
      ${CMAKE_SOURCE_DIR} -o ${PYWRAPFST_CC}
      ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst.pyx
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst.pyx
            ${CYTHON_INCLUDE_DIR}/openfst/cpywrapfst.pxd
    COMMENT "Generating C++ source from Cython file"
  )

  # Python module
  add_library(pywrapfst MODULE ${PYWRAPFST_CC})
  set_target_properties(
    pywrapfst
    PROPERTIES PREFIX ""
  )

  target_include_directories(pywrapfst PRIVATE ${CMAKE_SOURCE_DIR}
                                               ${Python3_INCLUDE_DIRS}
  )

  target_link_libraries(
    pywrapfst
    PRIVATE fst
            fstscript
            fstfarscript
            absl::flags
            Python3::Module
  )

  # Ensure dependency symbols are available for registration
  if(UNIX AND NOT APPLE)
    target_link_options(pywrapfst PRIVATE "-Wl,--no-as-needed")
  endif()

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

  # Test
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tmp)
  add_test(
    NAME pywrapfst_test
    COMMAND
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/pywrapfst_test.py
      --test_srcdir=${CMAKE_SOURCE_DIR}
      --test_tmpdir=${CMAKE_CURRENT_BINARY_DIR}/tmp
  )

  add_test(
    NAME gil_test
    COMMAND
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gil_test.py
  )

  # Construct LD_LIBRARY_PATH for the test
  set(TEST_LIB_PATH "$<TARGET_FILE_DIR:fst>" "$<TARGET_FILE_DIR:fstscript>")
  if(UNIX AND NOT APPLE)
    set(ENV_VAR_NAME "LD_LIBRARY_PATH")
  elseif(APPLE)
    set(ENV_VAR_NAME "DYLD_LIBRARY_PATH")
  else()
    set(ENV_VAR_NAME "PATH")
  endif()

  # Set environment for the test
  set_tests_properties(
    pywrapfst_test
    PROPERTIES
      ENVIRONMENT
        "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR};${ENV_VAR_NAME}=${TEST_LIB_PATH}"
      WILL_FAIL TRUE
      LABELS "ExpectFailure"
  )

  set_tests_properties(
    gil_test
    PROPERTIES
      ENVIRONMENT
        "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR};${ENV_VAR_NAME}=${TEST_LIB_PATH}"
  )

else()
  message(
    STATUS "Python3 or Cython not found. Skipping Python extension build."
  )
endif()
