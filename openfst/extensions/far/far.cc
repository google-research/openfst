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

#include "openfst/extensions/far/far.h"

#include <cstdint>
#include <ios>
#include <memory>
#include <sstream>
#include <string>

#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/stlist.h"
#include "openfst/extensions/far/sttable.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/util.h"


namespace fst {

bool IsFst(absl::string_view source) {
  file::FileInStream strm(
      // In portable mode, `absl::string_view` is not supported.
      std::string(source), std::ios_base::in | std::ios_base::binary);
  if (!strm) return false;
  int32_t magic_number = 0;
  ReadType(strm, &magic_number);
  bool match = magic_number == kFstMagicNumber;
  return match;
}

bool FarHeader::Read(const std::string& source) {
  FstHeader fsthdr;
  arctype_ = "unknown";
  // This function assumes that opening `source` multiple times will
  // produce the same sequence of bytes.  This is definitely not the case
  // for `/dev/stdin`.  Of course, there are many other ways to spell this,
  // and we can't detect them all, but we make a tiny effort to provide a
  // reasonable warning.  Maybe we should be checking for a regular file
  // instead?
  if (source.empty() || source == "/dev/stdin") {
    // Header reading unsupported on stdin. Assumes STList and StdArc.
    LOG(WARNING) << "Cannot reopen stdin; assuming far_type=stlist and "
                    "arc_type=standard; either may be wrong.";
    fartype_ = FarType::STLIST;
    arctype_ = "standard";
    return true;
  } else if (IsSTTable(source)) {  // Checks if STTable.
    fartype_ = FarType::STTABLE;
    if (!ReadSTTableHeader(source, &fsthdr)) return false;
    arctype_ = fsthdr.ArcType().empty() ? ErrorArc::Type() : fsthdr.ArcType();
    return true;
  } else if (IsSTList(source)) {  // Checks if STList.
    fartype_ = FarType::STLIST;
    if (!ReadSTListHeader(source, &fsthdr)) return false;
    arctype_ = fsthdr.ArcType().empty() ? ErrorArc::Type() : fsthdr.ArcType();
    return true;
  } else if (IsFst(source)) {  // Checks if FST.
    fartype_ = FarType::FST;
    file::FileInStream istrm(source, std::ios_base::in | std::ios_base::binary);
    if (!fsthdr.Read(istrm, source)) return false;
    arctype_ = fsthdr.ArcType().empty() ? ErrorArc::Type() : fsthdr.ArcType();
    return true;
  }
  return false;
}

}  // namespace fst
