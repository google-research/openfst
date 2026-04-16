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
#include "openfst/extensions/far/fst-far-reader.h"
#include "openfst/extensions/far/fst-far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"

using ::testing::IsNull;

namespace fst {
namespace {

TEST_F(FarTest, FstFar) {
  // FstFarWriter only supports writing a single FST.
  std::unique_ptr<FstFarWriter<LogArc>> writer(FstFarWriter<LogArc>::Create(
      JoinPath(::testing::TempDir(), "test1.fst")));
  writer->Add("1", *fst1_);
  writer.reset();

  std::unique_ptr<FstFarReader<LogArc>> reader(FstFarReader<LogArc>::Open(
      JoinPath(::testing::TempDir(), "test1.fst")));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(),
            JoinPath(::testing::TempDir(), "test1.fst"));
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst1_));
  reader->Next();
  ASSERT_TRUE(reader->Done());

  // Merge multiple files.
  std::vector<std::string> sources;
  sources.push_back(JoinPath(::testing::TempDir(), "test1.fst"));

  writer.reset(FstFarWriter<LogArc>::Create(
      JoinPath(::testing::TempDir(), "test2.fst")));
  writer->Add("2", *fst2_);
  writer.reset();
  sources.push_back(JoinPath(::testing::TempDir(), "test2.fst"));

  reader.reset(FstFarReader<LogArc>::Open(sources));
  ASSERT_FALSE(reader->Done());
  // Keys are filenames for FstFarReader.
  EXPECT_TRUE(reader->Find(sources[0]));
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst1_));

  reader->Next();
  ASSERT_FALSE(reader->Done());
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst2_));

  reader->Next();
  ASSERT_TRUE(reader->Done());
}

}  // namespace
}  // namespace fst
