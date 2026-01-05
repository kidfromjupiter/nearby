#include "bluez_agent.h"

#include <sdbus-c++/AdaptorInterfaces.h>

#include "generated/dbus/bluez/agentmanager_client.h"
#include "generated/dbus/bluez/agent_server.h"
#include "internal/platform/logging.h"
#include "internal/platform/implementation/linux/bluez.h"

namespace nearby
{
  namespace linux
  {
    // org.bluez.Agent1 methods (BlueZ will call these)
    void Agent::Release()
    {
      LOG(INFO) << "[agent] Release()\n";
    }

    std::string Agent::RequestPinCode(const sdbus::ObjectPath& device)
    {
      LOG(INFO) << "[agent] RequestPinCode(" << device << ")\n";
      // If you truly have no UI, you may want to reject instead of guessing:
      // throw sdbus::Error("org.bluez.Error.Rejected", "No PIN entry available");
      return "0000";
    }

    void Agent::DisplayPinCode(const sdbus::ObjectPath& device, const std::string& pincode)
    {
      LOG(INFO) << "[agent] DisplayPinCode(" << device << ", " << pincode << ")\n";
    }

    uint32_t Agent::RequestPasskey(const sdbus::ObjectPath& device)
    {
      LOG(INFO) << "[agent] RequestPasskey(" << device << ")\n";
      // Same idea: reject if you cannot securely provide a passkey
      // throw sdbus::Error("org.bluez.Error.Rejected", "No passkey entry available");
      return 123456;
    }

    void Agent::DisplayPasskey(const sdbus::ObjectPath& device, const uint32_t& passkey,
                               const uint16_t& entered)
    {
      LOG(INFO) << "[agent] DisplayPasskey(" << device << ", " << passkey
                  << ", entered=" << entered << ")\n";
    }

    void Agent::RequestConfirmation(const sdbus::ObjectPath& device, const uint32_t& passkey)
    {
      LOG(INFO) << "[agent] RequestConfirmation(" << device << ", " << passkey << ") -> ACCEPT\n";
      // Accept by returning normally.
      // Reject with:
      // throw sdbus::Error("org.bluez.Error.Rejected", "User rejected confirmation");
    }

    void Agent::RequestAuthorization(const sdbus::ObjectPath& device)
    {
      LOG(INFO) << "[agent] RequestAuthorization(" << device << ") -> ACCEPT\n";
    }

    void Agent::AuthorizeService(const sdbus::ObjectPath& device, const std::string& uuid)
    {
      LOG(INFO) << "[agent] AuthorizeService(" << device << ", " << uuid << ") -> ACCEPT\n";
      // You can enforce allowlist here if you want.
    }

    void Agent::Cancel()
    {
      LOG(INFO) << "[agent] Cancel()\n";
    }


    bool AgentManager::AgentRegistered(absl::string_view agent_object_path) {
      registered_agents_mutex_.ReaderLock();
      bool registered =
          registered_agents_.count(std::string(agent_object_path)) == 1;
      registered_agents_mutex_.ReaderUnlock();
      return registered;
    }
    bool AgentManager::Register(std::optional<absl::string_view> capability,
                                const sdbus::ObjectPath& agent_object_path) {
      absl::MutexLock l(&registered_agents_mutex_);

      const std::string agent_path_str = std::string(agent_object_path);

      if (registered_agents_.count(agent_path_str) == 1) {
        LOG(WARNING) << __func__ << ": Trying to register agent "
                     << agent_path_str << " which was already registered.";
        return true;
      }

      // Keep the agent alive by storing it.
      auto agent = std::make_shared<Agent>(
          getProxy().getConnection(),
          sdbus::ObjectPath(agent_object_path));

      try {
        // BlueZ capability examples: "NoInputNoOutput", "DisplayYesNo", "KeyboardOnly", ...
        const std::string cap =
            capability.has_value() ? std::string(*capability) : "NoInputNoOutput";

        // These are methods on org.bluez.AgentManager1_proxy (generated)
        RegisterAgent(agent->getObjectPath(), cap);
        RequestDefaultAgent(agent->getObjectPath());

      } catch (const sdbus::Error& e) {
        BLUEZ_LOG_METHOD_CALL_ERROR(&getProxy(), "RegisterAgent/RequestDefaultAgent", e);
        return false;
      }

      registered_agents_.emplace(agent_path_str, agent);

      LOG(INFO) << __func__ << ": Registered agent instance at path "
                << agent_path_str;

      return true;
    }
  };
}
