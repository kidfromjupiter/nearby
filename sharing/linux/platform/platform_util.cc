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

#include "sharing/linux/platform/platform_util.h"

#include <ifaddrs.h>
#include <net/if.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <sdbus-c++/Error.h>

#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/logging.h"

namespace nearby::sharing::linux::internal {

std::string GetEnvOrDefault(const char* key, std::string fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  return value;
}

std::string GetHomeDirectory() { return GetEnvOrDefault("HOME", "/tmp"); }

FilePath BuildPathFromBase(const std::string& base,
                           std::initializer_list<std::string> components) {
  std::filesystem::path path(base);
  for (const std::string& component : components) {
    path /= component;
  }
  return FilePath(path.string());
}

std::optional<std::string> GetLanguageCode() {
  const char* lang = std::getenv("LANG");
  if (lang == nullptr || *lang == '\0') {
    return std::string("en");
  }
  std::string value(lang);
  size_t dot = value.find('.');
  if (dot != std::string::npos) {
    value.resize(dot);
  }
  size_t underscore = value.find('_');
  if (underscore != std::string::npos) {
    value.resize(underscore);
  }
  if (value.empty()) {
    return std::string("en");
  }
  return value;
}

bool HasNonLoopbackInterface() {
  struct ifaddrs* interfaces = nullptr;
  if (getifaddrs(&interfaces) != 0) {
    return false;
  }

  bool connected = false;
  for (struct ifaddrs* current = interfaces; current != nullptr;
       current = current->ifa_next) {
    if (current->ifa_name == nullptr || current->ifa_flags == 0) {
      continue;
    }
    if ((current->ifa_flags & IFF_UP) == 0 ||
        (current->ifa_flags & IFF_LOOPBACK) != 0) {
      continue;
    }
    connected = true;
    break;
  }

  freeifaddrs(interfaces);
  return connected;
}

std::shared_ptr<::nearby::linux::BluetoothAdapter>
CreateFastInitBluetoothAdapter() {
  auto system_bus = ::nearby::linux::getSystemBusConnection();
  auto manager = ::nearby::linux::bluez::BluezObjectManager(*system_bus);
  try {
    auto interfaces = manager.GetManagedObjects();
    for (auto& [object, properties] : interfaces) {
      if (properties.count(sdbus::InterfaceName(
              org::bluez::Adapter1_proxy::INTERFACE_NAME)) == 1) {
        LOG(INFO) << __func__ << ": found bluetooth adapter " << object;
        return std::make_shared<::nearby::linux::BluetoothAdapter>(system_bus,
                                                                   object);
      }
    }
  } catch (const sdbus::Error& e) {
    DBUS_LOG_METHOD_CALL_ERROR(&manager, "GetManagedObjects", e);
  }

  LOG(WARNING) << __func__
               << ": couldn't find a bluetooth adapter on this system";
  return nullptr;
}

}  // namespace nearby::sharing::linux::internal
