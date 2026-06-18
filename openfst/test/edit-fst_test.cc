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
// Tests the edit operations possible with an EditFst. Further testing of an
// EditFst is performed by the fst_test.cc file that is part of this library.

#include "openfst/lib/edit-fst.h"

#include <array>
#include <map>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

// Names of files containing FAs. The unit test checks that:
//
//   - Equal(edit(fa1), fa2)
// where edit(fa1) are edits performed using EditFst on fa1 to make it
// equal to fa2.
//
std::array<absl::string_view, 2> fst_names = {"e1", "e2"};

class EditTest : public testing::Test {
 public:
  // Map from string IDs to FSTs.
  using NameToFstMap =
      std::map<absl::string_view, std::unique_ptr<MutableFst<Arc>>>;

 protected:
  // Sets up the equal_fsts_ map with a pair of fst's read from file.
  void SetUp() override {
    const std::string kTestPath = JoinPath(
        std::string("."), "openfst/test/testdata/edit");

    for (absl::string_view s : fst_names) {
      const std::string source =
          JoinPath(kTestPath, absl::StrCat(s, ".fst"));
      equal_fsts_[s] = absl::WrapUnique(VectorFst<Arc>::Read(source));
    }
  }

  // Deletes the fst's contained in equal_fsts_, and clears the map.
  void TearDown() override { equal_fsts_.clear(); }

  void VerifyAndTestEquality(const Fst<Arc>& fst1, const Fst<Arc>& fst2) {
    ASSERT_TRUE(Verify(fst1));
    ASSERT_TRUE(Verify(fst2));
    ASSERT_TRUE(Equal(fst1, fst2));
    ASSERT_TRUE(Equal(fst2, fst1));
  }

  // A collection of equal FSTs.
  NameToFstMap equal_fsts_;
};

// Verifies and checks FA equivalence on an fst read from file that has been
// non-destructively edited to be equal to a second fst read from file.
TEST_F(EditTest, Equiv) {
  // test default EditFst constructor
  EditFst<Arc> default_edit_fst;
  ASSERT_TRUE(Verify(default_edit_fst));

  ASSERT_EQ(2, equal_fsts_.size());

  NameToFstMap::const_iterator it1 = equal_fsts_.begin();
  NameToFstMap::const_iterator it2 = it1;
  ++it2;
  ExpandedFst<Arc>* e1 = it1->second.get();
  MutableFst<Arc>* e2 = it2->second.get();

  // transform e1 to become e2 via one or more non-destructive edits
  auto e1_edited = std::make_unique<EditFst<Arc>>(*e1);

  // change the sole arc from state 1 to 2 to have a weight of 5
  // instead of 2.5
  MutableArcIterator<EditFst<Arc>> ma_it(e1_edited.get(), 1);
  Arc old_arc = ma_it.Value();
  Arc new_arc(old_arc.ilabel, old_arc.olabel, Weight(5), old_arc.nextstate);
  ma_it.SetValue(new_arc);

  ASSERT_TRUE(Verify(*e1_edited));

  // change the weight of the e1_edited's final state, state 2, from
  // 3.5 to 7
  e1_edited->SetFinal(2, Weight(7));

  // the two previous mutations to e1 should make it identical to e2
  VerifyAndTestEquality(*e1_edited, *e2);

  // next, we do identical mutations to both e1_edited and e2, and
  // test for equality of both fst's after each mutation

  e1_edited->SetStart(1);
  e2->SetStart(1);

  VerifyAndTestEquality(*e1_edited, *e2);

  e1_edited->DeleteArcs(0);
  e2->DeleteArcs(0);

  VerifyAndTestEquality(*e1_edited, *e2);

  // try to iterate over e1_edited's arcs for state 0; final position
  // should be 0, since we just deleted all of state 0's arcs
  ArcIterator<Fst<Arc>> arc_iterator(*e1_edited, 0);
  for (; !arc_iterator.Done(); arc_iterator.Next()) {
  }
  ASSERT_EQ(0, arc_iterator.Position());

  e1_edited->AddArc(0, Arc(1, 1, Weight(0.3), 2));
  e2->AddArc(0, Arc(1, 1, Weight(0.3), 2));

  VerifyAndTestEquality(*e1_edited, *e2);

  StateId e1_new_state_id = e1_edited->AddState();
  StateId e2_new_state_id = e2->AddState();

  VerifyAndTestEquality(*e1_edited, *e2);

  e1_edited->AddArc(e1_new_state_id, Arc(1, 1, Weight(0.3), 2));
  e2->AddArc(e2_new_state_id, Arc(1, 1, Weight(0.3), 2));

  VerifyAndTestEquality(*e1_edited, *e2);

  // Test that a copy of e1_edited is still identical to e2.
  {
    std::unique_ptr<EditFst<Arc>> e1_edited_copy(e1_edited->Copy());
    VerifyAndTestEquality(*e1_edited_copy, *e2);
  }

  // Test that a thread-safe copy of e1_edited is still identical to e2.
  {
    std::unique_ptr<EditFst<Arc>> e1_edited_thread_safe_copy(
        e1_edited->Copy(true));
    VerifyAndTestEquality(*e1_edited_thread_safe_copy, *e2);
    e1_edited_thread_safe_copy->DeleteArcs(e1_new_state_id);
  }
}

}  // namespace
}  // namespace fst
