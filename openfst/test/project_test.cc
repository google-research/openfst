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
// Unit test for Project.

#include "openfst/lib/project.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;

class ProjectTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/project");
    const std::string project1_name = JoinPath(path, "p1.fst");
    const std::string project2_name = JoinPath(path, "p2.fst");
    const std::string project3_name = JoinPath(path, "p3.fst");

    pfst1_.reset(VectorFst<Arc>::Read(project1_name));
    pfst2_.reset(VectorFst<Arc>::Read(project2_name));
    pfst3_.reset(VectorFst<Arc>::Read(project3_name));
  }

  std::unique_ptr<const VectorFst<Arc>> pfst1_;
  std::unique_ptr<const VectorFst<Arc>> pfst2_;
  std::unique_ptr<const VectorFst<Arc>> pfst3_;
};

TEST_F(ProjectTest, Project) {
  VectorFst<Arc> vfst;
  Project(*pfst1_, &vfst, ProjectType::INPUT);
  EXPECT_TRUE(Verify(vfst));
  EXPECT_TRUE(vfst.Properties(kAcceptor, true));
  EXPECT_TRUE(Equal(*pfst2_, vfst));

  Project(*pfst1_, &vfst, ProjectType::OUTPUT);
  EXPECT_TRUE(Verify(vfst));
  EXPECT_TRUE(vfst.Properties(kAcceptor, true));
  EXPECT_TRUE(Equal(*pfst3_, vfst));

  // Empty FST.
  VectorFst<Arc> nfst;
  Project(nfst, &vfst, ProjectType::INPUT);
  EXPECT_TRUE(Verify(vfst));
  EXPECT_TRUE(vfst.Properties(kAcceptor, true));
}

TEST_F(ProjectTest, ProjectFst) {
  VectorFst<Arc> nfst;

  ProjectFst<Arc> dfst1(*pfst1_, ProjectType::INPUT);
  EXPECT_TRUE(Verify(dfst1));
  EXPECT_TRUE(dfst1.Properties(kAcceptor, true));
  EXPECT_TRUE(Equal(*pfst2_, dfst1));

  ProjectFst<Arc> ndfst1(nfst, ProjectType::INPUT);
  EXPECT_TRUE(Verify(ndfst1));
  EXPECT_TRUE(ndfst1.Properties(kAcceptor, true));
  EXPECT_TRUE(Equal(nfst, ndfst1));

  ProjectFst<Arc> dfst2(*pfst1_, ProjectType::OUTPUT);
  EXPECT_TRUE(Verify(dfst2));
  EXPECT_TRUE(dfst2.Properties(kAcceptor, true));
  EXPECT_TRUE(Equal(*pfst3_, dfst2));

  ProjectFst<Arc> ndfst2(nfst, ProjectType::OUTPUT);
  EXPECT_TRUE(Verify(ndfst2));
  EXPECT_TRUE(ndfst2.Properties(kAcceptor, true));
  EXPECT_TRUE(Equal(nfst, ndfst2));

  for (const bool safe : {false, true}) {
    ProjectFst<Arc> cfst(dfst1, safe);
    EXPECT_TRUE(Verify(cfst));
    EXPECT_TRUE(cfst.Properties(kAcceptor, true));
    EXPECT_TRUE(Equal(*pfst2_, cfst));
  }
}

}  // namespace
}  // namespace fst
