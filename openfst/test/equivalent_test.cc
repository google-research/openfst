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
// Unit test for Equivalent.

#include "openfst/lib/equivalent.h"

#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

// Names of files containing FAs. Each collection implements a different regular
// language. The unittest checks that:
//
//   - Equivalent(fa1, fa2) for every pair (fa1, fa2) in each
//   collection;
//
//   - ! Equivalent(fa1, fa2) for every pair (fa1, fa2) such that fa1
//   and fa2 belong to different collections.
//
// Portability note:
// When using `constexpr` and initializer lists, MSVC generates empty string
// addresses. Hence we use `const` instead of `constexpr`. For a similar issue
// see:
// https://stackoverflow.com/questions/74209112/constexpr-initializer-list-producing-no-output-with-msvc
ABSL_CONST_INIT const std::array kFstNames{
    std::initializer_list<absl::string_view>{"e1", "e2", "e3"},
    std::initializer_list<absl::string_view>{"e4"},
    std::initializer_list<absl::string_view>{"e5", "e6", "e7"},
    std::initializer_list<absl::string_view>{"e8"},
    std::initializer_list<absl::string_view>{"e9", "e10"},
    std::initializer_list<absl::string_view>{"e11", "e12"}};

class EquivTest : public testing::Test {
 protected:
  // Map from string IDs to FSTs.
  using Name2FstMap =
      absl::flat_hash_map<absl::string_view,
                          std::unique_ptr<const VectorFst<Arc>>>;

  void SetUp() override {
    const std::string kTestPath =
        JoinPath(std::string("."),
                       "openfst/test/testdata/equivalent");
    for (const auto& init_list : kFstNames) {
      Name2FstMap fsts;
      for (absl::string_view s : init_list) {
        const std::string source =
            JoinPath(kTestPath, absl::StrCat(s, ".fst"));
        fsts[s] = absl::WrapUnique(VectorFst<Arc>::Read(source));
      }
      equiv_fsts_.push_back(std::move(fsts));
    }
    // Extra set of FA's for the empty language: constructed explicitly
    // to implement some weird/pathological cases.
    Name2FstMap extra_fsts;

    // FA without states.
    auto empty_fa = std::make_unique<VectorFst<Arc>>();
    extra_fsts["empty1"] = std::move(empty_fa);

    // FA containing a start state and a transition, but without final
    // states.
    empty_fa = std::make_unique<VectorFst<Arc>>();
    empty_fa->SetStart(empty_fa->AddState());
    empty_fa->AddState();
    empty_fa->AddArc(0, Arc(1, 1, Weight::One(), 1));
    extra_fsts["empty2"] = std::move(empty_fa);

    // FA containing a start and a final state connected by a zero-weight
    // transition.
    empty_fa = std::make_unique<VectorFst<Arc>>();
    empty_fa->SetStart(empty_fa->AddState());
    empty_fa->SetFinal(empty_fa->AddState(), Weight::One());
    empty_fa->AddArc(0, Arc(1, 1, Weight::Zero(), 1));
    extra_fsts["empty3"] = std::move(empty_fa);

    equiv_fsts_.push_back(std::move(extra_fsts));
  }

  void TearDown() override { equiv_fsts_.clear(); }

  // Collections of equivalent FSTs. Each collection implements a
  // different regular language.
  std::vector<Name2FstMap> equiv_fsts_;
};

// Checks FA equivalence within collections.
TEST_F(EquivTest, Equiv) {
  for (auto i = 0; i < equiv_fsts_.size(); ++i) {
    for (Name2FstMap::const_iterator it = equiv_fsts_[i].begin();
         it != equiv_fsts_[i].end(); ++it) {
      for (Name2FstMap::const_iterator it2 = it; it2 != equiv_fsts_[i].end();
           ++it2) {
        EXPECT_TRUE(Equivalent(*it->second, *it2->second));
        EXPECT_TRUE(Equivalent(*it2->second, *it->second));
      }
    }
  }
}

// Makes sure acceptors picked from different collections are not
// equivalent.
TEST_F(EquivTest, Nequiv) {
  // Disable symbol table compatibility check
  absl::SetFlag(&FLAGS_fst_compat_symbols, false);

  for (auto i = 0; i < equiv_fsts_.size(); ++i) {
    for (auto j = i + 1; j < equiv_fsts_.size(); ++j) {
      for (Name2FstMap::const_iterator it = equiv_fsts_[i].begin();
           it != equiv_fsts_[i].end(); ++it) {
        for (Name2FstMap::const_iterator it2 = equiv_fsts_[j].begin();
             it2 != equiv_fsts_[j].end(); ++it2) {
          EXPECT_FALSE(Equivalent(*it->second, *it2->second));
          EXPECT_FALSE(Equivalent(*it2->second, *it->second));
        }
      }
    }
  }
}

}  // namespace
}  // namespace fst
