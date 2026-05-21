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

#include "openfst/static/command_dispatcher.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/flags/usage.h"
#include "absl/flags/reflection.h"
#include "openfst/compat/init.h"
#include "openfst/compat/file_path.h"
#include "absl/base/nullability.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"

#if defined(_MSC_VER)  // Windows MSVC compiler.
#include <stdlib.h>    // ::malloc and friends.
#include <string.h>    // For ::strnlen.

namespace {

char* strndup(const char* s, size_t n) {
  if (!s) return nullptr;

  const size_t len = ::strnlen(s, n);
  char* copy = (char*)::malloc(len + 1);
  if (copy) {
    ::memcpy(copy, s, len);
    copy[len] = '\0';
  }
  return copy;
}

}  // namespace

#endif  // _MSC_VER

namespace fst {

CommandDispatcher::CommandDispatcher(const std::string& main_command_name,
                                     const std::string& flag_restrict_path,
                                     std::vector<Command> commands)
    : main_command_name_(main_command_name),
      flag_restrict_path_(flag_restrict_path),
      commands_(std::move(commands)) {
  for (const auto& cmd : commands_) {
    // GetUsage depends on this.
    CHECK(absl::StartsWith(cmd.name, main_command_name_))  // Crash OK.
        << cmd.name << " must start with " << main_command_name_;
  }
}

std::string CommandDispatcher::GetUsage() const {
  std::string usage = absl::StrCat(main_command_name_,
                                   " <command> [flags]\n\n"
                                   "where <command> is one of\n");
  for (absl::string_view name : GetCommandNames()) {
    absl::StrAppend(&usage, "  ", name, "\n");
  }
  absl::StrAppend(&usage, "\nUse ", main_command_name_,
                  " commands for a list of all command names");
  absl::StrAppend(&usage, "\nUse ", main_command_name_,
                  " help <command> for a list of flags.");
  return usage;
}

std::vector<absl::string_view> CommandDispatcher::GetCommandNames() const {
  std::vector<absl::string_view> command_names;
  command_names.reserve(commands_.size());
  for (const auto& cmd : commands_) {
    command_names.push_back(absl::StripPrefix(cmd.name, main_command_name_));
  }
  return command_names;
}

const Command* absl_nullable CommandDispatcher::FindCommand(
    absl::string_view command) {
  auto iter =
      std::find_if(commands_.begin(), commands_.end(),
                   [command](const Command& c) { return c.name == command; });
  return iter != commands_.end() ? &*iter : nullptr;
}

int CommandDispatcher::RunCommand(const std::string& command, int argc,
                                  char** argv) {
  const auto* tool = FindCommand(command);
  if (tool == nullptr) {
    fst::InitOpenFst(GetUsage().c_str(), &argc, &argv, true);
    LOG(FATAL) << "Unknown command: '" << command << "'";  // Crash OK.
    return 1;
  } else {
    return tool->main(argc, argv);
  }
}

void CommandDispatcher::ShowFlags(const std::string& command) {
  const auto* tool = FindCommand(main_command_name_ + command);
  if (tool == nullptr) {
    std::cout << "Unknown command: " << command << '\n';
  } else {
    for (const auto& flag : tool->flags) {
      std::cout << absl::FindCommandLineFlag(flag)->Help();
    }
  }
}

int CommandDispatcher::Main(int argc, char** argv) {
  std::string cmd(Basename(argv[0]));
  if (cmd == main_command_name_) {
    const auto command_index =
        ParseCommandLineNonHelpFlags(&argc, &argv, false);
    if (command_index >= argc || (command_index + 1 == argc &&
                                  std::string(argv[command_index]) == "help")) {
      // No command or only 'help'
      fst::InitOpenFst(GetUsage().c_str(), &argc, &argv, true);
      LOG(INFO) << absl::ProgramUsageMessage();
      return 1;
    } else if ((command_index + 1 == argc &&
                std::string(argv[command_index]) == "commands")) {
      // Only 'commands'
      for (absl::string_view command_name : GetCommandNames()) {
        std::cout << command_name << "\n";
      }
      return 0;
    }

    if (command_index + 1 < argc &&
        std::string(argv[command_index]) == "help") {
      // help <command>
      cmd = argv[command_index + 1];
      argc = 1;
      fst::InitOpenFst(argv[0], &argc, &argv, false);
      ShowFlags(cmd);
      return 0;
    }

    cmd = main_command_name_ + argv[command_index];
    // remove command and shift arguments after the command back by one
    std::copy(argv + command_index + 1, argv + argc, argv + command_index);
    --argc;
    argv[0] = strndup(cmd.c_str(), cmd.size());
    // C++ standard requires argv[argc] is null.
    argv[argc] = nullptr;
  }
  return RunCommand(cmd, argc, argv);
}

}  // namespace fst
