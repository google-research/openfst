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
// Unit test for CompactFst.

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/test/compactors.h"

namespace fst {
namespace {

class CompactTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/compact");
    const std::string compact1_name = JoinPath(path, "c1.fst");
    const std::string compact2_name = JoinPath(path, "c2.fst");

    fst1_.reset(VectorFst<StdArc>::Read(compact1_name));
    fst2_.reset(VectorFst<StdArc>::Read(compact2_name));
  }

  std::unique_ptr<const VectorFst<StdArc>> fst1_;
  std::unique_ptr<const VectorFst<StdArc>> fst2_;
};

template <class Arc, class Container, class Compactor>
void Fill(Container* container, Compactor compactor, const Fst<Arc>& fst) {
  container->clear();
  typename Arc::StateId s = fst.Start();
  if ((compactor.Size() == -1) || (fst.Final(s) != Arc::Weight::Zero())) {
    container->push_back(compactor.Compact(
        s, Arc(kNoLabel, kNoLabel, fst.Final(s), kNoStateId)));
  }
  for (ArcIterator<Fst<Arc>> ait(fst, s); !ait.Done(); ait.Next())
    container->push_back(compactor.Compact(s, ait.Value()));
  for (StateIterator<Fst<Arc>> sit(fst); !sit.Done(); sit.Next()) {
    s = sit.Value();
    if (s == fst.Start()) continue;
    if ((compactor.Size() == -1) || (fst.Final(s) != Arc::Weight::Zero())) {
      container->push_back(compactor.Compact(
          s, Arc(kNoLabel, kNoLabel, fst.Final(s), kNoStateId)));
    }
    for (ArcIterator<Fst<Arc>> ait(fst, s); !ait.Done(); ait.Next())
      container->push_back(compactor.Compact(s, ait.Value()));
  }
}

// This test the compactor for unweighted FSTs.
TEST_F(CompactTest, UnweightedCompactor) {
  VectorFst<StdArc> nfst;
  CompactUnweightedFst<StdArc> cnfst(nfst);
  ASSERT_TRUE(Equal(nfst, cnfst));

  // Removes weights of fst1_ and compacts.
  ArcMapFst fst(*fst1_, RmWeightMapper<StdArc>());
  CompactArcFst<StdArc, UnweightedCompactor<StdArc>> cfst(fst);
  ASSERT_TRUE(Equal(fst, cfst));

  // Tests Read/Write.
  const std::string filename = JoinPath(::testing::TempDir(), "test.fst");
  cfst.Write(filename);
  {
    std::unique_ptr<Fst<StdArc>> pfst(Fst<StdArc>::Read(filename));
    ASSERT_TRUE(Equal(*pfst, fst));
  }

  // Tests construction from container.
  std::vector<UnweightedCompactor<StdArc>::Element> v;
  Fill(&v, UnweightedCompactor<StdArc>(), fst);
  using Compactor = CompactUnweightedFst<StdArc>::Compactor;
  CompactUnweightedFst<StdArc> cfst2(
      std::make_shared<Compactor>(v.begin(), v.end()));
  ASSERT_TRUE(Equal(fst, cfst2));

  // Tests copy constructor.
  for (const bool safe : {false, true}) {
    CompactUnweightedFst<StdArc> cpfst(cfst, safe);
    ASSERT_TRUE(Verify(cpfst));
    ASSERT_TRUE(Equal(fst, cpfst));
  }
}

