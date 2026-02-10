// Copyright 2023 Google LLC
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

#ifndef PLATFORM_IMPL_LINUX_AVAHI_H_
#define PLATFORM_IMPL_LINUX_AVAHI_H_

#include <memory>

#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/ProxyInterfaces.h>

#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/implementation/linux/generated/dbus/avahi/entrygroup_client.h"
#include "internal/platform/implementation/linux/generated/dbus/avahi/server2_client.h"
#include "internal/platform/implementation/linux/generated/dbus/avahi/servicebrowser_client.h"
#include "internal/platform/implementation/wifi_lan.h"

namespace nearby {
namespace linux {
namespace avahi {
namespace {
api::WifiLanMedium::DiscoveredServiceCallback MakeNoopDiscoveredServiceCallback() {
  api::WifiLanMedium::DiscoveredServiceCallback cb{};

  cb.service_discovered_cb = [](NsdServiceInfo) {};
  cb.service_lost_cb       = [](NsdServiceInfo) {};

  return cb;
}
} // namespace
class Server final
    : public sdbus::ProxyInterfaces<org::freedesktop::Avahi::Server2_proxy> {
 public:
  Server(sdbus::IConnection &system_bus,
  api::WifiLanMedium::DiscoveredServiceCallback callback)
      : ProxyInterfaces(system_bus, "org.freedesktop.Avahi", "/"),
        discovery_cb_(std::move(callback))
  {
    registerProxy();
  }
  Server(sdbus::IConnection& system_bus)
    : Server(system_bus, MakeNoopDiscoveredServiceCallback()) {}
  ~Server() { unregisterProxy(); }
  void SetDiscoveryCallback(
      api::WifiLanMedium::DiscoveredServiceCallback callback) {
    discovery_cb_ = std::move(callback);
  }
  api::WifiLanMedium::DiscoveredServiceCallback discovery_cb_;
 protected:
  void onStateChanged(const int32_t &state, const std::string &error) override {
  }
  void onResolveServiceReply(const int32_t& interface,
    const int32_t& protocol, const std::string& name,
    const std::string& type, const std::string& domain,
    const std::string& host, const int32_t& aprotocol,
    const std::string& address, const uint16_t& port,
    const std::vector<std::vector<uint8_t>>& txt, const uint32_t& flags,
    const sdbus::Error* error) override;
};

class EntryGroup final
    : public sdbus::ProxyInterfaces<org::freedesktop::Avahi::EntryGroup_proxy> {
 public:
  EntryGroup(sdbus::IConnection &system_bus,
             const sdbus::ObjectPath &entry_group_object_path)
      : ProxyInterfaces(system_bus, "org.freedesktop.Avahi",
                        entry_group_object_path) {
    registerProxy();
  }
  ~EntryGroup() {
    LOG(INFO) << __func__ << ": Freeing entry group "
                         << getObjectPath();

    try {
      Free();
    } catch (const sdbus::Error &e) {
      DBUS_LOG_METHOD_CALL_ERROR(this, "Free", e);
    }

    unregisterProxy();
  }

 protected:
  void onStateChanged(const int32_t &state, const std::string &error) override {
  }
};

class ServiceBrowser final
    : public sdbus::ProxyInterfaces<
          org::freedesktop::Avahi::ServiceBrowser_proxy> {
 public:
  ServiceBrowser(sdbus::IConnection &system_bus,
                 const sdbus::ObjectPath &service_browser_object_path,
                 std::shared_ptr<Server> avahi_server)
      : ProxyInterfaces(system_bus, "org.freedesktop.Avahi",
                        service_browser_object_path),
        server_(avahi_server) {
    registerProxy();
  }
  ~ServiceBrowser() {
    LOG(INFO) << __func__ << ": Freeing service browser "
                         << getObjectPath();

    try {
      Free();
    } catch (const sdbus::Error &e) {
      DBUS_LOG_METHOD_CALL_ERROR(this, "Free", e);
    }
    unregisterProxy();
  }

 protected:
  void onItemNew(const int32_t &interface, const int32_t &protocol,
                 const std::string &name, const std::string &type,
                 const std::string &domain, const uint32_t &flags) override;
  void onItemRemove(const int32_t &interface, const int32_t &protocol,
                    const std::string &name, const std::string &type,
                    const std::string &domain, const uint32_t &flags) override;
  void onFailure(const std::string &error) override;
  void onAllForNow() override;
  void onCacheExhausted() override;

 private:
  enum LookupResultFlags {
    kAvahiLookupResultFlagCached = 1,
    kAvahiLookupResultFlagWideArea = 2,
    kAvahiLookupResultFlagMulticast = 4,
    kAvahiLookupResultLocal = 8,
    kAvahiLookupResultOurOwn = 16,
    kAvahiLookupResultStatic = 32,
  };

  std::shared_ptr<Server> server_;
};
}  // namespace avahi
}  // namespace linux
}  // namespace nearby

#endif
