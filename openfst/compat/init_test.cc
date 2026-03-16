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

TEST(InitTestA, BasicTestRemoveFlags) {
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
}

}  // namespace
}  // namespace fst
