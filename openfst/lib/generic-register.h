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

#ifndef OPENFST_LIB_GENERIC_REGISTER_H_
#define OPENFST_LIB_GENERIC_REGISTER_H_

#include <functional>
#include <map>
#include <string>

#include "absl/base/nullability.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"

// Generic class representing a globally-stored correspondence between
// objects of KeyType and EntryType.
//
// KeyType must:
//
// * be such as can be stored as a key in a std::map<>.
//
// EntryType must be default constructible.
//
// The third template parameter should be the type of a subclass of this class
// (think CRTP). This is to allow GetRegister() to instantiate and return an
// object of the appropriate type.

namespace fst {

namespace internal {

bool LoadSharedObject(const char* absl_nonnull so_filename);

template <class T>
struct KeyLookupReferenceType {
  using type = const T&;
};

template <>
struct KeyLookupReferenceType<std::string> {
  using type = absl::string_view;
};
}  // namespace internal

template <class KeyType, class EntryType, class RegisterType>
class GenericRegister {
 public:
  using Key = KeyType;
  using KeyLookupRef = typename internal::KeyLookupReferenceType<KeyType>::type;
  using Entry = EntryType;

  static RegisterType* GetRegister() {
    static auto reg = new RegisterType;
    return reg;
  }

  void SetEntry(const KeyType& key, const EntryType& entry) {
    absl::MutexLock l(register_lock_);
    register_table_.emplace(key, entry);
  }

  EntryType GetEntry(KeyLookupRef key) const {
    const auto* entry = LookupEntry(key);
    if (entry) {
      return *entry;
    } else {
      return LoadEntryFromSharedObject(key);
    }
  }

  virtual ~GenericRegister() = default;

 protected:
  // Override this to define how to turn a key into an SO filename.
  virtual std::string ConvertKeyToSoFilename(KeyLookupRef key) const = 0;

 private:
  EntryType LoadEntryFromSharedObject(KeyLookupRef key) const {
    const std::string so_filename = ConvertKeyToSoFilename(key);
    if (!internal::LoadSharedObject(so_filename.c_str())) {
      return EntryType();
    }
    // We assume that the DSO constructs a static object in its global scope
    // that does the registration. Thus we need only load it, not call any
    // methods.
    const auto* entry = this->LookupEntry(key);
    if (entry == nullptr) {
      LOG(ERROR) << "GenericRegister::GetEntry: "
                 << "lookup failed in shared object: " << so_filename;
      return EntryType();
    }
    return *entry;
  }

  const EntryType* LookupEntry(KeyLookupRef key) const {
    absl::MutexLock l(register_lock_);
    if (const auto it = register_table_.find(key);
        it != register_table_.end()) {
      return &it->second;
    } else {
      return nullptr;
    }
  }

  mutable absl::Mutex register_lock_;
  std::map<KeyType, EntryType, std::less<>> register_table_;
};

// Generic register-er class capable of creating new register entries in the
// given RegisterType template parameter. This type must define types Key and
// Entry, and have appropriate static GetRegister() and instance SetEntry()
// functions. An easy way to accomplish this is to have RegisterType be the
// type of a subclass of GenericRegister.
template <class RegisterType>
class GenericRegisterer {
 public:
  using Key = typename RegisterType::Key;
  using Entry = typename RegisterType::Entry;

  GenericRegisterer(Key key, Entry entry) {
    RegisterType::GetRegister()->SetEntry(key, entry);
  }
};

}  // namespace fst

#endif  // OPENFST_LIB_GENERIC_REGISTER_H_
