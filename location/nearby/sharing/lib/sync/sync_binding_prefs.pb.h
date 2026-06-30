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

#ifndef LOCATION_NEARBY_SHARING_LIB_SYNC_SYNC_BINDING_PREFS_PB_H_
#define LOCATION_NEARBY_SHARING_LIB_SYNC_SYNC_BINDING_PREFS_PB_H_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nearby::sharing::sync {

class SyncBinding {
 public:
  enum SourceDeviceType {
    SOURCE_DEVICE_TYPE_UNKNOWN = 0,
    SOURCE_DEVICE_TYPE_PHONE = 1,
    SOURCE_DEVICE_TYPE_TABLET = 2,
    SOURCE_DEVICE_TYPE_LAPTOP = 3,
    SOURCE_DEVICE_TYPE_CAR = 4,
    SOURCE_DEVICE_TYPE_FOLDABLE = 5,
    SOURCE_DEVICE_TYPE_XR = 6,
  };

  void set_binding_id(std::string binding_id) {
    binding_id_ = std::move(binding_id);
  }
  void set_binding_id(std::string_view binding_id) {
    binding_id_ = std::string(binding_id);
  }
  const std::string& binding_id() const { return binding_id_; }
  void set_source_name(std::string source_name) {
    source_name_ = std::move(source_name);
  }
  void set_source_name(std::string_view source_name) {
    source_name_ = std::string(source_name);
  }
  const std::string& source_name() const { return source_name_; }
  void set_destination_directory(std::string destination_directory) {
    destination_directory_ = std::move(destination_directory);
  }
  void set_destination_directory(std::string_view destination_directory) {
    destination_directory_ = std::string(destination_directory);
  }
  const std::string& destination_directory() const {
    return destination_directory_;
  }
  void set_source_device_type(SourceDeviceType source_device_type) {
    source_device_type_ = source_device_type;
  }
  SourceDeviceType source_device_type() const { return source_device_type_; }

 private:
  std::string binding_id_;
  std::string source_name_;
  std::string destination_directory_;
  SourceDeviceType source_device_type_ = SOURCE_DEVICE_TYPE_UNKNOWN;
};

class SyncBindingPrefs {
 public:
  static const SyncBindingPrefs& default_instance() {
    static const SyncBindingPrefs* prefs = new SyncBindingPrefs();
    return *prefs;
  }

  SyncBinding* add_sync_bindings() {
    sync_bindings_.emplace_back();
    return &sync_bindings_.back();
  }
  const std::vector<SyncBinding>& sync_bindings() const {
    return sync_bindings_;
  }
  const SyncBinding& sync_bindings(int index) const {
    return sync_bindings_.at(index);
  }
  int sync_bindings_size() const {
    return static_cast<int>(sync_bindings_.size());
  }

  bool SerializeToString(std::string* output) const {
    if (output == nullptr) {
      return false;
    }
    output->clear();
    for (const SyncBinding& binding : sync_bindings_) {
      output->append(Escape(binding.binding_id()));
      output->push_back('\t');
      output->append(Escape(binding.source_name()));
      output->push_back('\t');
      output->append(Escape(binding.destination_directory()));
      output->push_back('\t');
      output->append(std::to_string(static_cast<int>(binding.source_device_type())));
      output->push_back('\n');
    }
    return true;
  }

  bool ParseFromString(const std::string& input) {
    sync_bindings_.clear();
    size_t start = 0;
    while (start < input.size()) {
      size_t end = input.find('\n', start);
      std::string line = input.substr(start, end == std::string::npos
                                                 ? std::string::npos
                                                 : end - start);
      if (!line.empty()) {
        std::vector<std::string> fields;
        size_t field_start = 0;
        while (fields.size() < 4) {
          size_t field_end = line.find('\t', field_start);
          fields.push_back(Unescape(line.substr(
              field_start, field_end == std::string::npos
                               ? std::string::npos
                               : field_end - field_start)));
          if (field_end == std::string::npos) {
            break;
          }
          field_start = field_end + 1;
        }
        if (fields.size() != 4) {
          return false;
        }
        SyncBinding binding;
        binding.set_binding_id(fields[0]);
        binding.set_source_name(fields[1]);
        binding.set_destination_directory(fields[2]);
        binding.set_source_device_type(
            static_cast<SyncBinding::SourceDeviceType>(std::stoi(fields[3])));
        sync_bindings_.push_back(std::move(binding));
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + 1;
    }
    return true;
  }

 private:
  static std::string Escape(const std::string& value) {
    std::string escaped;
    for (char c : value) {
      if (c == '\\' || c == '\t' || c == '\n') {
        escaped.push_back('\\');
      }
      if (c == '\t') {
        escaped.push_back('t');
      } else if (c == '\n') {
        escaped.push_back('n');
      } else {
        escaped.push_back(c);
      }
    }
    return escaped;
  }

  static std::string Unescape(const std::string& value) {
    std::string unescaped;
    bool escaping = false;
    for (char c : value) {
      if (escaping) {
        if (c == 't') {
          unescaped.push_back('\t');
        } else if (c == 'n') {
          unescaped.push_back('\n');
        } else {
          unescaped.push_back(c);
        }
        escaping = false;
      } else if (c == '\\') {
        escaping = true;
      } else {
        unescaped.push_back(c);
      }
    }
    return unescaped;
  }

  std::vector<SyncBinding> sync_bindings_;
};

class SyncConfigPrefs {
 public:
  bool SerializeToString(std::string* output) const {
    if (output == nullptr) {
      return false;
    }
    output->clear();
    return true;
  }

  bool ParseFromString(const std::string& input) {
    return input.empty();
  }
};

}  // namespace nearby::sharing::sync

#endif  // LOCATION_NEARBY_SHARING_LIB_SYNC_SYNC_BINDING_PREFS_PB_H_
