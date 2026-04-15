#include "internal/platform/implementation/linux/bluez_agent.h"

#include <sdbus-c++/AdaptorInterfaces.h>

#include "internal/platform/implementation/linux/generated/dbus/bluez/agent_server.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/agentmanager_client.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {

void Agent::Release() { LOG(INFO) << "[agent] Release()"; }

std::string Agent::RequestPinCode(const sdbus::ObjectPath& device) {
  LOG(INFO) << "[agent] RequestPinCode(" << device << ")";
  return "0000";
}

void Agent::DisplayPinCode(const sdbus::ObjectPath& device,
                           const std::string& pincode) {
  LOG(INFO) << "[agent] DisplayPinCode(" << device << ", " << pincode << ")";
}

uint32_t Agent::RequestPasskey(const sdbus::ObjectPath& device) {
  LOG(INFO) << "[agent] RequestPasskey(" << device << ")";
  return 123456;
}

void Agent::DisplayPasskey(const sdbus::ObjectPath& device,
                           const uint32_t& passkey,
                           const uint16_t& entered) {
  LOG(INFO) << "[agent] DisplayPasskey(" << device << ", " << passkey
            << ", entered=" << entered << ")";
}

void Agent::RequestConfirmation(const sdbus::ObjectPath& device,
                                const uint32_t& passkey) {
  LOG(INFO) << "[agent] RequestConfirmation(" << device << ", " << passkey
            << ") -> ACCEPT";
}

void Agent::RequestAuthorization(const sdbus::ObjectPath& device) {
  LOG(INFO) << "[agent] RequestAuthorization(" << device << ") -> ACCEPT";
}

void Agent::AuthorizeService(const sdbus::ObjectPath& device,
                             const std::string& uuid) {
  LOG(INFO) << "[agent] AuthorizeService(" << device << ", " << uuid
            << ") -> ACCEPT";
}

void Agent::Cancel() { LOG(INFO) << "[agent] Cancel()"; }

bool AgentManager::AgentRegistered(absl::string_view agent_object_path) {
  registered_agents_mutex_.ReaderLock();
  bool registered = registered_agents_.count(std::string(agent_object_path)) == 1;
  registered_agents_mutex_.ReaderUnlock();
  return registered;
}

bool AgentManager::Register(std::optional<absl::string_view> capability,
                            const sdbus::ObjectPath& agent_object_path) {
  absl::MutexLock l(&registered_agents_mutex_);

  const std::string agent_path_str = std::string(agent_object_path);

  if (registered_agents_.count(agent_path_str) == 1) {
    LOG(WARNING) << __func__ << ": Trying to register agent " << agent_path_str
                 << " which was already registered.";
    return true;
  }

  auto agent = std::make_shared<Agent>(getProxy().getConnection(),
                                       sdbus::ObjectPath(agent_object_path));

  try {
    const std::string cap =
        capability.has_value() ? std::string(*capability) : "NoInputNoOutput";
    RegisterAgent(agent->getObject().getObjectPath(), cap);
    RequestDefaultAgent(agent->getObject().getObjectPath());
  } catch (const sdbus::Error& e) {
    LOG(ERROR) << __func__ << ": Got error '" << e.getName()
               << "' with message '" << e.getMessage()
               << "' while calling RegisterAgent/RequestDefaultAgent on object "
               << getProxy().getObjectPath();
    return false;
  }

  registered_agents_.emplace(agent_path_str, agent);

  LOG(INFO) << __func__ << ": Registered agent instance at path "
            << agent_path_str;

  return true;
}

}  // namespace linux
}  // namespace nearby