// This test the compactor for unweighted acceptors.
TEST_F(CompactTest, UnweightedAcceptorCompactor) {
  VectorFst<StdArc> nfst;
  CompactUnweightedAcceptorFst<StdArc> cnfst(nfst);
  ASSERT_TRUE(Equal(nfst, cnfst));

  // Removes weights of fst1_, project and compact.
  ArcMapFst fst(ProjectFst<StdArc>(*fst1_, ProjectType::INPUT),
                RmWeightMapper<StdArc>());
  CompactUnweightedAcceptorFst<StdArc> cfst(fst);
  ASSERT_TRUE(Equal(fst, cfst));

  // Tests Read/Write.
  const std::string filename = JoinPath(::testing::TempDir(), "test.fst");
  cfst.Write(filename);
  {
    std::unique_ptr<Fst<StdArc>> pfst(Fst<StdArc>::Read(filename));
    ASSERT_TRUE(Equal(*pfst, fst));
  }

  // Tests construction from container.
  std::vector<UnweightedAcceptorCompactor<StdArc>::Element> v;
  Fill(&v, UnweightedAcceptorCompactor<StdArc>(), fst);
  using Compactor = CompactUnweightedAcceptorFst<StdArc>::Compactor;
  CompactUnweightedAcceptorFst<StdArc> cfst2(
      std::make_shared<Compactor>(v.begin(), v.end()));
  ASSERT_TRUE(Equal(fst, cfst2));
}

// This tests the compactor for acceptors.
TEST_F(CompactTest, AcceptorCompactor) {
  VectorFst<StdArc> nfst;
  CompactAcceptorFst<StdArc> cnfst(nfst);
  ASSERT_TRUE(Equal(nfst, cnfst));

  // Projects fst1_ and compact.
  ProjectFst<StdArc> fst(*fst1_, ProjectType::INPUT);
  CompactAcceptorFst<StdArc> cfst(fst);
  ASSERT_TRUE(Equal(fst, cfst));

  // Tests Read/Write.
  const std::string filename = JoinPath(::testing::TempDir(), "test.fst");
  cfst.Write(filename);
  {
    std::unique_ptr<Fst<StdArc>> pfst(Fst<StdArc>::Read(filename));
    ASSERT_TRUE(Equal(*pfst, fst));
  }

  // Tests construction from container.
  std::vector<AcceptorCompactor<StdArc>::Element> v;
  Fill(&v, AcceptorCompactor<StdArc>(), fst);
  using Compactor = CompactAcceptorFst<StdArc>::Compactor;
  CompactAcceptorFst<StdArc> cfst2(
      std::make_shared<Compactor>(v.begin(), v.end()));
  ASSERT_TRUE(Equal(fst, cfst2));
}

// This test the compactor for weighted string FSTs:
TEST_F(CompactTest, WeightedStringCompactor) {
  // Projects fst2_ and compacts.
  ProjectFst<StdArc> fst(*fst2_, ProjectType::INPUT);
  CompactWeightedStringFst<StdArc> cfst(fst);
  ASSERT_TRUE(Equal(fst, cfst));

  // Tests Read/Write.
  const std::string filename = JoinPath(::testing::TempDir(), "test.fst");
  cfst.Write(filename);
  {
    std::unique_ptr<Fst<StdArc>> pfst(Fst<StdArc>::Read(filename));
    ASSERT_TRUE(Equal(*pfst, fst));
  }

  // Tests construction from container.
  std::vector<WeightedStringCompactor<StdArc>::Element> v;
  Fill(&v, WeightedStringCompactor<StdArc>(), fst);
  using Compactor = CompactWeightedStringFst<StdArc>::Compactor;
  CompactWeightedStringFst<StdArc> cfst2(
      std::make_shared<Compactor>(v.begin(), v.end()));
  ASSERT_TRUE(Equal(fst, cfst2));
}

