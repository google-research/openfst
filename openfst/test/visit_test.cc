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
// Unit test for visitors.

#include "openfst/lib/visit.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equivalent.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;

class VisitTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."), "openfst/test/testdata/visit");
    const std::string visit1_name = JoinPath(path, "v1.fst");
    const std::string visit2_name = JoinPath(path, "v2.fst");
    const std::string visit3_name = JoinPath(path, "v3.fst");
    const std::string visit4_name = JoinPath(path, "v4.fst");

    vfst1_.reset(VectorFst<Arc>::Read(visit1_name));
    vfst2_.reset(VectorFst<Arc>::Read(visit2_name));
    vfst3_.reset(VectorFst<Arc>::Read(visit3_name));
    vfst4_.reset(VectorFst<Arc>::Read(visit4_name));
  }

  std::unique_ptr<const VectorFst<Arc>> vfst1_;
  std::unique_ptr<const VectorFst<Arc>> vfst2_;
  std::unique_ptr<const VectorFst<Arc>> vfst3_;
  std::unique_ptr<const VectorFst<Arc>> vfst4_;
};

TEST_F(VisitTest, DfsVisit) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;
  CopyVisitor<Arc> ncopy_visitor(&nfst2);
  LifoQueue<StateId> nlifo_queue;
  Visit(nfst1, &ncopy_visitor, &nlifo_queue);
  ASSERT_TRUE(Verify(nfst2));
  ASSERT_TRUE(Equivalent(nfst1, nfst2));

  VectorFst<Arc> cfst1;
  CopyVisitor<Arc> copy_visitor1(&cfst1);
  LifoQueue<StateId> lifo_queue1;
  Visit(*vfst1_, &copy_visitor1, &lifo_queue1);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Equivalent(*vfst1_, cfst1));

  PartialCopyVisitor<Arc> copy_visitor2(&cfst1, 3);
  Visit(*vfst1_, &copy_visitor2, &lifo_queue1);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Equivalent(*vfst2_, cfst1));
}

TEST_F(VisitTest, BfsVisit) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;
  CopyVisitor<Arc> ncopy_visitor(&nfst2);
  FifoQueue<StateId> nfifo_queue;
  Visit(nfst1, &ncopy_visitor, &nfifo_queue);
  ASSERT_TRUE(Verify(nfst2));
  ASSERT_TRUE(Equivalent(nfst1, nfst2));

  VectorFst<Arc> cfst1;
  CopyVisitor<Arc> copy_visitor1(&cfst1);
  FifoQueue<StateId> fifo_queue1;
  Visit(*vfst1_, &copy_visitor1, &fifo_queue1);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Equivalent(*vfst1_, cfst1));

  PartialCopyVisitor<Arc> copy_visitor2(&cfst1, 3);
  Visit(*vfst1_, &copy_visitor2, &fifo_queue1);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Equivalent(*vfst3_, cfst1));

  VectorFst<Arc> cfst2;
  PartialCopyVisitor<Arc> copy_visitor3(&cfst2, 3, false, false);
  Visit(*vfst1_, &copy_visitor3, &fifo_queue1);
  ASSERT_TRUE(Verify(cfst2));
  ASSERT_TRUE(Equivalent(*vfst4_, cfst2));
}

}  // namespace
}  // namespace fst
