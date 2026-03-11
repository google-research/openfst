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

#include "openfst/lib/symbol-table.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/str_cat.h"
#include "benchmark/benchmark.h"

ABSL_FLAG(std::string, symbol_table, "",
          "If non-empty, text symbol table to use for benchmarks.");

namespace fst {
namespace {

std::vector<std::string> MakeIntSyms(const int num_elements) {
  std::vector<std::string> syms(num_elements);
  for (int si = 0; si < num_elements; ++si) {
    syms[si] = absl::StrCat(si);
  }
  return syms;
}

void BM_SymbolTableAdd(benchmark::State& state) {
  const int elements = state.range(0);
  const std::vector<std::string> test_syms = MakeIntSyms(elements);

  for (auto _ : state) {
    SymbolTable symbols;
    for (const auto& sym : test_syms) {
      symbols.AddSymbol(sym, state.iterations() + 1);
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_SymbolTableAdd)->Range(8, 1 << 17);

void BM_AddSymbolDense(benchmark::State& state) {
  const int elements = state.range(0);
  const std::vector<std::string> test_syms = MakeIntSyms(elements);

  for (auto _ : state) {
    SymbolTable symbols;
    for (const auto& sym : test_syms) {
      symbols.AddSymbol(sym);
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_AddSymbolDense)->Range(8, 1 << 17);

void BM_SymbolTableCopy(benchmark::State& state) {
  const int elements = state.range(0);
  const std::vector<std::string> test_syms = MakeIntSyms(elements);

  SymbolTable symbols;
  for (const auto& sym : test_syms) {
    symbols.AddSymbol(sym);
  }

  for (auto _ : state) {
    std::unique_ptr<SymbolTable> copy(symbols.Copy());
    copy->AddSymbol("copy-on-write");
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SymbolTableCopy)->Range(8, 1 << 17);

void BM_SymbolTableFind(benchmark::State& state) {
  const int elements = state.range(0);
  const std::vector<std::string> test_syms = MakeIntSyms(elements);

  SymbolTable symbols;
  for (const auto& sym : test_syms) {
    symbols.AddSymbol(sym);
  }

  for (auto _ : state) {
    for (const auto& sym : test_syms) {
      symbols.Find(sym);
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_SymbolTableFind)->Range(8, 1 << 17);

void BM_SymbolTableLoadFind(benchmark::State& state) {
  if (absl::GetFlag(FLAGS_symbol_table).empty()) return;
  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(absl::GetFlag(FLAGS_symbol_table)));
  QCHECK(symbols != nullptr)
      << "Invalid symbol table: " << absl::GetFlag(FLAGS_symbol_table);

  std::vector<std::string> test_syms;
  test_syms.reserve(symbols->AvailableKey());
  for (const auto& kv : *symbols) {
    test_syms.push_back(kv.Symbol());
  }

  for (auto _ : state) {
    for (const auto& sym : test_syms) {
      symbols->Find(sym);
    }
  }
  state.SetItemsProcessed(state.iterations() * test_syms.size());
}
BENCHMARK(BM_SymbolTableLoadFind);

void BM_SymbolTableLookup(benchmark::State& state) {
  const int elements = state.range(0);
  const std::vector<std::string> test_syms = MakeIntSyms(elements);

  SymbolTable symbols;
  for (const auto& sym : test_syms) {
    symbols.AddSymbol(sym);
  }

  for (auto _ : state) {
    for (int label = 0; label < elements; ++label) {
      symbols.Find(label);
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_SymbolTableLookup)->Range(8, 1 << 17);

void BM_SymbolTableLoadLookup(benchmark::State& state) {
  if (absl::GetFlag(FLAGS_symbol_table).empty()) return;
  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(absl::GetFlag(FLAGS_symbol_table)));
  QCHECK(symbols != nullptr)
      << "Invalid symbol table: " << absl::GetFlag(FLAGS_symbol_table);

  const int num_syms = symbols->AvailableKey();
  for (auto _ : state) {
    for (int label = 0; label < num_syms; ++label) {
      symbols->Find(label);
    }
  }
  state.SetItemsProcessed(state.iterations() * num_syms);
}
BENCHMARK(BM_SymbolTableLoadLookup);

// A somewhat (very?) artificial test that creates a bogus symbol table and then
// measures access time for the various checksums.  Because calculation only
// happens the first time, this test essentially measures the mutex/lock time
// for the call.
void BM_LabeledCheckSum(benchmark::State& state) {
  static const SymbolTable* const symtab = [] {
    auto* symtab = new SymbolTable();
    for (char c = 'A'; c <= 'Z'; ++c) {
      symtab->AddSymbol(std::string(1, c), static_cast<int64_t>(c));
    }
    for (char c = 'a'; c <= 'z'; ++c) {
      symtab->AddSymbol(std::string(1, c), static_cast<int64_t>(c));
    }
    return symtab;
  }();
  for (auto s : state) {
    const std::string& checksum = symtab->LabeledCheckSum();
    benchmark::DoNotOptimize(checksum.data());
  }
}
BENCHMARK(BM_LabeledCheckSum)->ThreadRange(1, 64);

}  // namespace
}  // namespace fst
