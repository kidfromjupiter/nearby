#include <signal.h>

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/synchronization/notification.h"
#include "connections/connection_options.h"
#include "connections/core.h"
#include "connections/payload_type.h"
#include "connections/v3/advertising_options.h"
#include "connections/v3/connections_device.h"
#include "connections/v3/discovery_options.h"
#include "connections/v3/listeners.h"
#include "connections/implementation/service_controller_router.h"
#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/platform/file.h"
#include "internal/flags/nearby_flags.h"

namespace {
constexpr char kDefaultServiceId[] = "com.google.nearby.fileshare.cli";

absl::Notification g_shutdown;

void QuitHandler(int, siginfo_t*, void*) {
  if (!g_shutdown.HasBeenNotified()) {
    g_shutdown.Notify();
  }
}

struct Options {
  bool advertise = false;
  bool discover = false;
  std::string service_id = kDefaultServiceId;
  std::string save_dir;
  std::vector<std::string> send_paths;
  nearby::connections::BooleanMediumSelector mediums =
      nearby::connections::BooleanMediumSelector().SetAll(true);
  nearby::connections::BooleanMediumSelector upgrade_mediums =
      nearby::connections::BooleanMediumSelector().SetAll(true);
  bool upgrade_mediums_set = false;
};

void PrintUsage(const char* prog) {
  std::cerr
      << "Usage: " << prog
      << " [--advertise] [--discover] [--mediums=LIST]\n"
      << "              [--upgrade_mediums=LIST] [--send=PATH]\n"
      << "              [--save_dir=DIR] [--service_id=ID]\n"
      << "Flags:\n"
      << "  --advertise            Enable advertising\n"
      << "  --discover             Enable discovery\n"
      << "  --mediums=LIST          Comma-separated list of mediums to use for\n"
      << "                         advertising + discovery\n"
      << "  --upgrade_mediums=LIST  Comma-separated list of mediums to use for\n"
      << "                         upgrade (defaults to --mediums)\n"
      << "                         (bluetooth,ble,wifi_lan,wifi_hotspot,wifi_direct,\n"
      << "                          web_rtc,web_rtc_non_cellular,awdl,all)\n"
      << "  --send=PATH             File to send after connection (repeatable)\n"
      << "  --save_dir=DIR          Directory for received files\n"
      << "  --service_id=ID         Override service ID\n"
      << "  -h, --help              Show this help\n"
      << "Examples:\n"
      << "  " << prog
      << " --advertise --discover --mediums=ble --upgrade_mediums=wifi_lan\n"
      << "    --send=/tmp/hello.txt\n"
      << "  " << prog << " --discover --mediums=wifi_lan --save_dir=/tmp\n";
}

bool StartsWith(std::string_view input, std::string_view prefix) {
  return input.size() >= prefix.size() &&
         input.substr(0, prefix.size()) == prefix;
}

std::string Trim(std::string_view input) {
  size_t start = 0;
  size_t end = input.size();
  while (start < end && std::isspace(static_cast<unsigned char>(input[start]))) {
    ++start;
  }
  while (end > start &&
         std::isspace(static_cast<unsigned char>(input[end - 1]))) {
    --end;
  }
  return std::string(input.substr(start, end - start));
}

std::vector<std::string> SplitCommaList(std::string_view input) {
  std::vector<std::string> tokens;
  size_t start = 0;
  while (start <= input.size()) {
    size_t comma = input.find(',', start);
    if (comma == std::string_view::npos) comma = input.size();
    tokens.push_back(Trim(input.substr(start, comma - start)));
    start = comma + 1;
  }
  return tokens;
}

bool ApplyMediumToken(nearby::connections::BooleanMediumSelector& selector,
                      const std::string& token, std::string* error) {
  if (token.empty()) {
    return true;
  }
  if (token == "all") {
    selector.SetAll(true);
    return true;
  }
  if (token == "bluetooth") {
    selector.bluetooth = true;
    return true;
  }
  if (token == "ble") {
    selector.ble = true;
    return true;
  }
  if (token == "wifi_lan") {
    selector.wifi_lan = true;
    return true;
  }
  if (token == "wifi_hotspot") {
    selector.wifi_hotspot = true;
    return true;
  }
  if (token == "wifi_direct") {
    selector.wifi_direct = true;
    return true;
  }
  if (token == "web_rtc") {
    selector.web_rtc = true;
    selector.web_rtc_no_cellular = true;
    return true;
  }
  if (token == "web_rtc_non_cellular") {
    selector.web_rtc_no_cellular = true;
    return true;
  }
  if (token == "awdl") {
    selector.awdl = true;
    return true;
  }
  if (error != nullptr) {
    *error = "Unknown medium: " + token;
  }
  return false;
}

bool ParseMediums(const std::string& list,
                  nearby::connections::BooleanMediumSelector* selector,
                  std::string* error) {
  selector->SetAll(false);
  for (const auto& token : SplitCommaList(list)) {
    if (!ApplyMediumToken(*selector, token, error)) {
      return false;
    }
  }
  if (!selector->Any(true)) {
    if (error != nullptr) {
      *error = "No valid mediums specified";
    }
    return false;
  }
  return true;
}

bool ReadValueFlag(std::string_view arg, std::string_view name, int* index,
                   int argc, char** argv, std::string* out) {
  if (arg == name) {
    if (*index + 1 >= argc) {
      return false;
    }
    *out = argv[++(*index)];
    return true;
  }
  std::string prefix = std::string(name) + "=";
  if (StartsWith(arg, prefix)) {
    *out = std::string(arg.substr(prefix.size()));
    return true;
  }
  return false;
}

std::string MakeEndpointInfo() {
  std::string name;
  name.reserve(5);
  for (int i = 0; i < 5; ++i) {
    name.push_back('0' + (std::rand() % 10));
  }
  return name;
}

}  // namespace

