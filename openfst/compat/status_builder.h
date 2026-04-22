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

#ifndef OPENFST_COMPAT_STATUS_BUILDER_H_
#define OPENFST_COMPAT_STATUS_BUILDER_H_

#include <sstream>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace fst {

// Dummy implementation of StatusBuilder. Placeholder for real implementation
// allowing status to be annotated using operator<<.
class StatusBuilder {
 public:
  explicit StatusBuilder(const ::absl::Status& status) : status_(status) {}
  explicit StatusBuilder(::absl::Status&& status)
      : status_(std::move(status)) {}

  // Implicit conversion back to absl::Status.
  operator ::absl::Status() const {
    const auto& stream_msg = stream_.str();
    if (stream_msg.empty()) {
      return status_;
    }
    ::absl::Status result;
    if (status_.message().empty()) {
      result = absl::Status(status_.code(), stream_msg);
    } else {
      switch (message_join_style_) {
        case MessageJoinStyle::kAnnotate:
          result = absl::Status(status_.code(), absl::StrCat(status_.message(),
                                                             "; ", stream_msg));
          break;
        case MessageJoinStyle::kAppend:
          result = absl::Status(status_.code(),
                                absl::StrCat(status_.message(), stream_msg));
          break;
        case MessageJoinStyle::kPrepend:
          result = absl::Status(status_.code(),
                                absl::StrCat(stream_msg, status_.message()));
          break;
      }
    }
    status_.ForEachPayload([&](auto type_url, auto payload) {
      result.SetPayload(std::move(type_url), std::move(payload));
    });
    return result;
  }

  template <typename Adaptor>
  auto With(Adaptor&& adaptor) {
    return std::forward<Adaptor>(adaptor)(std::move(*this));
  }

  template <typename T>
  StatusBuilder& operator<<(const T& extra_msg) & {
    stream_ << extra_msg;
    return *this;
  }

  template <typename T>
  StatusBuilder&& operator<<(const T& extra_msg) && {
    stream_ << extra_msg;
    return std::move(*this);
  }

  StatusBuilder& SetPrepend() & {
    message_join_style_ = MessageJoinStyle::kPrepend;
    return *this;
  }

  StatusBuilder&& SetPrepend() && {
    message_join_style_ = MessageJoinStyle::kPrepend;
    return std::move(*this);
  }

  StatusBuilder& SetAppend() & {
    message_join_style_ = MessageJoinStyle::kAppend;
    return *this;
  }

  StatusBuilder&& SetAppend() && {
    message_join_style_ = MessageJoinStyle::kAppend;
    return std::move(*this);
  }

 private:
  enum class MessageJoinStyle {
    kAnnotate,
    kAppend,
    kPrepend,
  };

  absl::Status status_;
  std::ostringstream stream_;
  MessageJoinStyle message_join_style_ = MessageJoinStyle::kAnnotate;
};

// Macros shared by status matchers and status macros.

#define OPENFST_STATUS_IMPL_CONCAT_INNER(a, b) a##b
#define OPENFST_STATUS_IMPL_CONCAT(a, b) OPENFST_STATUS_IMPL_CONCAT_INNER(a, b)

#define OPENFST_STATUS_IMPL_GET_VARIADIC_INNER(_1, _2, _3, NAME, ...) NAME
#define OPENFST_STATUS_IMPL_GET_VARIADIC(args) \
  OPENFST_STATUS_IMPL_GET_VARIADIC_INNER args

// Internal helpers for macro expansion.
#define OPENFST_STATUS_IMPL_EAT(...)
#define OPENFST_STATUS_IMPL_REM(...) __VA_ARGS__
#define OPENFST_STATUS_IMPL_EMPTY()

// Internal helpers for emptyness arguments check.
#define OPENFST_STATUS_IMPL_IS_EMPTY_INNER(...) \
  OPENFST_STATUS_IMPL_IS_EMPTY_INNER_HELPER((__VA_ARGS__, 0, 1))
// MSVC expands variadic macros incorrectly, so we need this extra indirection
// to work around that (b/110959038).
#define OPENFST_STATUS_IMPL_IS_EMPTY_INNER_HELPER(args) \
  OPENFST_STATUS_IMPL_IS_EMPTY_INNER_I args
#define OPENFST_STATUS_IMPL_IS_EMPTY_INNER_I(e0, e1, is_empty, ...) is_empty

#define OPENFST_STATUS_IMPL_IS_EMPTY(...) \
  OPENFST_STATUS_IMPL_IS_EMPTY_I(__VA_ARGS__)
#define OPENFST_STATUS_IMPL_IS_EMPTY_I(...) \
  OPENFST_STATUS_IMPL_IS_EMPTY_INNER(_, ##__VA_ARGS__)

// Internal helpers for if statement.
#define OPENFST_STATUS_IMPL_IF_1(_Then, _Else) _Then
#define OPENFST_STATUS_IMPL_IF_0(_Then, _Else) _Else
#define OPENFST_STATUS_IMPL_IF(_Cond, _Then, _Else) \
  OPENFST_STATUS_IMPL_CONCAT(OPENFST_STATUS_IMPL_IF_, _Cond)(_Then, _Else)

// Expands to 1 if the input is parenthesized. Otherwise expands to 0.
#define OPENFST_STATUS_IMPL_IS_PARENTHESIZED(...) \
  OPENFST_STATUS_IMPL_IS_EMPTY(OPENFST_STATUS_IMPL_EAT __VA_ARGS__)

// If the input is parenthesized, removes the parentheses. Otherwise expands to
// the input unchanged.
#define OPENFST_STATUS_IMPL_UNPARENTHESIZE_IF_PARENTHESIZED(...)               \
  OPENFST_STATUS_IMPL_IF(OPENFST_STATUS_IMPL_IS_PARENTHESIZED(__VA_ARGS__),    \
                         OPENFST_STATUS_IMPL_REM, OPENFST_STATUS_IMPL_EMPTY()) \
  __VA_ARGS__

}  // namespace fst

#endif  // OPENFST_COMPAT_STATUS_BUILDER_H_
