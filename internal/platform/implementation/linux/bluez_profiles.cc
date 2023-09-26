#include "internal/platform/implementation/linux/bluez_profiles.h"

#include "internal/platform/logging.h"

namespace nearby{
namespace linux{
namespace bluez{
void BluezProfile::NewConnection(const sdbus::ObjectPath& device, const sdbus::UnixFd& fd, const std::map<std::string, sdbus::Variant>& fd_properties)
{
  NEARBY_VLOG(1) << __func__ << ": New connection to device " << device;
}


}
}
}
