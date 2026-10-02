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
// Unit test for Isomorphic.

#include "openfst/lib/isomorphic.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

constexpr absl::string_view kBaseDir =
    "openfst/test/testdata/isomorphic";

using Arc = StdArc;

TEST(IsomorphicTest, Isomorphic) {
  const std::string path = JoinPath(std::string("."), kBaseDir);
  const std::string iso1_name = JoinPath(path, "i1.fst");
  std::unique_ptr<const VectorFst<Arc>> ifst1(VectorFst<Arc>::Read(iso1_name));
  EXPECT_TRUE(Isomorphic(*ifst1, *ifst1));

  const std::string iso2_name = JoinPath(path, "i2.fst");
  std::unique_ptr<const VectorFst<Arc>> ifst2(VectorFst<Arc>::Read(iso2_name));
  EXPECT_TRUE(Isomorphic(*ifst1, *ifst2));

  const std::string iso3_name = JoinPath(path, "i3.fst");
  std::unique_ptr<const VectorFst<Arc>> ifst3(VectorFst<Arc>::Read(iso3_name));
  EXPECT_FALSE(Isomorphic(*ifst1, *ifst3));

  const std::string iso4_name = JoinPath(path, "i4.fst");
  std::unique_ptr<const VectorFst<Arc>> ifst4(VectorFst<Arc>::Read(iso4_name));
  EXPECT_FALSE(Isomorphic(*ifst1, *ifst4));
}

TEST(IsomorphicTest, NondetIsomorphic) {
  // Checks that nondet FSTs equal modulo arc ordering are isomorphic.
  const std::string path = JoinPath(std::string("."), kBaseDir);
  const std::string iso5_name = JoinPath(path, "i5.fst");
  std::unique_ptr<const VectorFst<Arc>> ifst5(VectorFst<Arc>::Read(iso5_name));
  const std::string iso6_name = JoinPath(path, "i6.fst");
  std::unique_ptr<const VectorFst<Arc>> ifst6(VectorFst<Arc>::Read(iso6_name));
  EXPECT_TRUE(Isomorphic(*ifst5, *ifst6));
}

TEST(IsomorphicTest, ManyToOneStateMappingNotIsomorphic) {
  // fst1: 0 -1-> 1(F), 0 -2-> 2(F) (3 states)
  VectorFst<Arc> fst1;
  fst1.AddStates(3);
  fst1.SetStart(0);
  fst1.AddArc(0, Arc(1, 1, Arc::Weight::One(), 1));
  fst1.AddArc(0, Arc(2, 2, Arc::Weight::One(), 2));
  fst1.SetFinal(1, Arc::Weight::One());
  fst1.SetFinal(2, Arc::Weight::One());

  // fst2: 0 -1-> 1(F), 0 -2-> 1(F) (2 states)
  VectorFst<Arc> fst2;
  fst2.AddStates(2);
  fst2.SetStart(0);
  fst2.AddArc(0, Arc(1, 1, Arc::Weight::One(), 1));
  fst2.AddArc(0, Arc(2, 2, Arc::Weight::One(), 1));
  fst2.SetFinal(1, Arc::Weight::One());

  EXPECT_FALSE(Isomorphic(fst1, fst2));
  EXPECT_FALSE(Isomorphic(fst2, fst1));
}

}  // namespace
}  // namespace fst
