// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.

#include "connections/cli/command_line.h"

#include <charconv>
#include <cctype>
#include <string>
#include <utility>

namespace nearby::connections::cli {
namespace {

std::string Trim(std::string_view value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1);
  }
  return std::string(value);
}

}  // namespace

bool Tokenize(std::string_view input, std::vector<std::string>* tokens,
              std::string* error) {
  tokens->clear();
  std::string token;
  bool token_started = false;
  bool escaped = false;
  char quote = '\0';
  for (char ch : input) {
    if (escaped) {
      token.push_back(ch);
      token_started = true;
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      token_started = true;
      continue;
    }
    if (quote != '\0') {
      if (ch == quote) {
        quote = '\0';
      } else {
        token.push_back(ch);
      }
      token_started = true;
      continue;
    }
    if (ch == '\'' || ch == '"') {
      quote = ch;
      token_started = true;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (token_started) {
        tokens->push_back(std::move(token));
        token.clear();
        token_started = false;
      }
      continue;
    }
    token.push_back(ch);
    token_started = true;
  }
  if (escaped) {
    *error = "command ends with an incomplete backslash escape";
    return false;
  }
  if (quote != '\0') {
    *error = "unterminated quoted string";
    return false;
  }
  if (token_started) tokens->push_back(std::move(token));
  return true;
}

bool ParseCommand(std::string_view input, CommandLine* command,
                  std::string* error) {
  std::vector<std::string> tokens;
  return Tokenize(input, &tokens, error) && ParseTokens(tokens, command, error);
}

bool ParseTokens(const std::vector<std::string>& tokens, CommandLine* command,
                 std::string* error) {
  *command = {};
  if (tokens.empty()) return true;
  command->command = tokens.front();
  for (size_t i = 1; i < tokens.size(); ++i) {
    const std::string& token = tokens[i];
    if (token.size() < 3 || token.substr(0, 2) != "--") {
      command->positionals.push_back(token);
      continue;
    }
    std::string option = token.substr(2);
    std::string value;
    size_t equals = option.find('=');
    if (equals != std::string::npos) {
      value = option.substr(equals + 1);
      option.resize(equals);
    } else if (option.rfind("no-", 0) == 0) {
      option.erase(0, 3);
      value = "false";
    } else if (i + 1 < tokens.size() && tokens[i + 1].rfind("--", 0) != 0) {
      value = tokens[++i];
    } else {
      value = "true";
    }
    if (option.empty()) {
      *error = "empty option name";
      return false;
    }
    if (!command->options.emplace(option, value).second) {
      *error = "option --" + option + " was specified more than once";
      return false;
    }
  }
  return true;
}

bool RejectUnknownOptions(const CommandLine& command,
                          const std::unordered_set<std::string>& allowed,
                          std::string* error) {
  for (const auto& [name, value] : command.options) {
    if (!allowed.contains(name)) {
      *error = "unknown option --" + name + " for " + command.command;
      return false;
    }
  }
  return true;
}

std::optional<std::string> GetString(const CommandLine& command,
                                     std::string_view name) {
  auto it = command.options.find(std::string(name));
  if (it == command.options.end()) return std::nullopt;
  return it->second;
}

bool GetBool(const CommandLine& command, std::string_view name,
             bool default_value, bool* value, std::string* error) {
  auto text = GetString(command, name);
  if (!text.has_value()) {
    *value = default_value;
    return true;
  }
  if (*text == "true" || *text == "1" || *text == "yes" || *text == "on") {
    *value = true;
    return true;
  }
  if (*text == "false" || *text == "0" || *text == "no" || *text == "off") {
    *value = false;
    return true;
  }
  *error =
      "--" + std::string(name) + " expects true or false, got '" + *text + "'";
  return false;
}

bool GetInt64(const CommandLine& command, std::string_view name,
              std::int64_t default_value, std::int64_t* value,
              std::string* error) {
  auto text = GetString(command, name);
  if (!text.has_value()) {
    *value = default_value;
    return true;
  }
  std::int64_t parsed = 0;
  const char* begin = text->data();
  const char* end = begin + text->size();
  auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc() || result.ptr != end) {
    *error =
        "--" + std::string(name) + " expects an integer, got '" + *text + "'";
    return false;
  }
  *value = parsed;
  return true;
}

std::vector<std::string> SplitCommaList(std::string_view input) {
  std::vector<std::string> result;
  while (true) {
    size_t comma = input.find(',');
    result.push_back(Trim(input.substr(0, comma)));
    if (comma == std::string_view::npos) break;
    input.remove_prefix(comma + 1);
  }
  return result;
}

}  // namespace nearby::connections::cli
