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

#include <signal.h>

#include <atomic>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "absl/types/span.h"
#include "connections/advertising_options.h"
#include "connections/cli/command_line.h"
#include "connections/connection_options.h"
#include "connections/core.h"
#include "connections/discovery_options.h"
#include "connections/implementation/service_controller_router.h"
#include "connections/listeners.h"
#include "connections/medium_selector.h"
#include "connections/out_of_band_connection_metadata.h"
#include "connections/params.h"
#include "connections/payload.h"
#include "connections/payload_type.h"
#include "connections/status.h"
#include "connections/strategy.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/file.h"
#include "internal/platform/input_stream.h"
#include "internal/platform/mac_address.h"

namespace nearby::connections::cli {
namespace {

constexpr char kDefaultServiceId[] = "com.google.nearby.connections.cli";
constexpr char kDefaultEndpointInfo[] = "nearby-cli";

std::atomic_bool g_interrupted = false;

void SignalHandler(int) {
  g_interrupted = true;
}

std::string LowerAndHyphenate(std::string value) {
  for (char& ch : value) {
    if (ch == '_') ch = '-';
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

std::string Hex(const ByteArray& bytes) {
  constexpr char kDigits[] = "0123456789abcdef";
  std::string result;
  result.reserve(bytes.size() * 2);
  for (unsigned char ch : bytes.string_data()) {
    result.push_back(kDigits[ch >> 4]);
    result.push_back(kDigits[ch & 0x0f]);
  }
  return result;
}

bool ParseHex(std::string_view text, ByteArray* bytes, std::string* error) {
  if (text.size() % 2 != 0) {
    *error = "hex data must contain an even number of digits";
    return false;
  }
  auto nibble = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
  };
  std::string decoded(text.size() / 2, '\0');
  for (size_t i = 0; i < text.size(); i += 2) {
    int high = nibble(text[i]);
    int low = nibble(text[i + 1]);
    if (high < 0 || low < 0) {
      *error = "invalid hex digit at offset " + std::to_string(i);
      return false;
    }
    decoded[i / 2] = static_cast<char>((high << 4) | low);
  }
  *bytes = ByteArray(std::move(decoded));
  return true;
}

bool ParseStrategy(std::string value, Strategy* strategy, std::string* error) {
  value = LowerAndHyphenate(std::move(value));
  if (value == "cluster" || value == "p2p-cluster") {
    *strategy = Strategy::kP2pCluster;
  } else if (value == "star" || value == "p2p-star") {
    *strategy = Strategy::kP2pStar;
  } else if (value == "point-to-point" || value == "p2p-point-to-point") {
    *strategy = Strategy::kP2pPointToPoint;
  } else {
    *error = "unknown strategy '" + value +
             "' (expected cluster, star, or point-to-point)";
    return false;
  }
  return true;
}

bool ApplyMedium(std::string name, BooleanMediumSelector* selector,
                 std::string* error) {
  name = LowerAndHyphenate(std::move(name));
  if (name == "all") {
    selector->SetAll(true);
  } else if (name == "bluetooth") {
    selector->bluetooth = true;
  } else if (name == "ble") {
    selector->ble = true;
  } else if (name == "web-rtc") {
    selector->web_rtc = true;
  } else if (name == "web-rtc-non-cellular" || name == "web-rtc-no-cellular") {
    selector->web_rtc_no_cellular = true;
  } else if (name == "wifi-lan") {
    selector->wifi_lan = true;
  } else if (name == "wifi-hotspot") {
    selector->wifi_hotspot = true;
  } else if (name == "wifi-direct") {
    selector->wifi_direct = true;
  } else if (name == "awdl") {
    selector->awdl = true;
  } else {
    *error = "unknown medium '" + name + "'";
    return false;
  }
  return true;
}

bool ParseMediums(std::string_view value, BooleanMediumSelector* selector,
                  std::string* error) {
  selector->SetAll(false);
  selector->web_rtc_no_cellular = false;
  for (const std::string& item : SplitCommaList(value)) {
    if (item.empty() || !ApplyMedium(item, selector, error)) return false;
  }
  if (!selector->Any(true)) {
    *error = "at least one medium is required";
    return false;
  }
  return true;
}

std::optional<Medium> ParseMedium(std::string value, std::string* error) {
  BooleanMediumSelector selector;
  selector.SetAll(false);
  selector.web_rtc_no_cellular = false;
  if (!ApplyMedium(std::move(value), &selector, error) ||
      selector.Count(true) != 1) {
    if (error->empty()) *error = "exactly one medium is required";
    return std::nullopt;
  }
  std::vector<Medium> mediums = selector.GetMediums(true);
  if (mediums.size() != 1) {
    *error = "exactly one medium is required";
    return std::nullopt;
  }
  return mediums.front();
}

bool ParseEndpointInfo(const CommandLine& command, std::string_view text_name,
                       std::string_view hex_name, std::string_view fallback,
                       ByteArray* result, std::string* error) {
  auto text = GetString(command, text_name);
  auto hex = GetString(command, hex_name);
  if (text && hex) {
    *error = "--" + std::string(text_name) + " and --" + std::string(hex_name) +
             " are mutually exclusive";
    return false;
  }
  if (hex) return ParseHex(*hex, result, error);
  *result = ByteArray(text.value_or(std::string(fallback)));
  return true;
}

const std::unordered_set<std::string>& CommonOptions() {
  static const auto* options = new std::unordered_set<std::string>{
      "strategy",  "mediums",     "auto-upgrade", "enforce-topology",
      "low-power", "out-of-band", "fast-uuid"};
  return *options;
}

std::unordered_set<std::string> WithCommon(
    std::initializer_list<const char*> extra) {
  std::unordered_set<std::string> result = CommonOptions();
  for (const char* value : extra) result.insert(value);
  return result;
}

bool ConfigureBase(const CommandLine& command, OptionsBase* options,
                   std::string* error) {
  auto strategy = GetString(command, "strategy");
  if (!ParseStrategy(strategy.value_or("cluster"), &options->strategy, error)) {
    return false;
  }
  auto mediums = GetString(command, "mediums");
  if (mediums && !ParseMediums(*mediums, &options->allowed, error))
    return false;
  return true;
}

bool SetBool(const CommandLine& command, std::string_view name,
             bool default_value, bool* target, std::string* error) {
  return GetBool(command, name, default_value, target, error);
}

bool CheckedInt(const CommandLine& command, std::string_view name,
                std::int64_t default_value, int* target, std::string* error) {
  std::int64_t value;
  if (!GetInt64(command, name, default_value, &value, error)) return false;
  if (value < std::numeric_limits<int>::min() ||
      value > std::numeric_limits<int>::max()) {
    *error = "--" + std::string(name) + " is outside the supported range";
    return false;
  }
  *target = static_cast<int>(value);
  return true;
}

std::string ProgressStatus(PayloadProgressInfo::Status status) {
  switch (status) {
    case PayloadProgressInfo::Status::kSuccess:
      return "success";
    case PayloadProgressInfo::Status::kFailure:
      return "failure";
    case PayloadProgressInfo::Status::kInProgress:
      return "in-progress";
    case PayloadProgressInfo::Status::kCanceled:
      return "canceled";
  }
  return "unknown";
}

std::string MediumName(Medium medium) {
  return location::nearby::proto::connections::Medium_Name(medium);
}

void PrintUsage(std::ostream& out, const char* program) {
  out << "Nearby Connections CLI (classic endpoint API)\n\n"
      << "Usage: " << program
      << " [--execute COMMAND]... [--script FILE] [--no-repl]\n\n"
      << "The process stays alive so asynchronous events can be handled. At "
         "the\n"
      << "nearby> prompt, run `help` for commands and `help options` for "
         "every\n"
      << "AdvertisingOptions, DiscoveryOptions, and ConnectionOptions flag.\n";
}

void PrintHelp(std::ostream& out, bool options) {
  if (!options) {
    out << R"(Commands:
  advertise [options]                  StartAdvertising
  stop-advertising                     StopAdvertising
  discover [options]                   StartDiscovery
  stop-discovery                       StopDiscovery
  inject ENDPOINT_ID INFO [options]    InjectEndpoint
  request ENDPOINT_ID [options]        RequestConnection
  accept ENDPOINT_ID                   AcceptConnection
  reject ENDPOINT_ID                   RejectConnection
  send-bytes ENDPOINTS DATA [--hex]    Send a bytes Payload
  send-file ENDPOINTS PATH [options]   Send a file Payload
  send-stream ENDPOINTS PATH [options] Send a stream Payload
  cancel PAYLOAD_ID                    CancelPayload
  disconnect ENDPOINT_ID               DisconnectFromEndpoint
  stop-all                             StopAllEndpoints
  upgrade ENDPOINT_ID                  InitiateBandwidthUpgrade
  save-path PATH                       SetCustomSavePath
  override-save-path ENDPOINT PATH     OverrideSavePath
  endpoint-id                          GetLocalEndpointId
  dump                                 Dump internal state
  endpoints                            List discovered/pending/connected peers
  help [options]                       Show help
  quit                                 Stop all activity and exit

ENDPOINTS is one endpoint ID or a comma-separated list. Quote data and paths
containing spaces. API results and listener events are printed asynchronously.
Connections are manual by default; add --auto-accept to advertise/request.
Run `help options` for the complete option flag reference.
)";
    return;
  }
  out << R"(Common option fields:
  --strategy cluster|star|point-to-point        (default: cluster)
  --mediums LIST                                bluetooth, ble, web-rtc,
       web-rtc-non-cellular, wifi-lan, wifi-hotspot, wifi-direct, awdl, all
  --[no-]auto-upgrade                           (default: true)
  --[no-]enforce-topology                       (API struct default)
  --[no-]low-power                              (default: false)
  --[no-]out-of-band                            (default: false)
  --fast-uuid UUID

advertise additionally exposes every AdvertisingOptions field:
  --service-id ID                               (default: com.google.nearby.connections.cli)
  --endpoint-info TEXT | --endpoint-info-hex HEX
  --device-info TEXT
  --[no-]enable-bluetooth-listening
  --[no-]enable-webrtc-listening
  --[no-]stable-endpoint-id
  --[no-]force-new-endpoint-id
  --[no-]auto-accept

discover additionally exposes every DiscoveryOptions field:
  --service-id ID
  --ble-alternate-uuid UINT16

request additionally exposes every ConnectionOptions/ConnectionInfo field:
  --endpoint-info TEXT | --endpoint-info-hex HEX
  --[no-]enable-bluetooth-listening
  --[no-]enable-webrtc-listening
  --remote-bluetooth-mac XX:XX:XX:XX:XX:XX
  --keep-alive-interval MILLISECONDS
  --keep-alive-timeout MILLISECONDS
  --[no-]non-disruptive-hotspot
  --local-endpoint-id ID
  --nonce INTEGER
  --[no-]supports-5ghz
  --bssid VALUE
  --ap-frequency INTEGER
  --supported-mediums LIST
  --connection-keep-alive-interval MILLISECONDS
  --connection-keep-alive-timeout MILLISECONDS
  --medium-roles LIST  (wifi-direct-group-owner,wifi-direct-group-client,
       wifi-hotspot-host,wifi-hotspot-client,wifi-aware-publisher,
       wifi-aware-subscriber,awdl-publisher,awdl-subscriber)
  --wifi-direct-auth LIST  (unknown,password,pin,device-name)
  --[no-]auto-accept

inject options:
  --service-id ID --medium MEDIUM --remote-bluetooth-mac MAC
  --ble-peripheral-id ID --psm INTEGER

send-bytes: --hex makes DATA hexadecimal.
send-file: --parent-folder RELATIVE --file-name NAME --offset BYTES.
send-stream: --offset BYTES.
Boolean flags accept true/false, yes/no, on/off, 1/0 and --no-FLAG.
)";
}

enum class EndpointState { kDiscovered, kPending, kConnected };

struct Endpoint {
  std::string info;
  EndpointState state = EndpointState::kDiscovered;
};

class FileInputStream final : public InputStream {
 public:
  explicit FileInputStream(std::string path) : file_(std::move(path)) {}

