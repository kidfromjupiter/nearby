
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__networkmanager_client_glue_h__proxy__H__
#define __sdbuscpp__networkmanager_client_glue_h__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {

class NetworkManager_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager";

protected:
    NetworkManager_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
        proxy_.uponSignal("CheckPermissions").onInterface(INTERFACE_NAME).call([this](){ this->onCheckPermissions(); });
        proxy_.uponSignal("StateChanged").onInterface(INTERFACE_NAME).call([this](const uint32_t& state){ this->onStateChanged(state); });
        proxy_.uponSignal("DeviceAdded").onInterface(INTERFACE_NAME).call([this](const sdbus::ObjectPath& device_path){ this->onDeviceAdded(device_path); });
        proxy_.uponSignal("DeviceRemoved").onInterface(INTERFACE_NAME).call([this](const sdbus::ObjectPath& device_path){ this->onDeviceRemoved(device_path); });
    }

    ~NetworkManager_proxy() = default;

    virtual void onCheckPermissions() = 0;
    virtual void onStateChanged(const uint32_t& state) = 0;
    virtual void onDeviceAdded(const sdbus::ObjectPath& device_path) = 0;
    virtual void onDeviceRemoved(const sdbus::ObjectPath& device_path) = 0;

public:
    void Reload(const uint32_t& flags)
    {
        proxy_.callMethod("Reload").onInterface(INTERFACE_NAME).withArguments(flags);
    }

    std::vector<sdbus::ObjectPath> GetDevices()
    {
        std::vector<sdbus::ObjectPath> result;
        proxy_.callMethod("GetDevices").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    std::vector<sdbus::ObjectPath> GetAllDevices()
    {
        std::vector<sdbus::ObjectPath> result;
        proxy_.callMethod("GetAllDevices").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    sdbus::ObjectPath GetDeviceByIpIface(const std::string& iface)
    {
        sdbus::ObjectPath result;
        proxy_.callMethod("GetDeviceByIpIface").onInterface(INTERFACE_NAME).withArguments(iface).storeResultsTo(result);
        return result;
    }

    sdbus::ObjectPath ActivateConnection(const sdbus::ObjectPath& connection, const sdbus::ObjectPath& device, const sdbus::ObjectPath& specific_object)
    {
        sdbus::ObjectPath result;
        proxy_.callMethod("ActivateConnection").onInterface(INTERFACE_NAME).withArguments(connection, device, specific_object).storeResultsTo(result);
        return result;
    }

    std::tuple<sdbus::ObjectPath, sdbus::ObjectPath> AddAndActivateConnection(const std::map<std::string, std::map<std::string, sdbus::Variant>>& connection, const sdbus::ObjectPath& device, const sdbus::ObjectPath& specific_object)
    {
        std::tuple<sdbus::ObjectPath, sdbus::ObjectPath> result;
        proxy_.callMethod("AddAndActivateConnection").onInterface(INTERFACE_NAME).withArguments(connection, device, specific_object).storeResultsTo(result);
        return result;
    }

    std::tuple<sdbus::ObjectPath, sdbus::ObjectPath, std::map<std::string, sdbus::Variant>> AddAndActivateConnection2(const std::map<std::string, std::map<std::string, sdbus::Variant>>& connection, const sdbus::ObjectPath& device, const sdbus::ObjectPath& specific_object, const std::map<std::string, sdbus::Variant>& options)
    {
        std::tuple<sdbus::ObjectPath, sdbus::ObjectPath, std::map<std::string, sdbus::Variant>> result;
        proxy_.callMethod("AddAndActivateConnection2").onInterface(INTERFACE_NAME).withArguments(connection, device, specific_object, options).storeResultsTo(result);
        return result;
    }

    void DeactivateConnection(const sdbus::ObjectPath& active_connection)
    {
        proxy_.callMethod("DeactivateConnection").onInterface(INTERFACE_NAME).withArguments(active_connection);
    }

    void Sleep(const bool& sleep)
    {
        proxy_.callMethod("Sleep").onInterface(INTERFACE_NAME).withArguments(sleep);
    }

    void Enable(const bool& enable)
    {
        proxy_.callMethod("Enable").onInterface(INTERFACE_NAME).withArguments(enable);
    }

    std::map<std::string, std::string> GetPermissions()
    {
        std::map<std::string, std::string> result;
        proxy_.callMethod("GetPermissions").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    void SetLogging(const std::string& level, const std::string& domains)
    {
        proxy_.callMethod("SetLogging").onInterface(INTERFACE_NAME).withArguments(level, domains);
    }

    std::tuple<std::string, std::string> GetLogging()
    {
        std::tuple<std::string, std::string> result;
        proxy_.callMethod("GetLogging").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    uint32_t CheckConnectivity()
    {
        uint32_t result;
        proxy_.callMethod("CheckConnectivity").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    uint32_t state()
    {
        uint32_t result;
        proxy_.callMethod("state").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    sdbus::ObjectPath CheckpointCreate(const std::vector<sdbus::ObjectPath>& devices, const uint32_t& rollback_timeout, const uint32_t& flags)
    {
        sdbus::ObjectPath result;
        proxy_.callMethod("CheckpointCreate").onInterface(INTERFACE_NAME).withArguments(devices, rollback_timeout, flags).storeResultsTo(result);
        return result;
    }

    void CheckpointDestroy(const sdbus::ObjectPath& checkpoint)
    {
        proxy_.callMethod("CheckpointDestroy").onInterface(INTERFACE_NAME).withArguments(checkpoint);
    }

    std::map<std::string, uint32_t> CheckpointRollback(const sdbus::ObjectPath& checkpoint)
    {
        std::map<std::string, uint32_t> result;
        proxy_.callMethod("CheckpointRollback").onInterface(INTERFACE_NAME).withArguments(checkpoint).storeResultsTo(result);
        return result;
    }

    void CheckpointAdjustRollbackTimeout(const sdbus::ObjectPath& checkpoint, const uint32_t& add_timeout)
    {
        proxy_.callMethod("CheckpointAdjustRollbackTimeout").onInterface(INTERFACE_NAME).withArguments(checkpoint, add_timeout);
    }

public:
    std::vector<sdbus::ObjectPath> Devices()
    {
        return proxy_.getProperty("Devices").onInterface(INTERFACE_NAME);
    }

    std::vector<sdbus::ObjectPath> AllDevices()
    {
        return proxy_.getProperty("AllDevices").onInterface(INTERFACE_NAME);
    }

    std::vector<sdbus::ObjectPath> Checkpoints()
    {
        return proxy_.getProperty("Checkpoints").onInterface(INTERFACE_NAME);
    }

    bool NetworkingEnabled()
    {
        return proxy_.getProperty("NetworkingEnabled").onInterface(INTERFACE_NAME);
    }

    bool WirelessEnabled()
    {
        return proxy_.getProperty("WirelessEnabled").onInterface(INTERFACE_NAME);
    }

    void WirelessEnabled(const bool& value)
    {
        proxy_.setProperty("WirelessEnabled").onInterface(INTERFACE_NAME).toValue(value);
    }

    bool WirelessHardwareEnabled()
    {
        return proxy_.getProperty("WirelessHardwareEnabled").onInterface(INTERFACE_NAME);
    }

    bool WwanEnabled()
    {
        return proxy_.getProperty("WwanEnabled").onInterface(INTERFACE_NAME);
    }

    void WwanEnabled(const bool& value)
    {
        proxy_.setProperty("WwanEnabled").onInterface(INTERFACE_NAME).toValue(value);
    }

    bool WwanHardwareEnabled()
    {
        return proxy_.getProperty("WwanHardwareEnabled").onInterface(INTERFACE_NAME);
    }

    bool WimaxEnabled()
    {
        return proxy_.getProperty("WimaxEnabled").onInterface(INTERFACE_NAME);
    }

    void WimaxEnabled(const bool& value)
    {
        proxy_.setProperty("WimaxEnabled").onInterface(INTERFACE_NAME).toValue(value);
    }

    bool WimaxHardwareEnabled()
    {
        return proxy_.getProperty("WimaxHardwareEnabled").onInterface(INTERFACE_NAME);
    }

    uint32_t RadioFlags()
    {
        return proxy_.getProperty("RadioFlags").onInterface(INTERFACE_NAME);
    }

    std::vector<sdbus::ObjectPath> ActiveConnections()
    {
        return proxy_.getProperty("ActiveConnections").onInterface(INTERFACE_NAME);
    }

    sdbus::ObjectPath PrimaryConnection()
    {
        return proxy_.getProperty("PrimaryConnection").onInterface(INTERFACE_NAME);
    }

    std::string PrimaryConnectionType()
    {
        return proxy_.getProperty("PrimaryConnectionType").onInterface(INTERFACE_NAME);
    }

    uint32_t Metered()
    {
        return proxy_.getProperty("Metered").onInterface(INTERFACE_NAME);
    }

    sdbus::ObjectPath ActivatingConnection()
    {
        return proxy_.getProperty("ActivatingConnection").onInterface(INTERFACE_NAME);
    }

    bool Startup()
    {
        return proxy_.getProperty("Startup").onInterface(INTERFACE_NAME);
    }

    std::string Version()
    {
        return proxy_.getProperty("Version").onInterface(INTERFACE_NAME);
    }

    std::vector<uint32_t> VersionInfo()
    {
        return proxy_.getProperty("VersionInfo").onInterface(INTERFACE_NAME);
    }

    std::vector<uint32_t> Capabilities()
    {
        return proxy_.getProperty("Capabilities").onInterface(INTERFACE_NAME);
    }

    uint32_t State()
    {
        return proxy_.getProperty("State").onInterface(INTERFACE_NAME);
    }

    uint32_t Connectivity()
    {
        return proxy_.getProperty("Connectivity").onInterface(INTERFACE_NAME);
    }

    bool ConnectivityCheckAvailable()
    {
        return proxy_.getProperty("ConnectivityCheckAvailable").onInterface(INTERFACE_NAME);
    }

    bool ConnectivityCheckEnabled()
    {
        return proxy_.getProperty("ConnectivityCheckEnabled").onInterface(INTERFACE_NAME);
    }

    void ConnectivityCheckEnabled(const bool& value)
    {
        proxy_.setProperty("ConnectivityCheckEnabled").onInterface(INTERFACE_NAME).toValue(value);
    }

    std::string ConnectivityCheckUri()
    {
        return proxy_.getProperty("ConnectivityCheckUri").onInterface(INTERFACE_NAME);
    }

    std::map<std::string, sdbus::Variant> GlobalDnsConfiguration()
    {
        return proxy_.getProperty("GlobalDnsConfiguration").onInterface(INTERFACE_NAME);
    }

    void GlobalDnsConfiguration(const std::map<std::string, sdbus::Variant>& value)
    {
        proxy_.setProperty("GlobalDnsConfiguration").onInterface(INTERFACE_NAME).toValue(value);
    }

private:
    sdbus::IProxy& proxy_;
};

}} // namespaces

#endif