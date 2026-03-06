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

#include "openfst/lib/mapped-file.h"

#include <errno.h>
#include <fcntl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "openfst/lib/file-util.h"

#if defined(_WIN32)
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#endif  // _WIN32
#if !defined(_WIN32) && !defined(O_TMPFILE)
#include "openfst/compat/file_path.h"
#endif  // _WIN32 || O_TMPFILE

namespace fst {
namespace {
// Calls ::close on fd on destruction.
struct ScopedFd {
  explicit ScopedFd(int fd) : fd(fd) {}
  ~ScopedFd() { ::close(fd); }
  const int fd;
};

ScopedFd OpenReadOnly(const std::string& filename) {
#ifdef _WIN32
  return ScopedFd(::_open(filename.c_str(), _O_RDONLY));
#else
  return ScopedFd(::open(filename.c_str(), O_RDONLY));
#endif  // _WIN32
}

// TODO: We can make this implementatio cleaner.
ScopedFd OpenWriteOnlyTemp() {
#ifdef _WIN32
  char buffer[PATH_MAX + 1];
  const DWORD ret = GetTempPathA(ARRAYSIZE(buffer), buffer);
  EXPECT_NE(0, ret) << "Unable to get temporary path: " << GetLastError();
  if (ret == 0) {
    return ScopedFd(-1);
  }
  const int fd =
      ::_open(absl::StrCat(absl::string_view(buffer, ret), "/tmpfile").c_str(),
              _O_WRONLY | _O_CREAT | _O_TEMPORARY, _S_IREAD | _S_IWRITE);
#elif defined(O_TMPFILE)
  // Linux platforms only.
  // See https://man7.org/linux/man-pages/man2/open.2.html.
  const int fd =
      ::open(::testing::TempDir().c_str(), O_WRONLY | O_TMPFILE, 644);
#else
  std::string path_template =
      JoinPath(::testing::TempDir(), "mapped-XXXXXX");
  // Not part of std:: namespace.
  const int fd = mkstemp(path_template.data());
#endif  // _WIN32
  return ScopedFd(fd);
}

class MappedFileTest : public testing::Test {
 protected:
  void SetUp() override {
    filename_ = "openfst/test/testdata/mapped-file/heart.txt";
  }
  std::string filename_;
};

TEST_F(MappedFileTest, TestAllocating) {
  std::string test_string =
      "alas, I have but one std::string to give for my std::cout";
  std::istringstream istr(test_string);
  // Map with memorymap=false allocates with kArchAlignment.
  std::unique_ptr<MappedFile> m(
      MappedFile::Map(istr, false, "", test_string.length()));
  ASSERT_TRUE(m);
  ASSERT_TRUE(m->data());
  EXPECT_EQ(
      0, reinterpret_cast<uintptr_t>(m->data()) % MappedFile::kArchAlignment);
  EXPECT_EQ(0, strncmp(test_string.c_str(), static_cast<const char*>(m->data()),
                       test_string.size()));
}

TEST_F(MappedFileTest, TestAllocateAlignment) {
  const std::string test_string =
      "alas, I have but one std::string to give for my std::cout";
  // Now allocate with some other alignments.
  for (int log_align = 0; log_align < 10; ++log_align) {
    const size_t align = size_t{1} << log_align;
    auto m = absl::WrapUnique(MappedFile::Allocate(test_string.size(), align));
    EXPECT_EQ(0, reinterpret_cast<uintptr_t>(m->data()) % align) << align;
    char* const region = static_cast<char*>(m->mutable_data());
    // Check that we can copy the string into the region without a segfault.
    std::copy_n(test_string.begin(), test_string.size(), region);
    EXPECT_EQ(0, strncmp(test_string.c_str(), region, test_string.size()));
  }
}

template <typename T>
void TestAllocateType() {
  const size_t align = alignof(T);
  auto m = absl::WrapUnique(MappedFile::AllocateType<T>(3));
  EXPECT_EQ(0, reinterpret_cast<uintptr_t>(m->data()) % align) << align;
  T* const region = static_cast<T*>(m->mutable_data());
  region[0] = 10;
  region[1] = 20;
  region[2] = 30;
  EXPECT_EQ(10, region[0]);
  EXPECT_EQ(20, region[1]);
  EXPECT_EQ(30, region[2]);
}

TEST_F(MappedFileTest, TestAllocateTypes) {
  TestAllocateType<char>();
  TestAllocateType<int>();
  TestAllocateType<uint64_t>();
  TestAllocateType<double>();
  TestAllocateType<long double>();
}

TEST_F(MappedFileTest, TestMappingFile) {
  file::FileInStream input_stream(filename_);
  ASSERT_TRUE(input_stream) << "Could not open testdata file " << filename_;
  ASSERT_TRUE(input_stream.seekg(12));  // skip here, here!
  std::unique_ptr<MappedFile> m1(
      MappedFile::Map(input_stream, false, filename_, 21));
  ASSERT_TRUE(m1);
  ASSERT_TRUE(m1->data());
  EXPECT_EQ(0, strncmp("it is the beating of ",
                       static_cast<const char*>(m1->data()), 21))
      << static_cast<const char*>(m1->data());
  std::unique_ptr<MappedFile> m2(
      MappedFile::Map(input_stream, false, filename_, 18));
  ASSERT_TRUE(m2);
  ASSERT_TRUE(m2->data());
  EXPECT_EQ(0, strncmp("his hideous heart!",
                       static_cast<const char*>(m2->data()), 18))
      << '"' << static_cast<const char*>(m2->data()) << '"';
}

TEST_F(MappedFileTest, TestMappingGoogleFile) {
  file::FileInStream input_stream(filename_);
  ASSERT_TRUE(input_stream) << "Could not open testdata file " << filename_;
  ASSERT_TRUE(input_stream.seekg(12));  // skip here, here!
  std::unique_ptr<MappedFile> m1(
      MappedFile::Map(input_stream, false, filename_, 21));
  ASSERT_TRUE(m1);
  ASSERT_TRUE(m1->data());
  EXPECT_EQ(0, strncmp("it is the beating of ",
                       static_cast<const char*>(m1->data()), 21))
      << '"' << static_cast<const char*>(m1->data()) << '"';
  std::unique_ptr<MappedFile> m2(
      MappedFile::Map(input_stream, false, filename_, 18));
  ASSERT_TRUE(m2->data());
  EXPECT_EQ(0, strncmp("his hideous heart!",
                       static_cast<const char*>(m2->data()), 18))
      << '"' << static_cast<const char*>(m2->data()) << '"';
}

TEST_F(MappedFileTest, TestMappingFileDescriptor) {
  const ScopedFd fd = OpenReadOnly(filename_);
  ASSERT_NE(fd.fd, -1) << "Could not open testdata file " << filename_;
  std::unique_ptr<MappedFile> m1(
      MappedFile::MapFromFileDescriptor(fd.fd, /*pos=*/12, /*size=*/21));
  ASSERT_TRUE(m1);
  ASSERT_TRUE(m1->data());
  EXPECT_EQ(0, strncmp("it is the beating of ",
                       static_cast<const char*>(m1->data()), 21))
      << static_cast<const char*>(m1->data());
  std::unique_ptr<MappedFile> m2(
      MappedFile::MapFromFileDescriptor(fd.fd, /*pos=*/12 + 21, /*size=*/18));
  ASSERT_TRUE(m2);
  ASSERT_TRUE(m2->data());
  EXPECT_EQ(0, strncmp("his hideous heart!",
                       static_cast<const char*>(m2->data()), 18))
      << '"' << static_cast<const char*>(m2->data()) << '"';
}

TEST_F(MappedFileTest, TestMappingFileDescriptorFails) {
  std::unique_ptr<MappedFile> m1(
      MappedFile::MapFromFileDescriptor(-1, /*pos=*/12, /*size=*/21));
  EXPECT_TRUE(m1 == nullptr);
  const ScopedFd fd = OpenWriteOnlyTemp();
  ASSERT_NE(fd.fd, -1) << "Could not open file in temp dir";
  std::unique_ptr<MappedFile> m2(
      MappedFile::MapFromFileDescriptor(fd.fd, /*pos=*/0, /*size=*/12));
  // TODO: Following test only works on Linux and Windows. Fix.
#if defined(_WIN32) || defined(O_TMPFILE)
  EXPECT_TRUE(m2 == nullptr);
#endif  // _WIN32 || O_TMPFILE
}

TEST_F(MappedFileTest, TestNoSuchFile) {
  std::istringstream input_stream;
  std::unique_ptr<MappedFile> m1(
      MappedFile::Map(input_stream, false, "/dev/nosuchfile", 21));
  EXPECT_TRUE(!m1);
}

TEST_F(MappedFileTest, ReadPastEnd) {
  file::FileInStream input_stream(filename_);
  ASSERT_TRUE(input_stream) << "Could not open testdata file " << filename_;
  std::unique_ptr<MappedFile> m1(
      MappedFile::Map(input_stream, false, filename_, 1024));
  EXPECT_TRUE(!m1);
}
}  // namespace
}  // namespace fst
