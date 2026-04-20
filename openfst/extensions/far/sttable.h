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
//
// A generic string-to-type table file format.
//
// This is not meant as a generalization of SSTable. This is more of a simple
// replacement for SSTable in order to provide an open-source implementation
// of the FAR format for the external version of the FST library.

#ifndef OPENFST_EXTENSIONS_FAR_STTABLE_H_
#define OPENFST_EXTENSIONS_FAR_STTABLE_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/macros.h"
#include "absl/base/nullability.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/file-stream-status.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/util.h"

namespace fst {

inline constexpr int32_t kSTTableMagicNumber = 2125656924;
inline constexpr int32_t kSTTableFileVersion = 1;

// String-type table writing class for an object of type T using a functor
// Writer. The Writer functor must provide at least the following interface:
//
//   struct Writer {
//     void operator()(std::ostream &, const T &) const;
//   };
template <class T, class Writer>
class STTableWriter {
 public:
  explicit STTableWriter(absl::string_view source)
      : stream_(
            // In portable mode, `absl::string_view` is not supported.
            std::string(source), std::ios_base::out | std::ios_base::binary),
        error_(false) {
    WriteType(stream_, kSTTableMagicNumber);
    WriteType(stream_, kSTTableFileVersion);
    if (stream_.fail()) {
      FSTERROR() << "STTableWriter::STTableWriter: Error writing to file: "
                 << source;
      error_ = true;
    }
  }

  static STTableWriter<T, Writer>* Create(absl::string_view source) {
    if (source.empty()) {
      LOG(ERROR) << "STTableWriter: Writing to standard out unsupported.";
      return nullptr;
    }
    return new STTableWriter<T, Writer>(source);
  }

  void Add(absl::string_view key, const T& t) {
    if (key.empty()) {
      FSTERROR() << "STTableWriter::Add: Key empty: " << key;
      error_ = true;
    } else if (key < last_key_) {
      FSTERROR() << "STTableWriter::Add: Key out of order: " << key;
      error_ = true;
    }
    if (error_) return;
    last_key_.assign(key.data(), key.size());
    positions_.push_back(stream_.tellp());
    WriteType(stream_, key);
    entry_writer_(stream_, t);
  }

  bool Error() const { return error_; }

  ~STTableWriter() {
    WriteType(stream_, positions_);
    WriteType(stream_, static_cast<int64_t>(positions_.size()));
  }

 private:
  Writer entry_writer_;
  file::FileOutStream stream_;
  std::vector<int64_t> positions_;  // Position in file of each key-entry pair.
  std::string last_key_;            // Last key.
  bool error_;

  STTableWriter(const STTableWriter&) = delete;
  STTableWriter& operator=(const STTableWriter&) = delete;
};

// String-type table reading class for object of type T using a functor Reader.
// Reader must provide at least the following interface:
//
//   struct Reader {
//     T *operator()(std::istream &) const;
//   };
//
template <class T, class Reader>
class STTableReader {
 public:
  [[deprecated("Use OpenWithStatus instead.")]]  //
  explicit STTableReader(std::vector<std::string> sources) {
    absl::StatusOr<InitArgs> args = PrepareInitArgs(sources);
    sources_ = std::move(sources);
    if (args.ok()) {
      streams_ = std::move(args->streams);
      positions_ = std::move(args->positions);
      keys_.resize(streams_.size());
      error_ = false;
      MakeHeap();
    } else {
      FSTERROR() << "STTableReader: " << args.status();
      error_ = true;
    }
  }

  static absl::StatusOr<absl_nonnull std::unique_ptr<STTableReader<T, Reader>>>
  OpenWithStatus(absl::string_view source) {
    if (source.empty()) {
      return absl::InvalidArgumentError(
          "`source` must not be empty (reading from stdin in not supported)");
    }
    return OpenWithStatus(std::vector<std::string>{{std::string(source)}});
  }

  // clang-format off
  [[deprecated("Use OpenWithStatus instead.")]] ABSL_REFACTOR_INLINE
  static STTableReader<T, Reader>* Open(absl::string_view source) {
    auto reader = OpenWithStatus(source);
    if (!reader.ok()) FSTERROR() << "STTableReader: " << reader.status();
    return reader.ok() ? reader->release() : nullptr;
  }
  // clang-format on

  static absl::StatusOr<absl_nonnull std::unique_ptr<STTableReader<T, Reader>>>
  OpenWithStatus(std::vector<std::string> sources) {
    absl::StatusOr<InitArgs> args = PrepareInitArgs(sources);
    if (!args.ok()) return args.status();
    return absl::WrapUnique(
        new STTableReader<T, Reader>(std::move(sources), *std::move(args)));
  }

