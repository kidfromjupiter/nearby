#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "absl/time/time.h"
#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/flags/nearby_flags.h"
#include "sharing/flags/generated/nearby_sharing_feature_flags.h"
#include "sharing/linux/daemon/daemon_service.h"
#include "sharing/linux/daemon/ipc_server.h"
#include "sharing/linux/nearby_noop_analytics_recorder.h"
#include "sharing/linux/platform/linux_sharing_platform.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/nearby_sharing_service_factory.h"
#include "sharing/nearby_sharing_settings.h"
#include "sharing/proto/enums.pb.h"

namespace nearby::sharing::linux {
namespace {

std::atomic<bool> g_interrupted = false;
IPCServer* g_ipc_server = nullptr;

void HandleSignal(int signal) {
  static_cast<void>(signal);
  g_interrupted = true;
  if (g_ipc_server != nullptr) {
    g_ipc_server->Stop();
  }
}

std::string GetHostname() {
  char hostname[256] = {};
  if (gethostname(hostname, sizeof(hostname)) == 0 && hostname[0] != '\0') {
    return std::string(hostname);
  }
  return "LinuxShare";
}

std::string GetDeviceName(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--name" && i + 1 < argc) {
      return argv[i + 1];
    }
  }
  const char* env_name = std::getenv("NEARBY_DEVICE_NAME");
  if (env_name != nullptr && *env_name != '\0') {
    return env_name;
  }
  return GetHostname();
}

template <typename Invoker>
NearbySharingService::StatusCodes WaitForStatus(Invoker invoker) {
  std::mutex mutex;
  std::condition_variable cv;
  std::optional<NearbySharingService::StatusCodes> status;
  invoker([&](NearbySharingService::StatusCodes callback_status) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      status = callback_status;
    }
    cv.notify_one();
  });

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [&] { return status.has_value(); });
  return *status;
}

void ConfigureFlags() {
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::sharing::config_package_nearby::nearby_sharing_feature::
          kEnableBleForTransfer,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kEnableBleL2cap,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kRefactorBleL2cap,
      true);
}

bool ConfigureSharingService(NearbySharingService& service,
                             const std::string& device_name) {
  service.GetSettings()->SetDataUsage(proto::WIFI_ONLY_DATA_USAGE);
  service.GetSettings()->SetDeviceName(
      device_name, [](DeviceNameValidationResult validation_result) {
        static_cast<void>(validation_result);
      });
  auto status = WaitForStatus([&](auto callback) {
    service.SetVisibility(proto::DEVICE_VISIBILITY_EVERYONE,
                          absl::ZeroDuration(), std::move(callback));
  });
  if (status != NearbySharingService::StatusCodes::kOk) {
    std::cerr << "SetVisibility failed: "
              << NearbySharingService::StatusCodeToString(status) << std::endl;
    return false;
  }
  return true;
}

}  // namespace
}  // namespace nearby::sharing::linux

int main(int argc, char** argv) {
  signal(SIGINT, nearby::sharing::linux::HandleSignal);
  signal(SIGTERM, nearby::sharing::linux::HandleSignal);

  nearby::sharing::linux::ConfigureFlags();

  const std::string device_name =
      nearby::sharing::linux::GetDeviceName(argc, argv);
  auto analytics_recorder = nearby::sharing::linux::NoOpAnalyticsRecorder();
  auto linux_platform =
      nearby::sharing::linux::LinuxSharingPlatform(device_name);
  auto* service =
      nearby::sharing::NearbySharingServiceFactory::GetInstance()
          ->CreateSharingService(linux_platform, &analytics_recorder,
                                 /*event_logger=*/nullptr,
                                 /*supports_file_sync=*/false);
  if (service == nullptr) {
    std::cerr << "failed to create NearbySharingService" << std::endl;
    return 1;
  }
  if (!nearby::sharing::linux::ConfigureSharingService(*service, device_name)) {
    nearby::sharing::linux::WaitForStatus(
        [&](auto callback) { service->Shutdown(std::move(callback)); });
    return 1;
  }

  IPCServer ipc_server;
  nearby::sharing::linux::g_ipc_server = &ipc_server;
  nearby::sharing::linux::DaemonService daemon(
      *service, [&](const nlohmann::json& event) { ipc_server.SendJson(event); });

  const std::string commands[] = {
      "status",        "start_receive", "stop_receive", "start_discovery",
      "stop_discovery", "send_file",     "accept",       "reject",
      "cancel",        "shutdown",
  };
  for (const std::string& command : commands) {
    ipc_server.RegisterHandler(command, [&](const nlohmann::json& request) {
      nlohmann::json result = daemon.HandleCommand(request);
      ipc_server.SendJson(result);
      if (request.value("command", "") == "shutdown") {
        ipc_server.Stop();
      }
    });
  }

  std::thread server_thread([&ipc_server]() { ipc_server.StartEventLoop(); });
  std::thread dispatch_thread([&ipc_server]() { ipc_server.DispatchLoop(); });

  server_thread.join();
  ipc_server.Stop();
  if (dispatch_thread.joinable()) {
    dispatch_thread.join();
  }
  daemon.Shutdown();
  nearby::sharing::linux::g_ipc_server = nullptr;
  return nearby::sharing::linux::g_interrupted ? 130 : 0;
}