  ExceptionOr<ByteArray> Read(std::int64_t size) override {
    return file_.Read(size);
  }
  ExceptionOr<size_t> Skip(size_t offset) override {
    return file_.Skip(offset);
  }
  Exception Close() override { return file_.Close(); }
  bool IsValid() const { return file_.GetTotalSize() >= 0; }

 private:
  InputFile file_;
};

}  // namespace

class NearbyCli {
 public:
  NearbyCli()
      : router_(std::make_unique<ServiceControllerRouter>()),
        core_(std::make_unique<Core>(router_.get())) {}

  ~NearbyCli() {
    if (!stopped_) {
      core_->StopAllEndpoints([](Status) {});
    }
  }

  bool Execute(const CommandLine& command) {
    if (command.command.empty() || command.command[0] == '#') return true;
    std::string error;
    bool keep_running = Dispatch(command, &error);
    if (!error.empty()) Print("error: " + error);
    return keep_running;
  }

  void PrintPrompt() {
    std::lock_guard<std::mutex> lock(output_mutex_);
    std::cout << "nearby> " << std::flush;
  }

 private:
  void Print(std::string message) {
    std::lock_guard<std::mutex> lock(output_mutex_);
    std::cout << message << '\n' << std::flush;
  }

  ResultCallback Result(std::string operation) {
    return [this, operation = std::move(operation)](Status status) {
      Print(operation + ": " + status.ToString());
    };
  }

