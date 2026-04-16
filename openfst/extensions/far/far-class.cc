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

#include "openfst/extensions/far/far-class.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far.h"
#include "openfst/extensions/far/script-impl.h"
#include "openfst/lib/arc.h"
#include "openfst/script/script-impl.h"

namespace fst {
namespace script {

// FarReaderClass.

absl_nullable std::unique_ptr<FarReaderClass> FarReaderClass::Open(
    absl::string_view source) {
  const std::vector<std::string> sources{std::string(source)};
  return FarReaderClass::Open(sources);
}

absl_nullable std::unique_ptr<FarReaderClass> FarReaderClass::Open(
    const std::vector<std::string>& sources) {
  if (sources.empty()) {
    LOG(ERROR) << "FarReaderClass::Open: No files specified";
    return nullptr;
  }
  auto arc_type = LoadArcTypeFromFar(sources.front());
  if (arc_type.empty()) {
    LOG(ERROR) << "FarReaderClass::Open: File could not be opened: "
               << sources.front();
    return nullptr;
  }
  // TODO: What if we have an empty FAR for the first one,
  // then non-empty?  We need to check all input FARs.
  OpenFarReaderClassArgs args(sources);
  args.retval = nullptr;
  Apply<Operation<OpenFarReaderClassArgs>>("OpenFarReaderClass", arc_type,
                                           &args);
  return std::move(args.retval);
}

REGISTER_FST_OPERATION(OpenFarReaderClass, StdArc, OpenFarReaderClassArgs);
REGISTER_FST_OPERATION(OpenFarReaderClass, LogArc, OpenFarReaderClassArgs);
REGISTER_FST_OPERATION(OpenFarReaderClass, Log64Arc, OpenFarReaderClassArgs);
REGISTER_FST_OPERATION(OpenFarReaderClass, ErrorArc, OpenFarReaderClassArgs);

// FarWriterClass.

std::unique_ptr<FarWriterClass> FarWriterClass::Create(
    const std::string& source, const std::string& arc_type, FarType type) {
  CreateFarWriterClassInnerArgs iargs(source, type);
  CreateFarWriterClassArgs args(iargs);
  args.retval = nullptr;
  Apply<Operation<CreateFarWriterClassArgs>>("CreateFarWriterClass", arc_type,
                                             &args);
  return std::move(args.retval);
}

REGISTER_FST_OPERATION(CreateFarWriterClass, StdArc, CreateFarWriterClassArgs);
REGISTER_FST_OPERATION(CreateFarWriterClass, LogArc, CreateFarWriterClassArgs);
REGISTER_FST_OPERATION(CreateFarWriterClass, Log64Arc,
                       CreateFarWriterClassArgs);
REGISTER_FST_OPERATION(CreateFarWriterClass, ErrorArc,
                       CreateFarWriterClassArgs);

}  // namespace script
}  // namespace fst
