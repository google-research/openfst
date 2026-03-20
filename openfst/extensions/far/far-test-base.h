// Copyright 2026 The OpenFst Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Shared test fixture for FAR tests.

#ifndef OPENFST_EXTENSIONS_FAR_FAR_TEST_BASE_H_
#define OPENFST_EXTENSIONS_FAR_FAR_TEST_BASE_H_

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"

namespace fst {

class FarTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path_name =
        JoinPath(std::string("."),
                       "openfst/extensions/far/testdata");
    const std::string fst1_name = JoinPath(path_name, "test1-01.fst");
    const std::string fst2_name = JoinPath(path_name, "test1-02.fst");
    const std::string fst3_name = JoinPath(path_name, "test1-03.fst");

    fst1_.reset(Fst<LogArc>::Read(fst1_name));
    fst2_.reset(Fst<LogArc>::Read(fst2_name));
    fst3_.reset(Fst<LogArc>::Read(fst3_name));
  }

  std::unique_ptr<Fst<LogArc>> fst1_;
  std::unique_ptr<Fst<LogArc>> fst2_;
  std::unique_ptr<Fst<LogArc>> fst3_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FAR_TEST_BASE_H_
