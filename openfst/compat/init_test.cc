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

#include "openfst/compat/init.h"

#include <cstring>

#include "gtest/gtest.h"
#include "absl/base/macros.h"
#include "absl/container/fixed_array.h"
#include "absl/flags/flag.h"

ABSL_FLAG(int, int_test_flag, 0, "Simple int test flag.");

namespace fst {
namespace {

TEST(InitTest, BasicTestRemoveFlags) {
  const char* const_argv[] = {
      "my_test",  "--int_test_flag=1", "--int_test_flag=2",
      "pos_arg1", "pos_arg2",          nullptr,
  };
  int argc = ABSL_ARRAYSIZE(const_argv) - 1;
  absl::FixedArray<char*, 0> argv_save(argc + 1);
  char** argv = argv_save.data();
  std::memcpy(argv, const_argv, sizeof(*argv) * (argc + 1));
  InitOpenFst("", &argc, &argv, /*remove_flags=*/true);
  EXPECT_EQ(3, argc);
  EXPECT_STREQ("my_test", argv[0]);
  EXPECT_STREQ("pos_arg1", argv[1]);
  EXPECT_STREQ("pos_arg2", argv[2]);
  EXPECT_EQ(2, absl::GetFlag(FLAGS_int_test_flag));
}

// Rearranges the flags pushing the positional arguments to the end.
TEST(ParseCommandLineFlagsTest, KeepFlags) {
  const char* const_argv[] = {
      "my_test", "pos_arg", "--int_test_flag=0", "--int_test_flag=3", nullptr,
  };
  int argc = ABSL_ARRAYSIZE(const_argv) - 1;
  absl::FixedArray<char*, 0> argv_save(argc + 1);
  char** argv = argv_save.data();
  std::memcpy(argv, const_argv, sizeof(*argv) * (argc + 1));
  const int pos = ParseCommandLineFlags(&argc, &argv, /*remove_flags=*/false);
  EXPECT_EQ(4, argc);
  EXPECT_STREQ("my_test", argv[0]);
  EXPECT_STREQ("--int_test_flag=0", argv[1]);
  EXPECT_STREQ("--int_test_flag=3", argv[2]);
  EXPECT_STREQ("pos_arg", argv[3]);
  EXPECT_EQ(3, pos);
  EXPECT_EQ(3, absl::GetFlag(FLAGS_int_test_flag));
}

// Removes the flags keeping the positional ones.
TEST(ParseCommandLineFlagsTest, RemoveFlags) {
  const char* const_argv[] = {
      "my_test", "pos_arg", "--int_test_flag=0", "--int_test_flag=3", nullptr,
  };
  int argc = ABSL_ARRAYSIZE(const_argv) - 1;
  absl::FixedArray<char*, 0> argv_save(argc + 1);
  char** argv = argv_save.data();
  std::memcpy(argv, const_argv, sizeof(*argv) * (argc + 1));
  const int pos = ParseCommandLineFlags(&argc, &argv, /*remove_flags=*/true);
  EXPECT_EQ(2, argc);
  EXPECT_STREQ("my_test", argv[0]);
  EXPECT_STREQ("pos_arg", argv[1]);
  EXPECT_EQ(1, pos);
  EXPECT_EQ(3, absl::GetFlag(FLAGS_int_test_flag));
}

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  // Do not call `InitOpenFst` since it calls `absl::SetProgramUsageMessage`,
  // which can be called at most once.
  return RUN_ALL_TESTS();
}
