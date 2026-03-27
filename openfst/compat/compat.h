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

#ifndef OPENFST_COMPAT_COMPAT_H_
#define OPENFST_COMPAT_COMPAT_H_

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "openfst/compat/compat_memory.h"

namespace fst {

// Downcasting.

template <typename To, typename From>
inline To down_cast(From* f) {
  return static_cast<To>(f);
}

template <typename To, typename From>
inline To down_cast(From& f) {
  return static_cast<To>(f);
}

// Bitcasting.
template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  static_assert(sizeof(Dest) == sizeof(Source),
                "Bitcasting unsafe for specified types");
  Dest dest;
  std::memcpy(&dest, &source, sizeof(dest));
  return dest;
}

namespace internal {

// TODO: Remove this once we migrate to C++20.
template <typename T>
struct type_identity {
  using type = T;
};

template <typename T>
using type_identity_t = typename type_identity<T>::type;

}  // namespace internal

template <typename To>
constexpr To implicit_cast(typename internal::type_identity_t<To> to) {
  return to;
}

// Checksums.
class CheckSummer {
 public:
  CheckSummer();

  void Reset();

  void Update(std::string_view data);

  std::string Digest() { return check_sum_; }

 private:
  static constexpr int kCheckSumLength = 32;
  int count_;
  std::string check_sum_;

  CheckSummer(const CheckSummer&) = delete;
  CheckSummer& operator=(const CheckSummer&) = delete;
};

// String munging.

namespace internal {

// Computes size of joined string.
template <class S>
size_t GetResultSize(const std::vector<S>& elements, size_t s_size) {
  const auto lambda = [](size_t partial, const S& right) {
    return partial + right.size();
  };
  return std::accumulate(elements.begin(), elements.end(), 0, lambda) +
         elements.size() * s_size - s_size;
}

}  // namespace internal

inline bool StrContains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != haystack.npos;
}

inline bool StrContains(std::string_view haystack, char needle) {
  return haystack.find(needle) != haystack.npos;
}

template <class S>
std::string StrJoin(const std::vector<S>& elements, std::string_view delim) {
  std::string result;
  if (elements.empty()) return result;
  const size_t s_size = delim.size();
  result.reserve(internal::GetResultSize(elements, s_size));
  auto it = elements.begin();
  result.append(it->data(), it->size());
  for (++it; it != elements.end(); ++it) {
    result.append(delim.data(), s_size);
    result.append(it->data(), it->size());
  }
  return result;
}

template <class S>
std::string StrJoin(const std::vector<S>& elements, char delim) {
  const std::string_view view_delim(&delim, 1);
  return StrJoin(elements, view_delim);
}

struct SkipEmpty {};

struct ByAnyChar {
 public:
  explicit ByAnyChar(std::string_view sp) : delimiters(sp) {}

  std::string delimiters;
};

namespace internal {

class StringSplitter {
 public:
  using const_iterator = std::vector<std::string_view>::const_iterator;
  using value_type = std::string_view;

  StringSplitter(std::string_view string, std::string delim,
                 bool skip_empty = false)
      : string_(std::move(string)),
        delim_(std::move(delim)),
        skip_empty_(skip_empty),
        vec_(SplitToSv()) {}

  inline operator
      std::vector<std::string_view>() && {
    return std::move(vec_);
  }

  inline operator
      std::vector<std::string>() {
    std::vector<std::string> str_vec(vec_.begin(), vec_.end());
    return str_vec;
  }

  const_iterator begin() const { return vec_.begin(); }
  const_iterator end() const { return vec_.end(); }

 private:
  std::vector<std::string_view> SplitToSv();

  std::string_view string_;
  std::string delim_;
  bool skip_empty_;
  std::vector<std::string_view> vec_;
};

}  // namespace internal

// `absl::StrSplit` replacements. Only support splitting on `char` or
// `ByAnyChar` (notable not on a multi-char string delimiter), and with or
// without `SkipEmpty`.
internal::StringSplitter StrSplit(std::string_view full, ByAnyChar delim);
internal::StringSplitter StrSplit(std::string_view full, char delim);
internal::StringSplitter StrSplit(std::string_view full, ByAnyChar delim,
                                  SkipEmpty);
internal::StringSplitter StrSplit(std::string_view full, char delim, SkipEmpty);

void StripTrailingAsciiWhitespace(std::string* full);

std::string_view StripTrailingAsciiWhitespace(std::string_view full);

class StringOrInt {
 public:
  template <typename T, typename = std::enable_if_t<
                            std::is_convertible_v<T, std::string_view>>>
  StringOrInt(T s) : str_(std::string(s)) {}

  StringOrInt(int i) {
    str_ = std::to_string(i);
  }

  const std::string& Get() const { return str_; }

 private:
  std::string str_;
};

inline std::string StrCat(const StringOrInt& s1, const StringOrInt& s2) {
  return s1.Get() + s2.Get();
}

inline std::string StrCat(const StringOrInt& s1, const StringOrInt& s2,
                          const StringOrInt& s3) {
  return s1.Get() + s2.Get() + s3.Get();
}

// For four or more args, wrap them up into an initializer list and use an
// explicit loop.
template <typename... Args>
std::string StrCat(const StringOrInt& s1, const StringOrInt& s2,
                   const StringOrInt& s3, const Args&... args) {
  const std::initializer_list<StringOrInt> list{
      s1, s2, s3, static_cast<const StringOrInt&>(args)...};
  std::ostringstream ostrm;
  for (const auto& s : list) ostrm << s.Get();
  return ostrm.str();
}

// TODO: Remove this once we migrate to C++20, where `starts_with`
// is available.
inline bool StartsWith(std::string_view text, std::string_view prefix) {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          std::memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

inline bool ConsumePrefix(std::string_view* s, std::string_view expected) {
  if (!StartsWith(*s, expected)) return false;
  s->remove_prefix(expected.size());
  return true;
}

template <typename IntType>
inline bool SimpleAtoi(std::string_view text, IntType* out) {
  if (out == nullptr) return false;
  *out = std::atoll(text.data());
  return true;
}

namespace internal {

// Random data taken from the hexadecimal digits of Pi's fractional component.
// https://en.wikipedia.org/wiki/Nothing-up-my-sleeve_number
static constexpr uint64_t kStaticRandomData[8] = {
    0x243f'6a88'85a3'08d3, 0x1319'8a2e'0370'7344, 0xa409'3822'299f'31d0,
    0x082e'fa98'ec4e'6c89, 0x4528'21e6'38d0'1377, 0xbe54'66cf'34e9'0c6c,
    0xc0ac'29b7'c97c'50dd, 0x3f84'd5b5'b547'0917,
};

inline size_t HashOfImpl(size_t arg_index) {
  return kStaticRandomData[arg_index % 8];
}

template <typename First, typename... T>
size_t HashOfImpl(size_t arg_index, const First& value, const T&... args) {
  static_assert(std::is_integral_v<First>);
  auto v = static_cast<std::make_unsigned_t<First>>(value);
  return kStaticRandomData[arg_index % 8] * v +
         HashOfImpl(arg_index + 1, args...);
}

}  // namespace internal

template <typename... T>
size_t HashOf(const T&... args) {
  return internal::HashOfImpl(0, args...);
}

}  // namespace fst

#endif  // OPENFST_COMPAT_COMPAT_H_
