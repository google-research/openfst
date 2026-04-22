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

#ifndef OPENFST_COMPAT_STATUS_MACROS_H_
#define OPENFST_COMPAT_STATUS_MACROS_H_

#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/check.h"
#include "openfst/compat/status_builder.h"

namespace fst {

#define RETURN_IF_ERROR(expr) \
  RETURN_IF_ERROR_IMPL(OPENFST_STATUS_IMPL_CONCAT(status, __COUNTER__), expr)

#define RETURN_IF_ERROR_IMPL(_status, expr)  \
  if (auto _status = (expr); _status.ok()) { \
  } else /* NOLINT */                        \
    return ::fst::StatusBuilder(_status)

#define ASSIGN_OR_RETURN(...)                                          \
  OPENFST_STATUS_IMPL_GET_VARIADIC(                                    \
      (__VA_ARGS__, ASSIGN_OR_RETURN_IMPL_3, ASSIGN_OR_RETURN_IMPL_2)) \
  (__VA_ARGS__)

#define ASSIGN_OR_RETURN_IMPL_2(lhs, rexpr) \
  ASSIGN_OR_RETURN_IMPL_3(lhs, rexpr, _)
#define ASSIGN_OR_RETURN_IMPL_3(lhs, rexpr, error_expression)              \
  ASSIGN_OR_RETURN_IMPL(OPENFST_STATUS_IMPL_CONCAT(statusor, __COUNTER__), \
                        lhs, rexpr, error_expression)
#define ASSIGN_OR_RETURN_IMPL(_statusor, lhs, rexpr, error_expression)  \
  auto _statusor = (rexpr);                                             \
  if (ABSL_PREDICT_FALSE(!_statusor.ok())) {                            \
    ::fst::StatusBuilder _(std::move(_statusor).status());              \
    (void)_; /* error expression is allowed to not use this variable */ \
    return (error_expression);                                          \
  }                                                                     \
  OPENFST_STATUS_IMPL_UNPARENTHESIZE_IF_PARENTHESIZED(lhs) =            \
      (*std::move(_statusor))

}  // namespace fst

#endif  // OPENFST_COMPAT_STATUS_MACROS_H_