class FileShareApp {
 public:
  explicit FileShareApp(Options options)
      : options_(std::move(options)),
        router_(std::make_unique<nearby::connections::ServiceControllerRouter>()),
        core_(std::make_unique<nearby::connections::Core>(router_.get())),
        local_device_(nearby::connections::v3::ConnectionsDevice(
            MakeEndpointInfo(), {})) {}

  void Start() {
    if (!options_.save_dir.empty()) {
      std::error_code error;
      std::filesystem::create_directories(options_.save_dir, error);
      if (error) {
        LOG(WARNING) << "Failed to create save dir: " << options_.save_dir
                     << " error=" << error.message();
      }
      core_->SetCustomSavePath(
          options_.save_dir,
          [](nearby::connections::Status status) {
            LOG(INFO) << "SetCustomSavePath status: " << status.ToString();
          });
    }

    if (options_.advertise) {
      StartAdvertising();
    }
    if (options_.discover) {
      StartDiscovery();
    }
  }

 private:
  void StartAdvertising() {
    nearby::connections::v3::AdvertisingOptions advertising;
    advertising.strategy = nearby::connections::Strategy::kP2pCluster;
    advertising.advertising_mediums = options_.mediums;
    advertising.upgrade_mediums =
        options_.upgrade_mediums_set ? options_.upgrade_mediums
                                     : options_.mediums;

    core_->StartAdvertisingV3(
        options_.service_id, advertising, local_device_,
        MakeConnectionListener(),
        [](nearby::connections::Status status) {
          LOG(INFO) << "Advertising status: " << status.ToString();
        });
  }

  void StartDiscovery() {
    nearby::connections::v3::DiscoveryOptions discovery;
    discovery.strategy = nearby::connections::Strategy::kP2pCluster;
    discovery.discovery_mediums = options_.mediums;

    core_->StartDiscoveryV3(
        options_.service_id, discovery, MakeDiscoveryListener(),
        [](nearby::connections::Status status) {
          LOG(INFO) << "Discovery status: " << status.ToString();
        });
  }

  nearby::connections::v3::ConnectionListener MakeConnectionListener() {
    nearby::connections::v3::ConnectionListener listener;
    listener.initiated_cb =
        [this](const nearby::NearbyDevice& remote_device,
               const nearby::connections::v3::InitialConnectionInfo& info) {
          LOG(INFO) << "Connection initiated with "
                    << remote_device.GetEndpointId()
                    << " auth_digits=" << info.authentication_digits;
          core_->AcceptConnectionV3(
              remote_device, MakePayloadListener(),
              [](nearby::connections::Status status) {
                LOG(INFO) << "AcceptConnection status: " << status.ToString();
              });
        };

    listener.result_cb =
        [this](const nearby::NearbyDevice& remote_device,
               nearby::connections::v3::ConnectionResult result) {
          LOG(INFO) << "Connection result for "
                    << remote_device.GetEndpointId()
                    << ": " << result.status.ToString();
          if (result.status.Ok()) {
            SendFilesTo(remote_device);
          }
        };

    listener.disconnected_cb =
        [this](const nearby::NearbyDevice& remote_device) {
          LOG(INFO) << "Disconnected from " << remote_device.GetEndpointId();
        };
    return listener;
  }