  ConnectionListener MakeConnectionListener(bool auto_accept) {
    ConnectionListener listener;
    listener.initiated_cb = [this, auto_accept](
                                const std::string& endpoint_id,
                                const ConnectionResponseInfo& info) {
      ConnectionResponseInfo display_info = info;
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        endpoints_[endpoint_id] = {info.remote_endpoint_info.string_data(),
                                   EndpointState::kPending};
      }
      Print("connection-initiated endpoint=" + endpoint_id +
            " info_hex=" + Hex(info.remote_endpoint_info) +
            " auth_digits=" + display_info.GetAuthenticationDigits() +
            " incoming=" + (info.is_incoming_connection ? "true" : "false") +
            " verified=" + (info.is_connection_verified ? "true" : "false"));
      if (auto_accept) {
        core_->AcceptConnection(endpoint_id, MakePayloadListener(),
                                Result("accept " + endpoint_id));
      }
    };
    listener.accepted_cb = [this](const std::string& endpoint_id) {
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        endpoints_[endpoint_id].state = EndpointState::kConnected;
      }
      Print("connection-accepted endpoint=" + endpoint_id);
    };
    listener.rejected_cb = [this](const std::string& endpoint_id,
                                  Status status) {
      Print("connection-rejected endpoint=" + endpoint_id +
            " status=" + status.ToString());
    };
    listener.disconnected_cb = [this](const std::string& endpoint_id) {
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        endpoints_.erase(endpoint_id);
      }
      Print("disconnected endpoint=" + endpoint_id);
    };
    listener.bandwidth_changed_cb = [this](const std::string& endpoint_id,
                                           Medium medium) {
      Print("bandwidth-changed endpoint=" + endpoint_id +
            " medium=" + MediumName(medium));
    };
    return listener;
  }

  PayloadListener MakePayloadListener() {
    PayloadListener listener;
    listener.payload_cb = [this](absl::string_view endpoint_id,
                                 Payload payload) {
      std::string message = "payload endpoint=" + std::string(endpoint_id) +
                            " id=" + std::to_string(payload.GetId());
      switch (payload.GetType()) {
        case PayloadType::kBytes:
          message +=
              " type=bytes size=" + std::to_string(payload.AsBytes().size()) +
              " data_hex=" + Hex(payload.AsBytes());
          break;
        case PayloadType::kFile: {
          message += " type=file name=" + payload.GetFileName() +
                     " parent=" + payload.GetParentFolder();
          if (InputFile* file = payload.AsFile()) {
            message += " path=" + file->GetFilePath();
          }
          break;
        }
        case PayloadType::kStream:
          message += " type=stream";
          break;
        case PayloadType::kUnknown:
          message += " type=unknown";
          break;
      }
      Print(std::move(message));
    };
    listener.payload_progress_cb = [this](absl::string_view endpoint_id,
                                          const PayloadProgressInfo& info) {
      Print("payload-progress endpoint=" + std::string(endpoint_id) +
            " id=" + std::to_string(info.payload_id) +
            " status=" + ProgressStatus(info.status) +
            " transferred=" + std::to_string(info.bytes_transferred) +
            " total=" + std::to_string(info.total_bytes));
    };
    return listener;
  }

  ConnectionRequestInfo RequestInfo(const ByteArray& endpoint_info,
                                    bool auto_accept) {
    ConnectionRequestInfo info;
    info.endpoint_info = endpoint_info;
    info.listener = MakeConnectionListener(auto_accept);
    return info;
  }

  bool Dispatch(const CommandLine& command, std::string* error) {
    if (command.command == "help") {
      if (!command.options.empty() || command.positionals.size() > 1) {
        *error = "usage: help [options]";
      } else {
        PrintHelp(std::cout, !command.positionals.empty() &&
                                 command.positionals[0] == "options");
      }
      return true;
    }
    if (command.command == "quit" || command.command == "exit") {
      if (!command.options.empty() || !command.positionals.empty()) {
        *error = "usage: quit";
        return true;
      }
      stopped_ = true;
      core_->StopAllEndpoints([](Status) {});
      return false;
    }
    if (command.command == "advertise") return Advertise(command, error);
    if (command.command == "stop-advertising") {
      return NoArgs(command, error, [this] {
        core_->StopAdvertising(Result("stop-advertising"));
      });
    }
    if (command.command == "discover") return Discover(command, error);
    if (command.command == "stop-discovery") {
      return NoArgs(command, error,
                    [this] { core_->StopDiscovery(Result("stop-discovery")); });
    }
    if (command.command == "inject") return Inject(command, error);
    if (command.command == "request") return Request(command, error);
    if (command.command == "accept") {
      return EndpointArg(command, error, [this](const std::string& endpoint) {
        core_->AcceptConnection(endpoint, MakePayloadListener(),
                                Result("accept " + endpoint));
      });
    }
    if (command.command == "reject") {
      return EndpointArg(command, error, [this](const std::string& endpoint) {
        core_->RejectConnection(endpoint, Result("reject " + endpoint));
      });
    }
    if (command.command == "send-bytes") return SendBytes(command, error);
    if (command.command == "send-file") return SendFile(command, error);
    if (command.command == "send-stream") return SendStream(command, error);
    if (command.command == "cancel") return Cancel(command, error);
    if (command.command == "disconnect") {
      return EndpointArg(command, error, [this](const std::string& endpoint) {
        core_->DisconnectFromEndpoint(endpoint,
                                      Result("disconnect " + endpoint));
      });
    }
    if (command.command == "stop-all") {
      return NoArgs(command, error,
                    [this] { core_->StopAllEndpoints(Result("stop-all")); });
    }
    if (command.command == "upgrade") {
      return EndpointArg(command, error, [this](const std::string& endpoint) {
        core_->InitiateBandwidthUpgrade(endpoint,
                                        Result("upgrade " + endpoint));
      });
    }
    if (command.command == "save-path") {
      return OneArg(command, error, "save-path PATH",
                    [this](const std::string& path) {
                      core_->SetCustomSavePath(path, Result("save-path"));
                    });
    }
    if (command.command == "override-save-path") {
      if (!command.options.empty() || command.positionals.size() != 2) {
        *error = "usage: override-save-path ENDPOINT_ID PATH";
      } else {
        core_->OverrideSavePath(command.positionals[0], command.positionals[1]);
        Print("override-save-path: submitted");
      }
      return true;
    }
    if (command.command == "endpoint-id") {
      return NoArgs(command, error,
                    [this] { Print(core_->GetLocalEndpointId()); });
    }
    if (command.command == "dump") {
      return NoArgs(command, error, [this] { Print(core_->Dump()); });
    }
    if (command.command == "endpoints") {
      return NoArgs(command, error, [this] { ListEndpoints(); });
    }
    *error = "unknown command '" + command.command + "'; run `help`";
    return true;
  }

  template <typename Callable>
  bool NoArgs(const CommandLine& command, std::string* error,
              Callable callable) {
    if (!command.options.empty() || !command.positionals.empty()) {
      *error = "command takes no arguments";
    } else {
      callable();
    }
    return true;
  }

  template <typename Callable>
  bool OneArg(const CommandLine& command, std::string* error,
              std::string_view usage, Callable callable) {
    if (!command.options.empty() || command.positionals.size() != 1) {
      *error = "usage: " + std::string(usage);
    } else {
      callable(command.positionals.front());
    }
    return true;
  }

  template <typename Callable>
  bool EndpointArg(const CommandLine& command, std::string* error,
                   Callable callable) {
    return OneArg(command, error, command.command + " ENDPOINT_ID",
                  std::move(callable));
  }

  bool Advertise(const CommandLine& command, std::string* error) {
    static const auto allowed = WithCommon(
        {"service-id", "endpoint-info", "endpoint-info-hex", "device-info",
         "enable-bluetooth-listening", "enable-webrtc-listening",
         "stable-endpoint-id", "force-new-endpoint-id", "auto-accept"});
    if (!command.positionals.empty() ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) *error = "advertise takes options only";
      return true;
    }
    AdvertisingOptions options{};
    bool auto_accept;
    ByteArray endpoint_info;
    if (!ConfigureBase(command, &options, error) ||
        !SetBool(command, "auto-upgrade", options.auto_upgrade_bandwidth,
                 &options.auto_upgrade_bandwidth, error) ||
        !SetBool(command, "enforce-topology",
                 options.enforce_topology_constraints,
                 &options.enforce_topology_constraints, error) ||
        !SetBool(command, "low-power", options.low_power, &options.low_power,
                 error) ||
        !SetBool(command, "enable-bluetooth-listening",
                 options.enable_bluetooth_listening,
                 &options.enable_bluetooth_listening, error) ||
        !SetBool(command, "enable-webrtc-listening",
                 options.enable_webrtc_listening,
                 &options.enable_webrtc_listening, error) ||
        !SetBool(command, "stable-endpoint-id", options.use_stable_endpoint_id,
                 &options.use_stable_endpoint_id, error) ||
        !SetBool(command, "force-new-endpoint-id",
                 options.force_new_endpoint_id, &options.force_new_endpoint_id,
                 error) ||
        !SetBool(command, "out-of-band", options.is_out_of_band_connection,
                 &options.is_out_of_band_connection, error) ||
        !SetBool(command, "auto-accept", false, &auto_accept, error) ||
        !ParseEndpointInfo(command, "endpoint-info", "endpoint-info-hex",
                           kDefaultEndpointInfo, &endpoint_info, error)) {
      return true;
    }
    options.fast_advertisement_service_uuid =
        GetString(command, "fast-uuid").value_or("");
    device_info_storage_ = GetString(command, "device-info").value_or("");
    options.device_info =
        device_info_storage_.empty() ? nullptr : device_info_storage_.c_str();
    std::string service_id =
        GetString(command, "service-id").value_or(kDefaultServiceId);
    core_->StartAdvertising(service_id, options,
                            RequestInfo(endpoint_info, auto_accept),
                            Result("advertise"));
    return true;
  }

  bool Discover(const CommandLine& command, std::string* error) {
    static const auto allowed =
        WithCommon({"service-id", "ble-alternate-uuid"});
    if (!command.positionals.empty() ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) *error = "discover takes options only";
      return true;
    }
    DiscoveryOptions options{};
    if (!ConfigureBase(command, &options, error) ||
        !SetBool(command, "auto-upgrade", options.auto_upgrade_bandwidth,
                 &options.auto_upgrade_bandwidth, error) ||
        !SetBool(command, "enforce-topology",
                 options.enforce_topology_constraints,
                 &options.enforce_topology_constraints, error) ||
        !SetBool(command, "low-power", options.low_power, &options.low_power,
                 error) ||
        !SetBool(command, "out-of-band", options.is_out_of_band_connection,
                 &options.is_out_of_band_connection, error)) {
      return true;
    }
    options.fast_advertisement_service_uuid =
        GetString(command, "fast-uuid").value_or("");
    std::int64_t uuid;
    if (!GetInt64(command, "ble-alternate-uuid", -1, &uuid, error)) return true;
    if (uuid < -1 || uuid > std::numeric_limits<uint16_t>::max()) {
      *error = "--ble-alternate-uuid must be between 0 and 65535";
      return true;
    }
    if (uuid >= 0) options.ble_options.alternate_uuid = uuid;

    DiscoveryListener listener;
    listener.endpoint_found_cb = [this](const std::string& endpoint_id,
                                        const ByteArray& endpoint_info,
                                        const std::string& service_id) {
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        endpoints_[endpoint_id] = {endpoint_info.string_data(),
                                   EndpointState::kDiscovered};
      }
      Print("endpoint-found endpoint=" + endpoint_id +
            " service=" + service_id + " info_hex=" + Hex(endpoint_info));
    };
    listener.endpoint_lost_cb = [this](const std::string& endpoint_id) {
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        endpoints_.erase(endpoint_id);
      }
      Print("endpoint-lost endpoint=" + endpoint_id);
    };
    listener.endpoint_distance_changed_cb =
        [this](const std::string& endpoint_id, DistanceInfo distance) {
          Print("endpoint-distance endpoint=" + endpoint_id +
                " value=" + std::to_string(static_cast<int>(distance)));
        };
    core_->StartDiscovery(
        GetString(command, "service-id").value_or(kDefaultServiceId), options,
        std::move(listener), Result("discover"));
    return true;
  }

  bool Request(const CommandLine& command, std::string* error) {
    static const auto allowed = WithCommon(
        {"endpoint-info", "endpoint-info-hex", "enable-bluetooth-listening",
         "enable-webrtc-listening", "remote-bluetooth-mac",
         "keep-alive-interval", "keep-alive-timeout", "non-disruptive-hotspot",
         "local-endpoint-id", "nonce", "supports-5ghz", "bssid", "ap-frequency",
         "supported-mediums", "connection-keep-alive-interval",
         "connection-keep-alive-timeout", "medium-roles", "wifi-direct-auth",
         "auto-accept"});
    if (command.positionals.size() != 1 ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) *error = "usage: request ENDPOINT_ID [options]";
      return true;
    }
    ConnectionOptions options{};
    bool auto_accept;
    ByteArray endpoint_info;
    if (!ConfigureBase(command, &options, error) ||
        !SetBool(command, "auto-upgrade", options.auto_upgrade_bandwidth,
                 &options.auto_upgrade_bandwidth, error) ||
        !SetBool(command, "enforce-topology",
                 options.enforce_topology_constraints,
                 &options.enforce_topology_constraints, error) ||
        !SetBool(command, "low-power", options.low_power, &options.low_power,
                 error) ||
        !SetBool(command, "enable-bluetooth-listening",
                 options.enable_bluetooth_listening,
                 &options.enable_bluetooth_listening, error) ||
        !SetBool(command, "enable-webrtc-listening",
                 options.enable_webrtc_listening,
                 &options.enable_webrtc_listening, error) ||
        !SetBool(command, "out-of-band", options.is_out_of_band_connection,
                 &options.is_out_of_band_connection, error) ||
        !SetBool(command, "non-disruptive-hotspot",
                 options.non_disruptive_hotspot_mode,
                 &options.non_disruptive_hotspot_mode, error) ||
        !SetBool(command, "supports-5ghz",
                 options.connection_info.supports_5_ghz,
                 &options.connection_info.supports_5_ghz, error) ||
        !SetBool(command, "auto-accept", false, &auto_accept, error) ||
        !ParseEndpointInfo(command, "endpoint-info", "endpoint-info-hex",
                           kDefaultEndpointInfo, &endpoint_info, error) ||
        !CheckedInt(command, "keep-alive-interval", 0,
                    &options.keep_alive_interval_millis, error) ||
        !CheckedInt(command, "keep-alive-timeout", 0,
                    &options.keep_alive_timeout_millis, error) ||
        !CheckedInt(command, "nonce", 0, &options.connection_info.nonce,
                    error) ||
        !CheckedInt(command, "ap-frequency", -1,
                    &options.connection_info.ap_frequency, error) ||
        !CheckedInt(command, "connection-keep-alive-interval", 0,
                    &options.connection_info.keep_alive_interval_millis,
                    error) ||
        !CheckedInt(command, "connection-keep-alive-timeout", 0,
                    &options.connection_info.keep_alive_timeout_millis,
                    error)) {
      return true;
    }
    options.fast_advertisement_service_uuid =
        GetString(command, "fast-uuid").value_or("");
    if (auto mac = GetString(command, "remote-bluetooth-mac");
        mac &&
        !MacAddress::FromString(*mac, options.remote_bluetooth_mac_address)) {
      *error = "invalid --remote-bluetooth-mac '" + *mac + "'";
      return true;
    }
    options.connection_info.local_endpoint_id =
        GetString(command, "local-endpoint-id").value_or("");
    options.connection_info.local_endpoint_info = endpoint_info;
    options.connection_info.bssid = GetString(command, "bssid").value_or("");
    if (auto supported = GetString(command, "supported-mediums")) {
      BooleanMediumSelector selector;
      if (!ParseMediums(*supported, &selector, error)) return true;
      options.connection_info.supported_mediums = selector.GetMediums(true);
    }
    if (!ParseMediumRoles(command, &options, error) ||
        !ParseWifiDirectAuth(command, &options, error)) {
      return true;
    }
    core_->RequestConnection(command.positionals.front(),
                             RequestInfo(endpoint_info, auto_accept), options,
                             Result("request " + command.positionals.front()));
    return true;
  }

  bool ParseMediumRoles(const CommandLine& command, ConnectionOptions* options,
                        std::string* error) {
    auto value = GetString(command, "medium-roles");
    if (!value) return true;
    location::nearby::connections::MediumRole roles;
    for (std::string role : SplitCommaList(*value)) {
      role = LowerAndHyphenate(std::move(role));
      if (role == "wifi-direct-group-owner") {
        roles.set_support_wifi_direct_group_owner(true);
      } else if (role == "wifi-direct-group-client") {
        roles.set_support_wifi_direct_group_client(true);
      } else if (role == "wifi-hotspot-host") {
        roles.set_support_wifi_hotspot_host(true);
      } else if (role == "wifi-hotspot-client") {
        roles.set_support_wifi_hotspot_client(true);
      } else if (role == "wifi-aware-publisher") {
        roles.set_support_wifi_aware_publisher(true);
      } else if (role == "wifi-aware-subscriber") {
        roles.set_support_wifi_aware_subscriber(true);
      } else if (role == "awdl-publisher") {
        roles.set_support_awdl_publisher(true);
      } else if (role == "awdl-subscriber") {
        roles.set_support_awdl_subscriber(true);
      } else {
        *error = "unknown medium role '" + role + "'";
        return false;
      }
    }
    options->connection_info.medium_role = std::move(roles);
    return true;
  }

  bool ParseWifiDirectAuth(const CommandLine& command,
                           ConnectionOptions* options, std::string* error) {
    auto value = GetString(command, "wifi-direct-auth");
    if (!value) return true;
    using Auth = location::nearby::proto::connections::WifiDirectAuthType;
    for (std::string auth : SplitCommaList(*value)) {
      auth = LowerAndHyphenate(std::move(auth));
      if (auth == "unknown") {
        options->connection_info.supported_wifi_direct_auth_types.push_back(
            Auth::WIFI_DIRECT_TYPE_UNKNOWN);
      } else if (auth == "password") {
        options->connection_info.supported_wifi_direct_auth_types.push_back(
            Auth::WIFI_DIRECT_WITH_PASSWORD);
      } else if (auth == "pin") {
        options->connection_info.supported_wifi_direct_auth_types.push_back(
            Auth::WIFI_DIRECT_WITH_PIN);
      } else if (auth == "device-name") {
        options->connection_info.supported_wifi_direct_auth_types.push_back(
            Auth::WIFI_DIRECT_WITH_DEVICE_NAME);
      } else {
        *error = "unknown Wi-Fi Direct auth type '" + auth + "'";
        return false;
      }
    }
    return true;
  }

  bool Inject(const CommandLine& command, std::string* error) {
    static const std::unordered_set<std::string> allowed = {
        "service-id", "medium", "remote-bluetooth-mac", "ble-peripheral-id",
        "psm"};
    if (command.positionals.size() != 2 ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) {
        *error = "usage: inject ENDPOINT_ID INFO --medium MEDIUM [options]";
      }
      return true;
    }
    auto medium_text = GetString(command, "medium");
    if (!medium_text) {
      *error = "inject requires --medium";
      return true;
    }
    auto medium = ParseMedium(*medium_text, error);
    if (!medium) return true;
    OutOfBandConnectionMetadata metadata;
    metadata.medium = *medium;
    metadata.endpoint_id = command.positionals[0];
    metadata.endpoint_info = ByteArray(command.positionals[1]);
    metadata.ble_peripheral_native_id =
        GetString(command, "ble-peripheral-id").value_or("");
    if (!CheckedInt(command, "psm", 0, &metadata.psm, error)) return true;
    if (auto mac = GetString(command, "remote-bluetooth-mac")) {
      MacAddress address;
      if (!MacAddress::FromString(*mac, address)) {
        *error = "invalid --remote-bluetooth-mac '" + *mac + "'";
        return true;
      }
      std::string bytes(6, '\0');
      std::vector<uint8_t> raw(6);
      address.ToBytes(absl::MakeSpan(raw));
      for (size_t i = 0; i < raw.size(); ++i) bytes[i] = raw[i];
      metadata.remote_bluetooth_mac_address = ByteArray(std::move(bytes));
    }
    core_->InjectEndpoint(
        GetString(command, "service-id").value_or(kDefaultServiceId),
        std::move(metadata), Result("inject " + command.positionals[0]));
    return true;
  }

  bool ParseEndpoints(std::string_view text,
                      std::vector<std::string>* endpoints, std::string* error) {
    *endpoints = SplitCommaList(text);
    for (const std::string& endpoint : *endpoints) {
      if (endpoint.empty()) {
        *error = "endpoint list contains an empty ID";
        return false;
      }
    }
    return true;
  }

  bool SendBytes(const CommandLine& command, std::string* error) {
    static const std::unordered_set<std::string> allowed = {"hex"};
    if (command.positionals.size() != 2 ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) {
        *error = "usage: send-bytes ENDPOINTS DATA [--hex]";
      }
      return true;
    }
    bool is_hex;
    ByteArray data;
    std::vector<std::string> endpoints;
    if (!GetBool(command, "hex", false, &is_hex, error) ||
        !ParseEndpoints(command.positionals[0], &endpoints, error)) {
      return true;
    }
    if (is_hex) {
      if (!ParseHex(command.positionals[1], &data, error)) return true;
    } else {
      data = ByteArray(command.positionals[1]);
    }
    Payload payload(std::move(data));
    Payload::Id id = payload.GetId();
    core_->SendPayload(endpoints, std::move(payload),
                       Result("send-bytes id=" + std::to_string(id)));
    return true;
  }

  bool SendFile(const CommandLine& command, std::string* error) {
    static const std::unordered_set<std::string> allowed = {
        "parent-folder", "file-name", "offset"};
    if (command.positionals.size() != 2 ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) {
        *error = "usage: send-file ENDPOINTS PATH [options]";
      }
      return true;
    }
    std::vector<std::string> endpoints;
    std::int64_t offset;
    if (!ParseEndpoints(command.positionals[0], &endpoints, error) ||
        !GetInt64(command, "offset", 0, &offset, error)) {
      return true;
    }
    if (offset < 0) {
      *error = "--offset cannot be negative";
      return true;
    }
    InputFile file(command.positionals[1]);
    if (file.GetTotalSize() < 0) {
      *error = "cannot open file '" + command.positionals[1] + "'";
      return true;
    }
    std::string file_name =
        GetString(command, "file-name").value_or(command.positionals[1]);
    size_t slash = file_name.find_last_of("/\\");
    if (slash != std::string::npos) file_name.erase(0, slash + 1);
    if (file_name.empty()) {
      *error = "file name cannot be empty";
      return true;
    }
    Payload payload(GetString(command, "parent-folder").value_or(""),
                    std::move(file_name), std::move(file));
    payload.SetOffset(static_cast<size_t>(offset));
    Payload::Id id = payload.GetId();
    core_->SendPayload(endpoints, std::move(payload),
                       Result("send-file id=" + std::to_string(id)));
    return true;
  }

  bool SendStream(const CommandLine& command, std::string* error) {
    static const std::unordered_set<std::string> allowed = {"offset"};
    if (command.positionals.size() != 2 ||
        !RejectUnknownOptions(command, allowed, error)) {
      if (error->empty()) {
        *error = "usage: send-stream ENDPOINTS PATH [--offset BYTES]";
      }
      return true;
    }
    std::vector<std::string> endpoints;
    std::int64_t offset;
    if (!ParseEndpoints(command.positionals[0], &endpoints, error) ||
        !GetInt64(command, "offset", 0, &offset, error)) {
      return true;
    }
    if (offset < 0) {
      *error = "--offset cannot be negative";
      return true;
    }
    auto stream = std::make_unique<FileInputStream>(command.positionals[1]);
    if (!stream->IsValid()) {
      *error = "cannot open file '" + command.positionals[1] + "'";
      return true;
    }
    Payload payload(std::move(stream));
    payload.SetOffset(static_cast<size_t>(offset));
    Payload::Id id = payload.GetId();
    core_->SendPayload(endpoints, std::move(payload),
                       Result("send-stream id=" + std::to_string(id)));
    return true;
  }

  bool Cancel(const CommandLine& command, std::string* error) {
    if (!command.options.empty() || command.positionals.size() != 1) {
      *error = "usage: cancel PAYLOAD_ID";
      return true;
    }
    CommandLine parsed;
    parsed.command = "cancel";
    parsed.options.emplace("id", command.positionals[0]);
    std::int64_t id;
    if (!GetInt64(parsed, "id", 0, &id, error)) return true;
    core_->CancelPayload(id, Result("cancel " + std::to_string(id)));
    return true;
  }

  void ListEndpoints() {
    std::ostringstream output;
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (endpoints_.empty()) {
      output << "no endpoints";
    } else {
      for (const auto& [id, endpoint] : endpoints_) {
        const char* state =
            endpoint.state == EndpointState::kConnected ? "connected"
            : endpoint.state == EndpointState::kPending ? "pending"
                                                        : "discovered";
        output << id << "  " << state
               << "  info_hex=" << Hex(ByteArray(endpoint.info)) << '\n';
      }
    }
    Print(output.str());
  }

  std::unique_ptr<ServiceControllerRouter> router_;
  std::unique_ptr<Core> core_;
  std::mutex output_mutex_;
  std::mutex state_mutex_;
  std::unordered_map<std::string, Endpoint> endpoints_;
  std::string device_info_storage_;
  bool stopped_ = false;
};

}  // namespace nearby::connections::cli

