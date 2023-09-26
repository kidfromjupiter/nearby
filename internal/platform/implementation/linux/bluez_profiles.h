#include <sdbus-c++/ProxyInterfaces.h>

#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/profile_manager_client.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/profile_server.h"

namespace nearby {
class CancellationFlag;
namespace linux {
namespace bluez {
class BluezProfile : public sdbus::AdaptorInterfaces<org::bluez::Profile1_adaptor>
{
  BluezProfile(const BluezProfile &) = delete;
  BluezProfile(BluezProfile &&) = delete;
  BluezProfile &operator=(const BluezProfile &) = delete;
  BluezProfile &operator=(BluezProfile &&) = delete;
  BluezProfile(sdbus::IConnection &system_bus, sdbus::ObjectPath profile_object_path) :
      AdaptorInterfaces(system_bus, std::move(profile_object_path))
  {
    registerAdaptor();
  }
  ~BluezProfile() { unregisterAdaptor(); }

  void NewConnection(const sdbus::ObjectPath &device, const sdbus::UnixFd &fd,
                     const std::map<std::string, sdbus::Variant> &fd_properties) override;

};
class BluezProfileManager : public sdbus::ProxyInterfaces<org::bluez::ProfileManager1_proxy>
{

  BluezProfileManager(const BluezProfileManager &) = delete;
  BluezProfileManager(BluezProfileManager &&) = delete;
  BluezProfileManager &operator=(const BluezProfileManager &) = delete;
  BluezProfileManager &operator=(BluezProfileManager &&) = delete;
  BluezProfileManager(sdbus::IConnection &system_bus) : ProxyInterfaces(system_bus, bluez::SERVICE_DEST, "/org/bluez")
  {
    registerProxy();
  }
  ~BluezProfileManager() { unregisterProxy(); }

public:
  bool ProfileRegistered(absl::string_view service_uuid);
  bool Register(std::optional<absl::string_view> service_name, absl::string_view service_uuid);
  bool Register(absl::string_view service_uuid) { return Register(std::nullopt, service_uuid); }
  void Unregister(absl::string_view service_uuid);


private:
  std::map<std::string, std::shared_ptr<BluezProfile>> registered_services_;
};
} // namespace bluez
} // namespace linux
} // namespace nearby
