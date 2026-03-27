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
#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/far-test-base.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/sstable-far-reader.h"
#include "openfst/extensions/far/sstable-far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

using ::testing::IsNull;
using ::testing::NotNull;

namespace fst {
namespace {

TEST_F(FarTest, SSTableNoFar) {
  std::unique_ptr<const SSTableFarReader<LogArc>> reader(
      SSTableFarReader<LogArc>::Open(
          JoinPath(::testing::TempDir(), "not_found.far")));
  EXPECT_THAT(reader, IsNull());
}

TEST_F(FarTest, SSTableFar) {
  std::unique_ptr<SSTableFarWriter<LogArc>> writer(
      SSTableFarWriter<LogArc>::Create(
          JoinPath(::testing::TempDir(), "test1.far")));
  writer->Add("1", *fst1_);
  writer->Add("2", *fst2_);
  writer->Add("3", *fst3_);
  writer.reset();

  std::unique_ptr<SSTableFarReader<LogArc>> reader(
      SSTableFarReader<LogArc>::Open(
          JoinPath(::testing::TempDir(), "test1.far")));
  for (auto i = 0; i < 2; ++i) {
    if (i) reader->Reset();
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
  }
  EXPECT_TRUE(reader->Find("2"));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "2");
  ASSERT_FALSE(reader->Done());

  writer.reset(SSTableFarWriter<LogArc>::Create(
      JoinPath(::testing::TempDir(), "test2.far")));
  writer->Add("10", *fst3_);
  writer->Add("20", *fst1_);
  writer->Add("30", *fst2_);
  writer.reset();

  std::vector<std::string> sources;
  sources.push_back(JoinPath(::testing::TempDir(), "test1.far"));
  sources.push_back(JoinPath(::testing::TempDir(), "test2.far"));
  reader.reset(SSTableFarReader<LogArc>::Open(sources));
  for (auto i = 0; i < 2; ++i) {
    if (i) reader->Reset();
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
  }
  EXPECT_TRUE(reader->Find("2"));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "2");
  ASSERT_TRUE(Equal(*(reader->GetFst()), *fst2_));
  ASSERT_FALSE(reader->Done());

  // Try again with a non-existent FAR at the end.
  sources.push_back(JoinPath(::testing::TempDir(), "not_found.far"));
  reader.reset(SSTableFarReader<LogArc>::Open(sources));
  EXPECT_THAT(reader, IsNull());

  // Try again with a non-existent FAR at the beginning.
  sources.pop_back();
  sources.insert(sources.begin(),
                 JoinPath(::testing::TempDir(), "not_found.far"));
  reader.reset(SSTableFarReader<LogArc>::Open(sources));
  EXPECT_THAT(reader, IsNull());
}

TEST_F(FarTest, SSTableNoFarClass) {
  namespace s = fst::script;
  auto reader = s::FarReaderClass::Open(
      JoinPath(::testing::TempDir(), "not_found.far"));
  EXPECT_THAT(reader, IsNull());
}

