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

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/extensions/far/far-test-base.h"
#include "openfst/extensions/far/stlist-far-reader.h"
#include "openfst/extensions/far/stlist-far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/util.h"

using ::testing::IsNull;

namespace fst {
namespace {

TEST_F(FarTest, STListNoFar) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  std::unique_ptr<STListFarReader<LogArc>> reader(STListFarReader<LogArc>::Open(
      JoinPath(::testing::TempDir(), "not_found.far")));
  EXPECT_THAT(reader, IsNull());
}

TEST_F(FarTest, STListFar) {
  std::unique_ptr<STListFarWriter<LogArc>> writer(
      STListFarWriter<LogArc>::Create(
          JoinPath(::testing::TempDir(), "test1.far")));
  writer->Add("1", *fst1_);
  writer->Add("2", *fst2_);
  writer->Add("3", *fst3_);
  writer.reset();

  std::unique_ptr<STListFarReader<LogArc>> reader(STListFarReader<LogArc>::Open(
      JoinPath(::testing::TempDir(), "test1.far")));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "1");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst1_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "2");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst2_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "3");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst3_));
  reader->Next();
  ASSERT_TRUE(reader->Done());

  writer.reset(STListFarWriter<LogArc>::Create(
      JoinPath(::testing::TempDir(), "test2.far")));
  writer->Add("10", *fst3_);
  writer->Add("20", *fst1_);
  writer->Add("30", *fst2_);
  writer.reset();

  std::vector<std::string> sources;
  sources.push_back(JoinPath(::testing::TempDir(), "test1.far"));
  sources.push_back(JoinPath(::testing::TempDir(), "test2.far"));
  reader.reset(STListFarReader<LogArc>::Open(sources));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "1");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst1_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "10");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst3_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "2");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst2_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "20");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst1_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "3");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst3_));
  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "30");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst2_));
  reader->Next();
  ASSERT_TRUE(reader->Done());

  // Try again with a non-existent FAR at the end.
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  sources.push_back(JoinPath(::testing::TempDir(), "not_found.far"));
  reader.reset(STListFarReader<LogArc>::Open(sources));
  EXPECT_THAT(reader, IsNull());

  // Try again with a non-existent FAR at the beginning.
  sources.pop_back();
  sources.insert(sources.begin(),
                 JoinPath(::testing::TempDir(), "not_found.far"));
  reader.reset(STListFarReader<LogArc>::Open(sources));
  EXPECT_THAT(reader, IsNull());
}

TEST_F(FarTest, STListEmptyFar) {
  std::unique_ptr<STListFarWriter<LogArc>> writer(
      STListFarWriter<LogArc>::Create(
          JoinPath(::testing::TempDir(), "empty.far")));
  writer.reset();

  std::unique_ptr<STListFarReader<LogArc>> reader(STListFarReader<LogArc>::Open(
      JoinPath(::testing::TempDir(), "empty.far")));
  // STList does not support Find().
  ASSERT_FALSE(reader->Error());
  ASSERT_TRUE(reader->Done());

  // STList also does not support Reset().
}

}  // namespace
}  // namespace fst
