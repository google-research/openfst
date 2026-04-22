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

#ifndef OPENFST_COMPAT_STATUS_MATCHERS_H_
#define OPENFST_COMPAT_STATUS_MATCHERS_H_

#include <utility>

#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "openfst/compat/status_builder.h"

namespace fst {

#define ABSL_ASSERT_OK_AND_ASSIGN(lhs, rexpr)                                  \
  ASSERT_OK_AND_ASSIGN_IMPL(OPENFST_STATUS_IMPL_CONCAT(statusor, __COUNTER__), \
                            lhs, rexpr)

#define ASSERT_OK_AND_ASSIGN_IMPL(_statusor, lhs, rexpr)     \
  auto _statusor = (rexpr);                                  \
  ABSL_ASSERT_OK(_statusor);                                 \
  OPENFST_STATUS_IMPL_UNPARENTHESIZE_IF_PARENTHESIZED(lhs) = \
      (*std::move(_statusor));

}  // namespace fst

#endif  // OPENFST_COMPAT_STATUS_MATCHERS_H_
