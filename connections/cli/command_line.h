// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CONNECTIONS_CLI_COMMAND_LINE_H_
#define CONNECTIONS_CLI_COMMAND_LINE_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nearby::connections::cli {

struct CommandLine {
  std::string command;
  std::vector<std::string> positionals;
  std::unordered_map<std::string, std::string> options;
};

// Splits a shell-like command line. Supports single/double quotes and backslash
// escapes, but deliberately performs no expansion or substitution.
bool Tokenize(std::string_view input, std::vector<std::string>* tokens,
              std::string* error);

// Parses: command [positional ...] [--key=value] [--key value] [--flag]
// [--no-flag]. A bare flag has value "true"; --no-x has value "false".
bool ParseCommand(std::string_view input, CommandLine* command,
                  std::string* error);
bool ParseTokens(const std::vector<std::string>& tokens, CommandLine* command,
                 std::string* error);

bool RejectUnknownOptions(const CommandLine& command,
                          const std::unordered_set<std::string>& allowed,
                          std::string* error);
std::optional<std::string> GetString(const CommandLine& command,
                                     std::string_view name);
bool GetBool(const CommandLine& command, std::string_view name,
             bool default_value, bool* value, std::string* error);
bool GetInt64(const CommandLine& command, std::string_view name,
              std::int64_t default_value, std::int64_t* value,
              std::string* error);
std::vector<std::string> SplitCommaList(std::string_view input);

}  // namespace nearby::connections::cli

#endif  // CONNECTIONS_CLI_COMMAND_LINE_H_
