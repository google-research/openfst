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

#ifndef OPENFST_STATIC_COMMAND_DISPATCHER_H_
#define OPENFST_STATIC_COMMAND_DISPATCHER_H_

// Command dispatcher to call different main functions based on command-line
// argument.  Useful for implementing tools like "fst", which can run any
// fst operation.  For example, "fst info in.fst" will run fstinfo_main
// if configured with a Command{"info", &fstinfo_main, ...}.

#include <functional>
#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"

namespace fst {

struct Command {
  const std::string name;
  const std::function<int(int, char**)> main;
  const std::vector<std::string> flags;
};

class CommandDispatcher {
 public:
  CommandDispatcher(const std::string& main_command_name,
                    const std::string& flag_restrict_path,
                    std::vector<Command> commands);

  int Main(int argc, char** argv);

 private:
  const std::string main_command_name_;
  const std::string flag_restrict_path_;
  const std::vector<Command> commands_;

  std::string GetUsage() const;
  std::vector<absl::string_view> GetCommandNames() const;
  const Command* absl_nullable FindCommand(absl::string_view command);
  int RunCommand(const std::string& command, int argc, char** argv);
  void ShowFlags(const std::string& command);
};

}  // namespace fst

#endif  // OPENFST_STATIC_COMMAND_DISPATCHER_H_