  nearby::connections::v3::DiscoveryListener MakeDiscoveryListener() {
    nearby::connections::v3::DiscoveryListener listener;
    listener.endpoint_found_cb =
        [this](const nearby::NearbyDevice& remote_device,
               const absl::string_view service_id) {
          LOG(INFO) << "Found endpoint " << remote_device.GetEndpointId()
                    << " service_id=" << service_id;
          nearby::connections::ConnectionOptions options;
          options.strategy = nearby::connections::Strategy::kP2pStar;
          options.allowed = options_.upgrade_mediums_set
                                ? options_.upgrade_mediums
                                : options_.mediums;
          options.auto_upgrade_bandwidth = true;
          auto device = CacheDiscoveredDevice(remote_device);
          // disable advertising
          core_ -> StopAdvertisingV3(
              [](nearby::connections::Status status) {
                LOG(INFO) << "StopAdvertising status: " << status.ToString();
              });
          core_->RequestConnectionV3(
              local_device_, *device, options, MakeConnectionListener(),
              [](nearby::connections::Status status) {
                LOG(INFO) << "RequestConnection status: " << status.ToString();
              });
        };
    listener.endpoint_lost_cb =
        [](const nearby::NearbyDevice& remote_device) {
          LOG(INFO) << "Lost endpoint " << remote_device.GetEndpointId();
        };
    return listener;
  }

  nearby::connections::v3::PayloadListener MakePayloadListener() {
    nearby::connections::v3::PayloadListener listener;
    listener.payload_received_cb =
        [this](const nearby::NearbyDevice& remote_device,
               nearby::connections::Payload payload) {
          LOG(INFO) << "Payload received from "
                    << remote_device.GetEndpointId()
                    << " id=" << payload.GetId();
          if (payload.GetType() == nearby::connections::PayloadType::kFile) {
            if (auto* file = payload.AsFile()) {
              std::lock_guard<std::mutex> lock(incoming_mutex_);
              incoming_files_[payload.GetId()] = file->GetFilePath();
            }
          }
        };

    listener.payload_progress_cb =
        [this](const nearby::NearbyDevice& remote_device,
               const nearby::connections::PayloadProgressInfo& info) {
          if (info.status ==
              nearby::connections::PayloadProgressInfo::Status::kSuccess) {
            std::string path;
            {
              std::lock_guard<std::mutex> lock(incoming_mutex_);
              auto it = incoming_files_.find(info.payload_id);
              if (it != incoming_files_.end()) {
                path = it->second;
                incoming_files_.erase(it);
              }
            }
            if (!path.empty()) {
              LOG(INFO) << "Received file from "
                        << remote_device.GetEndpointId() << " path=" << path;
            } else {
              LOG(INFO) << "Received file from "
                        << remote_device.GetEndpointId()
                        << " payload_id=" << info.payload_id;
            }
          } else if (info.status ==
                     nearby::connections::PayloadProgressInfo::Status::kFailure) {
            LOG(WARNING) << "Payload failed from "
                         << remote_device.GetEndpointId()
                         << " payload_id=" << info.payload_id;
          }
        };
    return listener;
  }

  std::optional<nearby::connections::Payload> BuildFilePayload(
      const std::string& path) {
    std::error_code error;
    std::filesystem::path fs_path(path);
    if (!std::filesystem::exists(fs_path, error)) {
      LOG(ERROR) << "File does not exist: " << path;
      return std::nullopt;
    }
    auto size = std::filesystem::file_size(fs_path, error);
    if (error) {
      LOG(ERROR) << "Failed to get file size: " << path
                 << " error=" << error.message();
      return std::nullopt;
    }
    std::string file_name = fs_path.filename().string();
    if (file_name.empty()) {
      LOG(ERROR) << "Invalid file name: " << path;
      return std::nullopt;
    }
    nearby::InputFile input_file(path);
    return nearby::connections::Payload("", file_name, std::move(input_file));
  }

  void SendFilesTo(const nearby::NearbyDevice& remote_device) {
    if (options_.send_paths.empty()) {
      return;
    }
    const std::string endpoint_id = remote_device.GetEndpointId();
    {
      std::lock_guard<std::mutex> lock(sent_mutex_);
      if (!sent_to_.insert(endpoint_id).second) {
        return;
      }
    }
    for (const auto& path : options_.send_paths) {
      auto payload = BuildFilePayload(path);
      if (!payload.has_value()) {
        continue;
      }
      core_->SendPayloadV3(
          remote_device, std::move(payload.value()),
          [endpoint_id](nearby::connections::Status status) {
            LOG(INFO) << "SendPayload to " << endpoint_id
                      << " status=" << status.ToString();
          });
    }
  }

