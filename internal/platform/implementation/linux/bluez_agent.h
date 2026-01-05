#pragma once

#include <cstdint>
#include <string>
#include <utility>   // std::move

#include <sdbus-c++/sdbus-c++.h>

#include "generated/dbus/bluez/agentmanager_client.h"
#include "internal/platform/logging.h"
#include "generated/dbus/bluez/agent_server.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/implementation/linux/bluez.h"


namespace nearby::linux
{
  class Agent final : public sdbus::AdaptorInterfaces<org::bluez::Agent1_adaptor>
  {
  public:
    Agent(const Agent&) = delete;
    Agent(Agent&&) = delete;
    Agent& operator=(const Agent&) = delete;
    Agent& operator=(Agent&&) = delete;

    Agent(sdbus::IConnection& system_bus, sdbus::ObjectPath path)
      : AdaptorInterfaces(system_bus, std::move(path))
    {
      registerAdaptor();
      LOG(INFO) << "Created new Agent at path: " << getObjectPath();
    }

    ~Agent()
    {
      unregisterAdaptor();
    }

  private:
    // org.bluez.Agent1 methods (BlueZ will call these)
    void Release() override;

    std::string RequestPinCode(const sdbus::ObjectPath& device) override;
    void DisplayPinCode(const sdbus::ObjectPath& device,
                        const std::string& pincode) override;

    uint32_t RequestPasskey(const sdbus::ObjectPath& device) override;
    void DisplayPasskey(const sdbus::ObjectPath& device,
                        const uint32_t& passkey,
                        const uint16_t& entered) override;

    void RequestConfirmation(const sdbus::ObjectPath& device,
                             const uint32_t& passkey) override;

    void RequestAuthorization(const sdbus::ObjectPath& device) override;

    void AuthorizeService(const sdbus::ObjectPath& device,
                          const std::string& uuid) override;

    void Cancel() override;
  };

  class AgentManager final
    : public sdbus::ProxyInterfaces<org::bluez::AgentManager1_proxy>
  {
  public:
    AgentManager(const AgentManager&) = delete;
    AgentManager(AgentManager&&) = delete;
    AgentManager& operator=(const AgentManager&) = delete;
    AgentManager& operator=(AgentManager&&) = delete;

    explicit AgentManager(sdbus::IConnection& system_bus)
      : ProxyInterfaces(system_bus, bluez::SERVICE_DEST, "/org/bluez")
    {
      registerProxy();
    }

    ~AgentManager() { unregisterProxy(); }

    bool Register(std::optional<absl::string_view> capability,
                  const sdbus::ObjectPath& agent_object_path);

    // In AgentManager class declaration:
    bool AgentRegistered(absl::string_view agent_object_path);

  private:
    absl::Mutex registered_agents_mutex_;
    std::map<std::string, std::shared_ptr<Agent>> registered_agents_
    ABSL_GUARDED_BY(registered_agents_mutex_);
  };
} // namespace nearby::linux
