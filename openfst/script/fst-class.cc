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
// These classes are only recommended for use in high-level scripting
// applications. Most users should use the lower-level templated versions
// corresponding to these classes.

#include "openfst/script/fst-class.h"

#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <string>

#include "absl/base/casts.h"
#include "absl/base/nullability.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace script {
namespace {

// Helper functions.

template <class F>
absl_nullable std::unique_ptr<F> ReadFstClass(std::istream& istrm,
                                              const std::string& source) {
  if (!istrm) {
    LOG(ERROR) << "ReadFstClass: Can't open file: " << source;
    return nullptr;
  }
  FstHeader hdr;
  if (!hdr.Read(istrm, source)) return nullptr;
  const FstReadOptions read_options(source, &hdr);
  const auto& arc_type = hdr.ArcType();
  static const auto* reg = FstClassIORegistration<F>::Register::GetRegister();
  const auto reader = reg->GetReader(arc_type);
  if (!reader) {
    LOG(ERROR) << "ReadFstClass: Unknown arc type: " << arc_type;
    return nullptr;
  }
  return reader(istrm, read_options);
}

template <class F>
absl_nullable std::unique_ptr<FstClassImplBase> CreateFstClass(
    absl::string_view arc_type) {
  static const auto* reg = FstClassIORegistration<F>::Register::GetRegister();
  auto creator = reg->GetCreator(arc_type);
  if (!creator) {
    FSTERROR() << "CreateFstClass: Unknown arc type: " << arc_type;
    return nullptr;
  }
  return creator();
}

template <class F>
absl_nullable std::unique_ptr<FstClassImplBase> ConvertFstClass(
    const FstClass& other) {
  static const auto* reg = FstClassIORegistration<F>::Register::GetRegister();
  auto converter = reg->GetConverter(other.ArcType());
  if (!converter) {
    FSTERROR() << "ConvertFstClass: Unknown arc type: " << other.ArcType();
    return nullptr;
  }
  return converter(other);
}

}  // namespace

// FstClass methods.

absl_nullable std::unique_ptr<FstClass> FstClass::Read(
    const std::string& source) {
  if (!source.empty()) {
    file::FileInStream istrm(source, std::ios_base::in | std::ios_base::binary);
    return ReadFstClass<FstClass>(istrm, source);
  } else {
    SetBinaryMode(stdin);
    return ReadFstClass<FstClass>(std::cin, "standard input");
  }
}

absl_nullable std::unique_ptr<FstClass> FstClass::Read(
    std::istream& istrm, const std::string& source) {
  return ReadFstClass<FstClass>(istrm, source);
}

bool FstClass::WeightTypesMatch(const WeightClass& weight,
                                absl::string_view op_name) const {
  if (WeightType() != weight.Type()) {
    FSTERROR() << op_name << ": FST and weight with non-matching weight types: "
               << WeightType() << " and " << weight.Type();
    return false;
  }
  return true;
}

// MutableFstClass methods.

absl_nullable std::unique_ptr<MutableFstClass> MutableFstClass::Read(
    const std::string& source, bool convert) {
  if (convert == false) {
    if (!source.empty()) {
      file::FileInStream in(source, std::ios_base::in | std::ios_base::binary);
      return ReadFstClass<MutableFstClass>(in, source);
    } else {
      SetBinaryMode(stdin);
      return ReadFstClass<MutableFstClass>(std::cin, "standard input");
    }
  } else {  // Converts to VectorFstClass if not mutable.
    std::unique_ptr<FstClass> ifst(FstClass::Read(source));
    if (!ifst) return nullptr;
    if (ifst->Properties(kMutable, false) == kMutable) {
      return absl::WrapUnique(
          absl::down_cast<MutableFstClass*>(ifst.release()));
    } else {
      return std::make_unique<VectorFstClass>(*ifst.release());
    }
  }
}

// VectorFstClass methods.

absl_nullable std::unique_ptr<VectorFstClass> VectorFstClass::Read(
    const std::string& source) {
  if (!source.empty()) {
    file::FileInStream in(source, std::ios_base::in | std::ios_base::binary);
    return ReadFstClass<VectorFstClass>(in, source);
  } else {
    SetBinaryMode(stdin);
    return ReadFstClass<VectorFstClass>(std::cin, "standard input");
  }
}

VectorFstClass::VectorFstClass(absl::string_view arc_type)
    : MutableFstClass(CreateFstClass<VectorFstClass>(arc_type)) {}

VectorFstClass::VectorFstClass(const FstClass& other)
    : MutableFstClass(ConvertFstClass<VectorFstClass>(other)) {}

// Registration.

REGISTER_FST_CLASSES(StdArc);
REGISTER_FST_CLASSES(LogArc);
REGISTER_FST_CLASSES(Log64Arc);

}  // namespace script
}  // namespace fst