  std::shared_ptr<nearby::connections::v3::ConnectionsDevice>
  CacheDiscoveredDevice(const nearby::NearbyDevice& remote_device) {
    const std::string endpoint_id = remote_device.GetEndpointId();
    std::lock_guard<std::mutex> lock(discovered_mutex_);
    auto it = discovered_devices_.find(endpoint_id);
    if (it != discovered_devices_.end()) {
      return it->second;
    }
    std::string endpoint_info;
    if (remote_device.GetType() ==
        nearby::NearbyDevice::Type::kConnectionsDevice) {
      auto* connections_device =
          dynamic_cast<const nearby::connections::v3::ConnectionsDevice*>(
              &remote_device);
      if (connections_device != nullptr) {
        endpoint_info = connections_device->GetEndpointInfo();
      }
    }
    auto device = std::make_shared<nearby::connections::v3::ConnectionsDevice>(
        endpoint_id, endpoint_info, remote_device.GetConnectionInfos());
    discovered_devices_.emplace(endpoint_id, device);
    return device;
  }

  Options options_;
  std::unique_ptr<nearby::connections::ServiceControllerRouter> router_;
  std::unique_ptr<nearby::connections::Core> core_;
  nearby::connections::v3::ConnectionsDevice local_device_;
  std::mutex discovered_mutex_;
  std::unordered_map<std::string,
                     std::shared_ptr<nearby::connections::v3::ConnectionsDevice>>
      discovered_devices_;
  std::mutex incoming_mutex_;
  std::unordered_map<std::int64_t, std::string> incoming_files_;
  std::mutex sent_mutex_;
  std::unordered_set<std::string> sent_to_;
};

int main(int argc, char** argv) {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      ::nearby::connections::config_package_nearby::
          nearby_connections_feature::kEnableBleL2cap,
      true);

  Options options;
  bool mediums_override = false;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (arg == "--advertise" || arg == "--advert") {
      options.advertise = true;
      continue;
    }
    if (arg == "--discover" || arg == "--scan") {
      options.discover = true;
      continue;
    }
    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return 0;
    }
    std::string value;
    if (ReadValueFlag(arg, "--mediums", &i, argc, argv, &value)) {
      std::string error;
      nearby::connections::BooleanMediumSelector selector;
      if (!ParseMediums(value, &selector, &error)) {
        std::cerr << "Error: " << error << "\n";
        return 2;
      }
      options.mediums = selector;
      mediums_override = true;
      continue;
    }
    if (ReadValueFlag(arg, "--upgrade_mediums", &i, argc, argv, &value)) {
      std::string error;
      nearby::connections::BooleanMediumSelector selector;
      if (!ParseMediums(value, &selector, &error)) {
        std::cerr << "Error: " << error << "\n";
        return 2;
      }
      options.upgrade_mediums = selector;
      options.upgrade_mediums_set = true;
      continue;
    }
    if (ReadValueFlag(arg, "--send", &i, argc, argv, &value)) {
      if (value.empty()) {
        std::cerr << "Error: --send requires a path\n";
        return 2;
      }
      options.send_paths.push_back(value);
      continue;
    }
    if (ReadValueFlag(arg, "--save_dir", &i, argc, argv, &value)) {
      options.save_dir = value;
      continue;
    }
    if (ReadValueFlag(arg, "--service_id", &i, argc, argv, &value)) {
      options.service_id = value;
      continue;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    PrintUsage(argv[0]);
    return 2;
  }

  if (!options.advertise && !options.discover) {
    std::cerr << "Error: specify at least one of --advertise or --discover\n";
    PrintUsage(argv[0]);
    return 2;
  }

  if (mediums_override && !options.mediums.Any(true)) {
    std::cerr << "Error: no mediums enabled\n";
    return 2;
  }
  if (options.upgrade_mediums_set && !options.upgrade_mediums.Any(true)) {
    std::cerr << "Error: no upgrade mediums enabled\n";
    return 2;
  }

  struct sigaction action {};
  action.sa_sigaction = QuitHandler;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);

  FileShareApp app(std::move(options));
  app.Start();

  g_shutdown.WaitForNotification();
  LOG(INFO) << "Shutting down";
  return 0;
}