// This test the compactor for unweighted acceptors:
TEST_F(CompactTest, StringCompactor) {
  // Remove weights of fst2_, project and compact,
  ArcMapFst fst(ProjectFst<StdArc>(*fst2_, ProjectType::INPUT),
                RmWeightMapper<StdArc>());
  CompactStringFst<StdArc> cfst(fst);
  ASSERT_TRUE(Equal(fst, cfst));

  // Test Read/Write.
  const std::string filename = JoinPath(::testing::TempDir(), "test.fst");
  cfst.Write(filename);
  {
    std::unique_ptr<Fst<StdArc>> pfst(Fst<StdArc>::Read(filename));
    ASSERT_TRUE(Equal(*pfst, fst));
  }

  // Test construction from container.
  std::vector<StringCompactor<StdArc>::Element> v;
  Fill(&v, StringCompactor<StdArc>(), fst);
  using Compactor = CompactStringFst<StdArc>::Compactor;
  CompactStringFst<StdArc> cfst2(
      std::make_shared<Compactor>(v.begin(), v.end()));
  ASSERT_TRUE(Equal(fst, cfst2));

  // Test construction via convenience function.
  StdCompactStringFst cfst3 = MakeStdCompactStringFst(v.begin(), v.end());
  ASSERT_TRUE(Equal(fst, cfst3));

  // Test construction from string.
  std::string s;
  for (char i = 0; i < 20; ++i) s.push_back(i);
  CompactStringFst<StdArc> cfst4(
      std::make_shared<Compactor>(s.begin(), s.end()));
  ASSERT_TRUE(Equal(fst, cfst4));

  // Test construction via convenience function.
  StdCompactStringFst cfst5 = MakeStdCompactStringFst(s.begin(), s.end());
  ASSERT_TRUE(Equal(fst, cfst5));
}

TEST_F(CompactTest, WriteFstAcceptorFromProjectFst) {
  const ProjectFst<StdArc> fst(*fst1_, ProjectType::INPUT);
  const CompactAcceptorFst<StdArc> rcfst(fst);

  std::ostringstream ref;
  rcfst.Write(ref, FstWriteOptions());
  const std::string& refstr = ref.str();

  std::ostringstream oss;
  WriteCompactArcFst<CompactAcceptorFst<StdArc>>(
      fst, AcceptorCompactor<StdArc>(), oss, FstWriteOptions());
  const std::string& otfstr = oss.str();
  for (auto i = 0; i < refstr.size(); ++i) {
    if (refstr[i] != otfstr[i]) {
      LOG(INFO) << "Different at " << i << " " << static_cast<int>(refstr[i])
                << " " << static_cast<int>(otfstr[i]);
    }
  }
  EXPECT_EQ(ref.str(), oss.str());

  std::istringstream iss(oss.str());
  {
    std::unique_ptr<Fst<StdArc>> cfst(Fst<StdArc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*cfst));
    EXPECT_TRUE(Equal(fst, *cfst));
    EXPECT_EQ(cfst->Type(), "compact_acceptor");
  }
}

TEST_F(CompactTest, WriteFstAcceptorFromVectorFst) {
  const ProjectFst<StdArc> fst(*fst1_, ProjectType::INPUT);
  const CompactAcceptorFst<StdArc> rcfst(fst);

  std::ostringstream ref;
  rcfst.Write(ref, FstWriteOptions());
  const std::string& refstr = ref.str();

  std::ostringstream oss;
  WriteCompactArcFst<CompactAcceptorFst<StdArc>>(
      StdVectorFst(fst), AcceptorCompactor<StdArc>(), oss, FstWriteOptions());
  const std::string& otfstr = oss.str();
  for (auto i = 0; i < refstr.size(); ++i) {
    if (refstr[i] != otfstr[i]) {
      LOG(INFO) << "Different at " << i << " " << static_cast<int>(refstr[i])
                << " " << static_cast<int>(otfstr[i]);
    }
  }
  EXPECT_EQ(ref.str(), oss.str());
}

TEST_F(CompactTest, WriteFstAcceptorFromCompactAcceptorFst) {
  const ProjectFst<StdArc> fst(*fst1_, ProjectType::INPUT);
  const CompactAcceptorFst<StdArc> rcfst(fst);

  std::ostringstream ref;
  rcfst.Write(ref, FstWriteOptions());
  const std::string& refstr = ref.str();

  std::ostringstream oss;
  WriteCompactArcFst<CompactAcceptorFst<StdArc>>(
      CompactAcceptorFst<StdArc>(fst), AcceptorCompactor<StdArc>(), oss,
      FstWriteOptions());
  const std::string& otfstr = oss.str();
  for (auto i = 0; i < refstr.size(); ++i) {
    if (refstr[i] != otfstr[i]) {
      LOG(INFO) << "Different at " << i << " " << static_cast<int>(refstr[i])
                << " " << static_cast<int>(otfstr[i]);
    }
  }
  EXPECT_EQ(ref.str(), oss.str());
}