// The same as the SSTableFarClass test, but with FarReaderClass and
// FarWriterClass instead.
TEST_F(FarTest, SSTableFarClass) {
  namespace s = fst::script;

  auto writer = s::FarWriterClass::Create(
      JoinPath(::testing::TempDir(), "test1.far"), "log",
      FarType::SSTABLE);
  s::FstClass vfst1(*fst1_);
  s::FstClass vfst2(*fst2_);
  s::FstClass vfst3(*fst3_);
  writer->Add("1", vfst1);
  writer->Add("2", vfst2);
  writer->Add("3", vfst3);
  auto* typed_writer = writer->GetFarWriter<LogArc>();
  EXPECT_EQ(typed_writer->Type(), FarType::SSTABLE);
  writer.reset();

  auto reader = s::FarReaderClass::Open(
      JoinPath(::testing::TempDir(), "test1.far"));
  for (auto i = 0; i < 2; ++i) {
    if (i) reader->Reset();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "1");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst1));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "2");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst2));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "3");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst3));
    reader->Next();
    ASSERT_TRUE(reader->Done());
  }
  EXPECT_TRUE(reader->Find("2"));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "2");
  ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst2));
  ASSERT_FALSE(reader->Done());
  auto* typed_reader = reader->GetFarReader<LogArc>();
  EXPECT_EQ(typed_reader->Type(), FarType::SSTABLE);

  writer = s::FarWriterClass::Create(
      JoinPath(::testing::TempDir(), "test2.far"), "log",
      FarType::SSTABLE);
  writer->Add("10", vfst3);
  writer->Add("20", vfst1);
  writer->Add("30", vfst2);
  writer.reset();

  std::vector<std::string> sources;
  sources.push_back(JoinPath(::testing::TempDir(), "test1.far"));
  sources.push_back(JoinPath(::testing::TempDir(), "test2.far"));
  reader = s::FarReaderClass::Open(sources);
  for (auto i = 0; i < 2; ++i) {
    if (i) reader->Reset();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "1");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst1));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "10");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst3));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "2");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst2));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "20");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst1));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "3");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst3));
    reader->Next();
    ASSERT_FALSE(reader->Done());
    ASSERT_EQ(reader->GetKey(), "30");
    ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst2));
    reader->Next();
    ASSERT_TRUE(reader->Done());
  }
  EXPECT_TRUE(reader->Find("2"));
  ASSERT_FALSE(reader->Done());
  ASSERT_EQ(reader->GetKey(), "2");
  ASSERT_TRUE(s::Equal(*(reader->GetFstClass()), vfst2));
  ASSERT_FALSE(reader->Done());

  // Try again with a non-existent FAR at the end.
  sources.push_back(JoinPath(::testing::TempDir(), "not_found.far"));
  reader = s::FarReaderClass::Open(sources);
  EXPECT_THAT(reader, IsNull());

  // Try again with a non-existent FAR at the beginning.
  sources.pop_back();
  sources.insert(sources.begin(),
                 JoinPath(::testing::TempDir(), "not_found.far"));
  reader = s::FarReaderClass::Open(sources);
  EXPECT_THAT(reader, IsNull());
}

TEST_F(FarTest, SSTableEmptyFar) {
  std::unique_ptr<const SSTableFarWriter<LogArc>> writer(
      SSTableFarWriter<LogArc>::Create(
          JoinPath(::testing::TempDir(), "empty.far")));
  writer.reset();

  std::unique_ptr<SSTableFarReader<LogArc>> reader(
      SSTableFarReader<LogArc>::Open(
          JoinPath(::testing::TempDir(), "empty.far")));
  ASSERT_FALSE(reader->Error());
  ASSERT_FALSE(reader->Find("foo"));
  ASSERT_TRUE(reader->Done());

  // Nothing changes after Reset().
  reader->Reset();
  ASSERT_FALSE(reader->Error());
  ASSERT_FALSE(reader->Find("foo"));
  ASSERT_TRUE(reader->Done());
}

TEST_F(FarTest, SSTableEmptyFarClass) {
  namespace s = fst::script;

  auto writer = s::FarWriterClass::Create(
      JoinPath(::testing::TempDir(), "empty.far"), "log",
      FarType::SSTABLE);
  auto* typed_writer = writer->GetFarWriter<LogArc>();
  EXPECT_EQ(typed_writer->Type(), FarType::SSTABLE);
  writer.reset();

  auto reader = s::FarReaderClass::Open(
      JoinPath(::testing::TempDir(), "empty.far"));
  ASSERT_THAT(reader, NotNull());
  ASSERT_FALSE(reader->Error());
  ASSERT_FALSE(reader->Find("foo"));
  ASSERT_TRUE(reader->Done());

  // Nothing changes after Reset().
  reader->Reset();
  ASSERT_FALSE(reader->Error());
  ASSERT_FALSE(reader->Find("foo"));
  ASSERT_TRUE(reader->Done());

  // We wrote this as LogArc, but it will come back as ErrorArc.
  EXPECT_THAT(reader->GetFarReader<LogArc>(), IsNull());
  auto* typed_reader = reader->GetFarReader<ErrorArc>();
  ASSERT_THAT(typed_reader, NotNull());
  EXPECT_EQ(typed_reader->Type(), FarType::SSTABLE);
}

}  // namespace
}  // namespace fst
