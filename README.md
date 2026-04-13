![OpenFst Logo](docs/images/logo.svg)

# OpenFst Library

[![GitHub license](https://img.shields.io/badge/license-Apache2-blue.svg)](https://github.com/google-research/nisaba/blob/main/LICENSE)
[![Bazel (x64 Linux)](https://github.com/google-research/openfst/actions/workflows/bazel_x64_linux.yml/badge.svg)](https://github.com/google-research/openfst/actions/workflows/bazel_x64_linux.yml)
[![CMake (x64 Linux)](https://github.com/google-research/openfst/actions/workflows/cmake_x64_linux.yml/badge.svg)](https://github.com/google-research/openfst/actions/workflows/cmake_x64_linux.yml)
[![Bazel (arm64 macOS)](https://github.com/google-research/openfst/actions/workflows/bazel_arm64_macos.yml/badge.svg)](https://github.com/google-research/openfst/actions/workflows/bazel_arm64_macos.yml)
[![CMake (arm64 macOS)](https://github.com/google-research/openfst/actions/workflows/cmake_arm64_macos.yml/badge.svg)](https://github.com/google-research/openfst/actions/workflows/cmake_arm64_macos.yml)
[![Bazel (x64 Windows)](https://github.com/google-research/openfst/actions/workflows/bazel_x64_windows.yml/badge.svg)](https://github.com/google-research/openfst/actions/workflows/bazel_x64_windows.yml)

This library is for constructing, combining, optimizing, and searching *weighted
finite-state transducers* (FSTs). Weighted finite-state transducers are automata
where each transition has an input label, an output label, and a
[weight](http://www.openfst.org/twiki/bin/view/FST/FstQuickTour#FstWeights). The
more familiar finite-state acceptor is represented as a transducer with each
transition's input and output label equal. Finite-state acceptors are used to
represent sets of strings (specifically, *regular* or *rational sets*);
finite-state transducers are used to represent binary relations between pairs of
strings (specifically, *rational transductions*). The weights can be used to
represent the cost of taking a particular transition.

FSTs have key applications in speech recognition and synthesis, machine
translation, optical character recognition, pattern matching, string processing,
machine learning, information extraction and retrieval among others. Often a
weighted transducer is used to represent a probabilistic model (e.g., an *n-gram
model*, *pronunciation model*). FSTs can be optimized by
*[determinization](http://www.openfst.org/twiki/bin/view/FST/DeterminizeDoc)*
and *minimization*, models can be applied to hypothesis sets (also represented
as automata) or cascaded by finite-state
*[composition](http://www.openfst.org/twiki/bin/view/FST/ComposeDoc)*, and the
best results can be selected by
*[shortest-path](http://www.openfst.org/twiki/bin/view/FST/ShortestPathDoc)*
algorithms.

This library was developed by contributors from
[Google Research](https://research.google/) and
[NYU's Courant Institute](https://cims.nyu.edu/). It is intended to be
comprehensive, reliable, flexible, efficient, and to scale well.

Please see https://www.openfst.org for extensive documentation.

*   [Background Material](docs/background.md)
*   [Quick Tour](docs/quick_tour.md)
*   [Creating FSTs](docs/quick_tour.md#creatingfsts)
*   [Accessing FSTs](docs/quick_tour.md#accessingfsts)
*   [FST Operations](docs/quick_tour.md#fstoperations)
*   [Calling Operations](docs/quick_tour.md#operationcalling)
*   [Example -- FST Application](docs/quick_tour.md#operationexample)
*   [Available Operations](docs/quick_tour.md#availableoperations)
*   [FST Weights](docs/quick_tour.md#fstweights)
*   [Advanced Usage](docs/advanced_usage.md)
*   [Conventions](docs/conventions.md)
*   [Extensions](docs/extensions.md)
*   [Efficiency](docs/efficiency.md)
*   [Glossary](docs/glossary.md)
*   [Documented Source Code](https://www.openfst.org/doxygen/fst/html/)
*   [Forum](https://openfst.org/twiki/bin/view/Forum/FstForum)
*   [Contributed and related projects](https://openfst.org/twiki/bin/view/Contrib/FstContrib)

## Building

### Prerequisites

*   A C++17 compatible compiler such as
    [gcc >= 7.5.0 or clang >= 14.0.0](https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md#compilers-tools-build-systems).

### Bazel

OpenFst can be built and tested using [Bazel](https://bazel.build) 8 or newer.

```bash
# Build the entire project
bazel build //...

# Run all tests
bazel test //...
```

Alternatively, [Bazelisk](https://github.com/bazelbuild/bazelisk) can be used
for building.

### CMake

OpenFst can also be built with [CMake](https://cmake.org) 3.22 or higher.

#### Build and Install

Dependencies like Abseil, GoogleTest, and Google Benchmark are automatically
downloaded using `FetchContent`.

```bash
# Configure the project
# Use -DOPENFST_BUILD_TESTS=OFF to skip building tests
cmake -S . -B build -DOPENFST_BUILD_TESTS=ON

# Build the project
# On Linux, `-j$(nproc)` can be used to reduce typing.
# https://man7.org/linux/man-pages/man1/nproc.1.html
cmake --build build -j$(getconf _NPROCESSORS_ONLN)

# Run tests
ctest --test-dir build --output-on-failure -j$(getconf _NPROCESSORS_ONLN)

# Install the project
# Use --prefix to specify an installation directory
cmake --install build --prefix /usr/local
```

Note that several tests are expected to fail without `-DBUILD_SHARED_LIBS=ON`.

#### Configuration Options

You can enable or disable specific features using CMake options (default is
`OFF` unless noted):

Option                          | Description                                      | Default
:------------------------------ | :----------------------------------------------- | :------
`OPENFST_ENABLE_BIN`            | Build `fst::script` and command-line executables | `ON`
`OPENFST_ENABLE_CATEGORIAL`     | Enable categorial semiring extension             | `OFF`
`OPENFST_ENABLE_COMPACT_FSTS`   | Enable CompactFst extensions                     | `OFF`
`OPENFST_ENABLE_COMPRESS`       | Enable compression extension                     | `OFF`
`OPENFST_ENABLE_CONST_FSTS`     | Enable ConstFst extensions                       | `OFF`
`OPENFST_ENABLE_FAR`            | Enable FAR extensions                            | `OFF`
`OPENFST_ENABLE_LINEAR_FSTS`    | Enable LinearTagger/ClassifierFst extensions     | `OFF`
`OPENFST_ENABLE_LOOKAHEAD_FSTS` | Enable LookAheadFst extensions                   | `OFF`
`OPENFST_ENABLE_MPDT`           | Enable MPDT extensions                           | `OFF`
`OPENFST_ENABLE_NGRAM_FSTS`     | Enable NGramFst extensions                       | `OFF`
`OPENFST_ENABLE_PDT`            | Enable PDT extensions                            | `OFF`
`OPENFST_ENABLE_PYTHON`         | Enable Python extension                          | `OFF`
`OPENFST_ENABLE_SPECIAL`        | Enable special-matcher extensions                | `OFF`

There are also meta-options to enable groups of extensions:

Option                | Description
:-------------------- | :----------
`OPENFST_ENABLE_FSTS` | Enable all FST extensions (Compact, Const, Linear, LookAhead, NGram, Special, Compress)
`OPENFST_ENABLE_GRM`  | Enable all dependencies of OpenGrm (FAR, PDT, MPDT, NGram)

Example usage:

```bash
cmake -S . -B build -DOPENFST_ENABLE_FAR=ON -DOPENFST_ENABLE_PYTHON=ON
```

### Python with Bazel

To build and test the Python wrapper (`pywrapfst`) run

```bash
bazel test -c opt openfst/extensions/python/...
```

See the corresponding unit test `pywrapfst_test.py` for a detailed API example.
Access the API by depending on `//openfst:pywrapfst` and import the wrappers
like so:

```python
from openfst import pywrapfst
```

### Python with CMake

OpenFst provides a Python wrapper (`pywrapfst`).

#### Prerequisites

To build the Python extension, you need:

*   Python 3.6 or higher and development headers (e.g., `python3-dev`).
*   [Cython](https://cython.org/) (`pip install cython`).
*   [absl-py](https://github.com/abseil/abseil-py) (`pip install absl-py`) to
    run tests.

#### Build and Test

The Python extension requires OpenFst to be built as **shared libraries**.

```bash
# Configure with shared libraries enabled
cmake -S . -B build -DOPENFST_ENABLE_PYTHON=ON -DOPENFST_BUILD_TESTS=ON -DBUILD_SHARED_LIBS=ON

# Build the project
cmake --build build -j$(getconf _NPROCESSORS_ONLN)

# Run the Python test
ctest --test-dir build -R pywrapfst_test --output-on-failure
```

## Citing OpenFst

If you would like to reference OpenFst in a publication, please use:

```bibtex
@inproceedings{allauzen2007openfst,
  author    = {Allauzen, Cyril and Riley, Michael and Schalkwyk, Johan and Skut, Wojciech and Mohri, Mehryar},
  title     = {{OpenFst}: A General and Efficient Weighted Finite-State Transducer Library},
  booktitle = {Implementation and Application of Automata: Proceedings of the 12th International Conference (CIAA 2007)},
  series    = {Lecture Notes in Computer Science},
  volume    = {4783},
  pages     = {11--23},
  year      = {2007},
  editor    = {Holub, Jan and \v{Z}}{\v{d}}{\'a}rek, Jan},
  publisher = {Springer},
  address   = {Berlin, Heidelberg},
  location  = {Prague, Czech Republic},
  isbn      = {978-3-540-76336-9},
  doi       = {10.1007/978-3-540-76336-9_3},
  note      = {\texttt{http://www.openfst.org}},
}
```

## Pull Requests

At this time, we do not accept pull requests.

Commits may be force-pushed at any time until we start accepting them (soft
target before the end of March 2026).

## License

OpenFst is licensed under the terms of the Apache license. See
[LICENSE](LICENSE) for more information.

## Disclaimer

This is not an officially supported Google product. This project is not eligible
for the
[Google Open Source Software Vulnerability Rewards Program](https://bughunters.google.com/open-source-security).