int main(int argc, char** argv) {
  using nearby::connections::cli::CommandLine;
  using nearby::connections::cli::g_interrupted;
  using nearby::connections::cli::NearbyCli;
  using nearby::connections::cli::ParseCommand;
  using nearby::connections::cli::ParseTokens;
  using nearby::connections::cli::PrintUsage;

  std::vector<std::string> commands;
  std::string script;
  bool repl = true;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      PrintUsage(std::cout, argv[0]);
      return 0;
    }
    if (arg == "--no-repl") {
      repl = false;
      continue;
    }
    auto read_value = [&](std::string_view name, std::string* value) {
      std::string prefix = std::string(name) + "=";
      if (arg == name && i + 1 < argc) {
        *value = argv[++i];
        return true;
      }
      if (arg.rfind(prefix, 0) == 0) {
        *value = arg.substr(prefix.size());
        return true;
      }
      return false;
    };
    std::string value;
    if (read_value("--execute", &value)) {
      commands.push_back(std::move(value));
    } else if (read_value("--script", &value)) {
      script = std::move(value);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      PrintUsage(std::cerr, argv[0]);
      return 2;
    }
  }

  struct sigaction action = {};
  action.sa_handler = nearby::connections::cli::SignalHandler;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);

  NearbyCli cli;
  auto execute_line = [&](const std::string& line) {
    CommandLine command;
    std::string error;
    if (!ParseCommand(line, &command, &error)) {
      std::cerr << "error: " << error << "\n";
      return true;
    }
    return cli.Execute(command);
  };
  for (const std::string& command : commands) {
    if (!execute_line(command)) return 0;
  }
  if (!script.empty()) {
    std::ifstream input(script);
    if (!input) {
      std::cerr << "Unable to open script: " << script << "\n";
      return 2;
    }
    std::string line;
    while (std::getline(input, line) && !g_interrupted) {
      if (!execute_line(line)) return 0;
    }
  }
  if (!repl) return 0;

  std::string line;
  while (!g_interrupted) {
    cli.PrintPrompt();
    if (!std::getline(std::cin, line)) break;
    if (!execute_line(line)) return 0;
  }
  return 0;
}
