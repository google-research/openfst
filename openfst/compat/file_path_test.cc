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

#include "openfst/compat/file_path.h"

#include "gtest/gtest.h"

namespace fst {
namespace {

TEST(FilePathTest, CheckJoinPath) {
  EXPECT_EQ("", JoinPath("", ""));
  EXPECT_EQ("hello", JoinPath("", "hello"));
  EXPECT_EQ("hello", JoinPath("hello", ""));
  EXPECT_EQ("hello/", JoinPath("hello/", ""));
  EXPECT_EQ("hello/world", JoinPath("hello", "world"));
  EXPECT_EQ("hello/world", JoinPath("hello/", "world"));
  EXPECT_EQ("hello/world", JoinPath("hello/", "/world"));
  EXPECT_EQ("hello/", JoinPath("hello/", ""));
  EXPECT_EQ("/world", JoinPath("", "/world"));
}

TEST(FilePathTest, CheckJoinPathRespectAbsolute) {
  EXPECT_EQ("", JoinPathRespectAbsolute("", ""));
  EXPECT_EQ("hello", JoinPathRespectAbsolute("", "hello"));
  EXPECT_EQ("hello", JoinPathRespectAbsolute("hello", ""));
  EXPECT_EQ("hello/", JoinPathRespectAbsolute("hello/", ""));
  EXPECT_EQ("hello/world", JoinPathRespectAbsolute("hello", "world"));
  EXPECT_EQ("hello/world", JoinPathRespectAbsolute("hello/", "world"));
  EXPECT_EQ("/world", JoinPathRespectAbsolute("hello/", "/world"));
}

TEST(FilePathTest, CheckJoinPathThreeComponents) {
  EXPECT_EQ("", JoinPath("", "", ""));
  EXPECT_EQ("hello", JoinPath("", "", "hello"));
  EXPECT_EQ("hello/world", JoinPath("hello", "world", ""));
  EXPECT_EQ("hello/world/", JoinPath("hello/", "world/", ""));
  EXPECT_EQ("hello/world/", JoinPath("hello", "/world/", ""));
  EXPECT_EQ("hello/world/test", JoinPath("hello", "world", "test"));
}

TEST(FilePathTest, CheckBasename) {
  EXPECT_EQ("", Basename(""));
  EXPECT_EQ("", Basename("/"));
  EXPECT_EQ("hello", Basename("hello"));
  EXPECT_EQ("hello", Basename("/a/b/c/hello"));
}

TEST(FilePathTest, CheckDirname) {
  EXPECT_EQ("/hello", Dirname("/hello/"));
  EXPECT_EQ("/", Dirname("/hello"));
  EXPECT_EQ("/hello", Dirname("/hello/world"));
  EXPECT_EQ("hello", Dirname("hello/world"));
  EXPECT_EQ("hello", Dirname("hello/"));
  EXPECT_EQ("", Dirname("world"));
  EXPECT_EQ("/", Dirname("/"));
  EXPECT_EQ("", Dirname(""));
}

TEST(FilePathTest, CheckExtension) {
  EXPECT_EQ("gif", Extension("foo.gif"));
  EXPECT_EQ("", Extension("foo."));
  EXPECT_EQ("", Extension(""));
  EXPECT_EQ("", Extension("/"));
  EXPECT_EQ("", Extension("foo"));
  EXPECT_EQ("", Extension("foo/"));
  EXPECT_EQ("gif", Extension("/a/path/to/foo.gif"));
  EXPECT_EQ("html", Extension("/a/path.bar/to/foo.html"));
  EXPECT_EQ("", Extension("/a/path.bar/to/foo"));
  EXPECT_EQ("baz", Extension("/a/path.bar/to/foo.bar.baz"));
}

TEST(FilePathTest, CheckStem) {
  EXPECT_EQ("foo", Stem("foo.gif"));
  EXPECT_EQ("foo", Stem("foo."));
  EXPECT_EQ("", Stem(""));
  EXPECT_EQ("", Stem("/"));
  EXPECT_EQ("foo", Stem("foo"));
  EXPECT_EQ("", Stem("foo/"));
  EXPECT_EQ("foo", Stem("/a/path/to/foo.gif"));
  EXPECT_EQ("foo", Stem("/a/path.bar/to/foo.html"));
  EXPECT_EQ("foo", Stem("/a/path.bar/to/foo"));
  EXPECT_EQ("foo.bar", Stem("/a/path.bar/to/foo.bar.baz"));
}

}  // namespace
}  // namespace fst
