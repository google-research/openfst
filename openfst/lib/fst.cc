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
// FST definitions.

#include "openfst/lib/fst.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

#include "absl/base/nullability.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"

// FST flag definitions.

ABSL_FLAG(bool, fst_verify_properties, false,
          "Verify FST properties queried by TestProperties");

ABSL_FLAG(bool, fst_default_cache_gc, true,
          "Enable garbage collection of cache");

ABSL_FLAG(int64_t, fst_default_cache_gc_limit, 1 << 20LL,
          "Cache byte size that triggers garbage collection");

ABSL_FLAG(bool, fst_align, false, "Write FST data aligned where appropriate");

ABSL_FLAG(std::string, save_relabel_ipairs, "",
          "Save input relabel pairs to file");
ABSL_FLAG(std::string, save_relabel_opairs, "",
          "Save output relabel pairs to file");

ABSL_FLAG(std::string, fst_read_mode, "read",
          "Default file reading mode for mappable files");

namespace fst {

// Checks FST magic number and reads in the header; if rewind = true,
// the stream is repositioned before call if possible.
bool FstHeader::Read(std::istream& strm, const std::string& source,
                     bool rewind) {
  int64_t pos = 0;
  if (rewind) pos = strm.tellg();
  int32_t magic_number = 0;
  ReadType(strm, &magic_number);
  if (magic_number != kFstMagicNumber) {
    LOG(ERROR) << "FstHeader::Read: Bad FST header: " << source
               << ". Magic number not matched. Got: " << magic_number;
    if (rewind) strm.seekg(pos);
    return false;
  }
  ReadType(strm, &fsttype_);
  ReadType(strm, &arctype_);
  ReadType(strm, &version_);
  ReadType(strm, &flags_);
  ReadType(strm, &properties_);
  ReadType(strm, &start_);
  ReadType(strm, &numstates_);
  ReadType(strm, &numarcs_);
  if (!strm) {
    LOG(ERROR) << "FstHeader::Read: Read failed: " << source;
    return false;
  }
  if (rewind) strm.seekg(pos);
  return true;
}

// Writes FST magic number and FST header.
bool FstHeader::Write(std::ostream& strm, absl::string_view) const {
  WriteType(strm, kFstMagicNumber);
  WriteType(strm, fsttype_);
  WriteType(strm, arctype_);
  WriteType(strm, version_);
  WriteType(strm, flags_);
  WriteType(strm, properties_);
  WriteType(strm, start_);
  WriteType(strm, numstates_);
  WriteType(strm, numarcs_);
  return true;
}

std::string FstHeader::DebugString() const {
  return absl::StrCat("fsttype: \"", fsttype_, "\" arctype: \"", arctype_,
                      "\" version: \"", version_, "\" flags: \"", flags_,
                      "\" properties: \"", properties_, "\" start: \"", start_,
                      "\" numstates: \"", numstates_, "\" numarcs: \"",
                      numarcs_, "\"");
}

FstReadOptions::FstReadOptions(const absl::string_view source,
                               const FstHeader* absl_nullable header,
                               const SymbolTable* absl_nullable isymbols,
                               const SymbolTable* absl_nullable osymbols)
    : source(source),
      header(header),
      isymbols(isymbols),
      osymbols(osymbols),
      read_isymbols(true),
      read_osymbols(true) {
  mode = ReadMode(absl::GetFlag(FLAGS_fst_read_mode));
}

FstReadOptions::FstReadOptions(const absl::string_view source,
                               const SymbolTable* isymbols,
                               const SymbolTable* osymbols)
    : FstReadOptions(source, /*header=*/nullptr, isymbols, osymbols) {}

FstReadOptions::FileReadMode FstReadOptions::ReadMode(absl::string_view mode) {
  if (mode == "read") return READ;
  if (mode == "map") return MAP;
  LOG(ERROR) << "Unknown file read mode " << mode;
  return READ;
}

std::string FstReadOptions::DebugString() const {
  return absl::StrCat(
      "source: \"", source, "\" mode: \"", (mode == READ ? "READ" : "MAP"),
      "\" read_isymbols: \"", (read_isymbols ? "true" : "false"),
      "\" read_osymbols: \"", (read_osymbols ? "true" : "false"),
      "\" header: \"", (header ? "set" : "null"), "\" isymbols: \"",
      (isymbols ? "set" : "null"), "\" osymbols: \"",
      (osymbols ? "set" : "null"), "\"");
}

}  // namespace fst