  // clang-format off
  [[deprecated("Use OpenWithStatus instead.")]] ABSL_REFACTOR_INLINE
  static STTableReader<T, Reader>* Open(std::vector<std::string> sources) {
    auto reader = OpenWithStatus(std::move(sources));
    if (!reader.ok()) FSTERROR() << "STTableReader: " << reader.status();
    return reader.ok() ? reader->release() : nullptr;
  }
  // clang-format on

  void Reset() {
    if (error_) return;
    for (size_t i = 0; i < streams_.size(); ++i) {
      if (!positions_[i].empty()) {
        streams_[i]->seekg(positions_[i].front());
      }
    }
    MakeHeap();
  }

  bool Find(absl::string_view key) {
    if (error_) return false;
    for (size_t i = 0; i < streams_.size(); ++i) LowerBound(i, key);
    MakeHeap();
    if (heap_.empty()) return false;
    return keys_[current_] == key;
  }

  bool Done() const { return error_ || heap_.empty(); }

  void Next() {
    if (error_) return;
    if (streams_[current_]->tellg() <= positions_[current_].back()) {
      ReadType(*(streams_[current_]), &(keys_[current_]));
      if (streams_[current_]->fail()) {
        FSTERROR() << "STTableReader: Error reading file: "
                   << sources_[current_];
        error_ = true;
        return;
      }
      std::push_heap(heap_.begin(), heap_.end(), Compare{&keys_});
    } else {
      heap_.pop_back();
    }
    if (!heap_.empty()) PopHeap();
  }

  const std::string& GetKey() const { return keys_[current_]; }

  const T* GetEntry() const { return entry_.get(); }

  bool Error() const { return error_; }

 private:
  // Constructor arguments, in a separate struct because they're passed
  // both from the factory function and the deprecated constructor.
  struct InitArgs {
    std::vector<std::unique_ptr<file::FileInStream>> streams;
    std::vector<std::vector<int64_t>> positions;
  };

  // Comparison functor used to compare stream IDs in the heap.
  struct Compare {
    explicit Compare(const std::vector<std::string>* keys) : keys(keys) {}

    bool operator()(size_t i, size_t j) const {
      return (*keys)[i] > (*keys)[j];
    }

   private:
    const std::vector<std::string>* keys;
  };

  // Positions the stream at the position corresponding to the lower bound for
  // the specified key.
  void LowerBound(size_t id, absl::string_view find_key) {
    auto* strm = streams_[id].get();
    const auto& positions = positions_[id];
    if (positions.empty()) return;
    size_t low = 0;
    size_t high = positions.size() - 1;
    std::string key;
    while (low < high) {
      const size_t mid = (low + high) / 2;
      strm->seekg(positions[mid]);
      ReadType(*strm, &key);
      if (key > find_key) {
        high = mid;
      } else if (key < find_key) {
        low = mid + 1;
      } else {
        for (size_t i = mid; i > low; --i) {
          strm->seekg(positions[i - 1]);
          ReadType(*strm, &key);
          if (key != find_key) {
            strm->seekg(positions[i]);
            return;
          }
        }
        strm->seekg(positions[low]);
        return;
      }
    }
    strm->seekg(positions[low]);
  }

  // Adds all streams to the heap.
  void MakeHeap() {
    heap_.clear();
    for (size_t i = 0; i < streams_.size(); ++i) {
      if (positions_[i].empty()) continue;
      ReadType(*streams_[i], &(keys_[i]));
      if (streams_[i]->fail()) {
        FSTERROR() << "STTableReader: Error reading file: " << sources_[i];
        error_ = true;
        return;
      }
      heap_.push_back(i);
    }
    if (heap_.empty()) return;
    std::make_heap(heap_.begin(), heap_.end(), Compare{&keys_});
    PopHeap();
  }

  // Positions the stream with the lowest key at the top of the heap, sets
  // current_ to the ID of that stream, and reads the current entry from that
  // stream.
  void PopHeap() {
    std::pop_heap(heap_.begin(), heap_.end(), Compare{&keys_});
    current_ = heap_.back();
    entry_.reset(entry_reader_(*streams_[current_]));
    if (!entry_) error_ = true;
    if (streams_[current_]->fail()) {
      FSTERROR() << "STTableReader: Error reading entry for key: "
                 << keys_[current_] << ", file: " << sources_[current_];
      error_ = true;
    }
  }