TEST_F(CompactTest, WriteFstAcceptorFromCompactFst) {
  const ProjectFst<StdArc> fst(*fst1_, ProjectType::INPUT);
  const CompactAcceptorFst<StdArc> rcfst(fst);

  std::ostringstream ref;
  rcfst.Write(ref, FstWriteOptions());
  const std::string& refstr = ref.str();

  std::ostringstream oss;
  WriteCompactArcFst<CompactAcceptorFst<StdArc>>(
      CompactFst<StdArc, TrivialCompactor<StdArc>>(fst),
      AcceptorCompactor<StdArc>(), oss, FstWriteOptions());
  const std::string& otfstr = oss.str();
  for (auto i = 0; i < refstr.size(); ++i) {
    if (refstr[i] != otfstr[i]) {
      LOG(INFO) << "Different at " << i << " " << static_cast<int>(refstr[i])
                << " " << static_cast<int>(otfstr[i]);
    }
  }
  EXPECT_EQ(ref.str(), oss.str());
}

TEST_F(CompactTest, WriteFstStringCompactorFromArcMapFst) {
  const ArcMapFst fst(ProjectFst<StdArc>(*fst2_, ProjectType::INPUT),
                      RmWeightMapper<StdArc>());

  std::ostringstream oss;
  WriteCompactArcFst<CompactStringFst<StdArc>>(fst, StringCompactor<StdArc>(),
                                               oss, FstWriteOptions());
  std::istringstream iss(oss.str());
  {
    std::unique_ptr<Fst<StdArc>> cfst(Fst<StdArc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*cfst));
    EXPECT_TRUE(Equal(fst, *cfst));
    EXPECT_EQ(cfst->Type(), "compact_string");
  }
}

TEST_F(CompactTest, WriteFstStringCompactorFromVectorFst) {
  const ArcMapFst fst(ProjectFst<StdArc>(*fst2_, ProjectType::INPUT),
                      RmWeightMapper<StdArc>());

  std::ostringstream oss;
  WriteCompactArcFst<CompactStringFst<StdArc>>(
      StdVectorFst(fst), StringCompactor<StdArc>(), oss, FstWriteOptions());
  std::istringstream iss(oss.str());
  {
    std::unique_ptr<Fst<StdArc>> cfst(Fst<StdArc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*cfst));
    EXPECT_TRUE(Equal(fst, *cfst));
    EXPECT_EQ(cfst->Type(), "compact_string");
  }
}

TEST_F(CompactTest, WriteFstStringCompactorFromCompactStringFst) {
  const ArcMapFst fst(ProjectFst<StdArc>(*fst2_, ProjectType::INPUT),
                      RmWeightMapper<StdArc>());

  std::ostringstream oss;
  WriteCompactArcFst<CompactStringFst<StdArc>>(CompactStringFst<StdArc>(fst),
                                               StringCompactor<StdArc>(), oss,
                                               FstWriteOptions());
  std::istringstream iss(oss.str());
  {
    std::unique_ptr<Fst<StdArc>> cfst(Fst<StdArc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*cfst));
    EXPECT_TRUE(Equal(fst, *cfst));
    EXPECT_EQ(cfst->Type(), "compact_string");
  }
}

TEST_F(CompactTest, WriteFstStringCompactorFromCompactFst) {
  const ArcMapFst fst(ProjectFst<StdArc>(*fst2_, ProjectType::INPUT),
                      RmWeightMapper<StdArc>());

  std::ostringstream oss;
  WriteCompactArcFst<CompactStringFst<StdArc>>(
      CompactFst<StdArc, TrivialCompactor<StdArc>>(fst),
      StringCompactor<StdArc>(), oss, FstWriteOptions());
  std::istringstream iss(oss.str());
  {
    std::unique_ptr<Fst<StdArc>> cfst(Fst<StdArc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*cfst));
    EXPECT_TRUE(Equal(fst, *cfst));
    EXPECT_EQ(cfst->Type(), "compact_string");
  }
}

}  // namespace
}  // namespace fst