  // Opens the streams, reads the header information, and returns the
  // constructor arguments.
  static absl::StatusOr<InitArgs> PrepareInitArgs(
      absl::Span<const std::string> sources) {
    std::vector<std::unique_ptr<file::FileInStream>> streams(sources.size());
    std::vector<std::vector<int64_t>> positions(sources.size());
    for (size_t i = 0; i < sources.size(); ++i) {
      streams[i] = std::make_unique<file::FileInStream>(
          sources[i], std::ios_base::in | std::ios_base::binary);
      if (const absl::Status status = GetFileStreamStatus(*streams[i]);
          !status.ok()) {
        return absl::Status(status.code(),
                            absl::StrCat(status.message(), ":", sources[i]));
      }
      int32_t magic_number = 0;
      ReadType(*streams[i], &magic_number);
      int32_t file_version = 0;
      ReadType(*streams[i], &file_version);
      if (magic_number != kSTTableMagicNumber) {
        return absl::InvalidArgumentError(
            absl::StrCat("Wrong file type: ", sources[i], ", expected ",
                         kSTTableMagicNumber, ", got ", magic_number));
      }
      if (file_version != kSTTableFileVersion) {
        return absl::InvalidArgumentError(
            absl::StrCat("Wrong file version: ", sources[i], ", expected ",
                         kSTTableFileVersion, ", got ", file_version));
      }
      int64_t num_entries;
      streams[i]->seekg(-static_cast<int>(sizeof(int64_t)), std::ios_base::end);
      ReadType(*streams[i], &num_entries);
      if (num_entries > 0) {
        streams[i]->seekg(
            -static_cast<int>(sizeof(int64_t)) * (num_entries + 1),
            std::ios_base::end);
        positions[i].resize(num_entries);
        for (size_t j = 0; (j < num_entries) && (!streams[i]->fail()); ++j) {
          ReadType(*streams[i], &(positions[i][j]));
        }
        streams[i]->seekg(positions[i][0]);
        if (const absl::Status status = GetFileStreamStatus(*streams[i]);
            !status.ok()) {
          return absl::Status(status.code(),
                              absl::StrCat(status.message(), ":", sources[i]));
        }
      }
    }
    return InitArgs{std::move(streams), std::move(positions)};
  }

  explicit STTableReader(std::vector<std::string>&& sources, InitArgs&& args)
      : sources_(std::move(sources)),
        streams_(std::move(args.streams)),
        positions_(std::move(args.positions)),
        keys_(sources_.size()),
        error_(false) {
    MakeHeap();
  }

  Reader entry_reader_;
  std::vector<std::string> sources_;  // Input file names.
  std::vector<std::unique_ptr<file::FileInStream>>
      streams_;                                  // Corresponding input streams.
  std::vector<std::vector<int64_t>> positions_;  // Index of positions.
  std::vector<std::string> keys_;  // Lowest unread key for each stream.
  std::vector<int64_t>
      heap_;         // Heap containing ID of streams with unread keys.
  int64_t current_;  // ID of current stream to be read.
  mutable std::unique_ptr<T> entry_;  // The currently read entry.
  bool error_;
};

// String-type table header reading function template on the entry header type.
// The Header type must provide at least the following interface:
//
//   struct Header {
//     void Read(std::istream &istrm, const string &source);
//   };
template <class Header>
bool ReadSTTableHeader(const std::string& source, Header* header) {
  if (source.empty()) {
    LOG(ERROR) << "ReadSTTable: Can't read header from standard input";
    return false;
  }
  file::FileInStream strm(source, std::ios_base::in | std::ios_base::binary);
  if (!strm) {
    LOG(ERROR) << "ReadSTTableHeader: Could not open file: " << source;
    return false;
  }
  int32_t magic_number = 0;
  ReadType(strm, &magic_number);
  int32_t file_version = 0;
  ReadType(strm, &file_version);
  if (magic_number != kSTTableMagicNumber) {
    LOG(ERROR) << "ReadSTTableHeader: Wrong file type: " << source;
    return false;
  }
  if (file_version != kSTTableFileVersion) {
    LOG(ERROR) << "ReadSTTableHeader: Wrong file version: " << source;
    return false;
  }
  int64_t i = -1;
  strm.seekg(-static_cast<int>(sizeof(int64_t)), std::ios_base::end);
  ReadType(strm, &i);  // Reads number of entries
  if (strm.fail()) {
    LOG(ERROR) << "ReadSTTableHeader: Error reading file: " << source;
    return false;
  }
  if (i == 0) return true;  // No entry header to read.
  strm.seekg(-2 * static_cast<int>(sizeof(int64_t)), std::ios_base::end);
  ReadType(strm, &i);  // Reads position for last entry in file.
  strm.seekg(i);
  std::string key;
  ReadType(strm, &key);
  if (!header->Read(strm, source + ":" + key)) {
    LOG(ERROR) << "ReadSTTableHeader: Error reading FstHeader: " << source;
    return false;
  }
  if (strm.fail()) {
    LOG(ERROR) << "ReadSTTableHeader: Error reading file: " << source;
    return false;
  }
  return true;
}

bool IsSTTable(absl::string_view source);

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_STTABLE_H_
