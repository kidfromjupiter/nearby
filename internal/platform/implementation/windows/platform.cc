// Copyright 2021 Google LLC
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

#include "internal/platform/implementation/platform.h"

#include <knownfolders.h>
#include <shlobj.h>
#include <windows.h>

#include <memory>
#include <sstream>
#include <string>

#include "internal/platform/implementation/shared/count_down_latch.h"
#include "internal/platform/implementation/shared/file.h"
#include "internal/platform/implementation/windows/atomic_boolean.h"
#include "internal/platform/implementation/windows/atomic_reference.h"
#include "internal/platform/implementation/windows/ble.h"
#include "internal/platform/implementation/windows/ble_v2.h"
#include "internal/platform/implementation/windows/bluetooth_adapter.h"
#include "internal/platform/implementation/windows/bluetooth_classic_medium.h"
#include "internal/platform/implementation/windows/cancelable.h"
#include "internal/platform/implementation/windows/condition_variable.h"
#include "internal/platform/implementation/windows/executor.h"
#include "internal/platform/implementation/windows/future.h"
#include "internal/platform/implementation/windows/listenable_future.h"
#include "internal/platform/implementation/windows/log_message.h"
#include "internal/platform/implementation/windows/mutex.h"
#include "internal/platform/implementation/windows/scheduled_executor.h"
#include "internal/platform/implementation/windows/server_sync.h"
#include "internal/platform/implementation/windows/settable_future.h"
#include "internal/platform/implementation/windows/submittable_executor.h"
#include "internal/platform/implementation/windows/webrtc.h"
#include "internal/platform/implementation/windows/wifi.h"
#include "internal/platform/implementation/windows/wifi_hotspot.h"
#include "internal/platform/implementation/windows/wifi_lan.h"

namespace location {
namespace nearby {
namespace api {

namespace {
std::string GetDownloadPathInternal(std::string& parent_folder,
                                    std::string& file_name) {
  PWSTR basePath;

  // Retrieves the full path of a known folder identified by the folder's
  // KNOWNFOLDERID.
  // https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
  SHGetKnownFolderPath(
      FOLDERID_Downloads,  //  rfid: A reference to the KNOWNFOLDERID that
                           //  identifies the folder.
      0,           // dwFlags: Flags that specify special retrieval options.
      NULL,        // hToken: An access token that represents a particular user.
      &basePath);  // ppszPath: When this method returns, contains the address
                   // of a pointer to a null-terminated Unicode string that
                   // specifies the path of the known folder. The calling
                   // process is responsible for freeing this resource once it
                   // is no longer needed by calling CoTaskMemFree, whether
                   // SHGetKnownFolderPath succeeds or not.
  size_t bufferSize;
  wcstombs_s(&bufferSize, NULL, 0, basePath, 0);
  std::string fullpathUTF8(bufferSize - 1, '\0');
  wcstombs_s(&bufferSize, fullpathUTF8.data(), bufferSize, basePath, _TRUNCATE);

  std::replace(fullpathUTF8.begin(), fullpathUTF8.end(), '\\', '/');

  // If parent_folder starts with a \\ or /, then strip it
  while (!parent_folder.empty() &&
         (*parent_folder.begin() == '\\' || *parent_folder.begin() == '/')) {
    parent_folder.erase(0, 1);
  }

  // If parent_folder ends with a \\ or /, then strip it
  while (!parent_folder.empty() &&
         (*parent_folder.rbegin() == '\\' || *parent_folder.rbegin() == '/')) {
    parent_folder.erase(parent_folder.size() - 1, 1);
  }

  // If file_name starts with a \\, then strip it
  while (!file_name.empty() &&
         (*file_name.begin() == '\\' || *file_name.begin() == '/')) {
    file_name.erase(0, 1);
  }

  // If file_name ends with a \\, then strip it
  while (!file_name.empty() &&
         (*file_name.rbegin() == '\\' || *file_name.rbegin() == '/')) {
    file_name.erase(file_name.size() - 1, 1);
  }

  CoTaskMemFree(basePath);

  std::stringstream path("");

  if (parent_folder.empty() && file_name.empty()) {
    return fullpathUTF8;
  }
  if (parent_folder.empty()) {
    path << fullpathUTF8.c_str() << "/" << file_name.c_str();
    std::string retVal = path.str();
    return retVal;
  }
  if (file_name.empty()) {
    path << fullpathUTF8.c_str() << "/" << parent_folder.c_str();
    std::string retVal = path.str();
    return retVal;
  }

  path << fullpathUTF8.c_str() << "/" << parent_folder.c_str() << "/"
       << file_name.c_str();
  std::string retVal = path.str();
  return retVal;
}

// If the file already exists we add " (x)", where x is an incrementing number,
// starting at 1, using the next non-existing number, to the file name, just
// before the first dot, or at the end if no dot. The absolute path is returned.
std::string CreateOutputFileWithRename(absl::string_view path) {
  auto last_separator = path.find_last_of('/');
  std::string folder(path.substr(0, last_separator));
  std::string file_name(path.substr(last_separator));

  int count = 0;

  // Locate the first dot
  auto first = file_name.find_first_of('.', 0);

  if (first == std::string::npos) {
    first = file_name.size();
  }

  // Break the string at the dot.
  auto file_name1 = file_name.substr(0, first);
  auto file_name2 = file_name.substr(first);

  // Construct the target file name
  std::string target(path);

  std::fstream file;
  file.open(target, std::fstream::binary | std::fstream::in);

  // While we successfully open the file, keep incrementing the count.
  while (!(file.rdstate() & std::ifstream::failbit)) {
    file.close();
    target = absl::StrCat(folder, file_name1, " (", ++count, ")", file_name2);
    file.clear();
    file.open(target, std::fstream::binary | std::fstream::in);
  }

  // The above leaves the file open, so close it.
  file.close();

  return target;
}

}  // namespace

std::string ImplementationPlatform::GetDownloadPath(std::string& parent_folder,
                                                    std::string& file_name) {
  return CreateOutputFileWithRename(
      GetDownloadPathInternal(parent_folder, file_name));
}

OSName ImplementationPlatform::GetCurrentOS() { return OSName::kWindows; }

std::unique_ptr<AtomicBoolean> ImplementationPlatform::CreateAtomicBoolean(
    bool initial_value) {
  return absl::make_unique<windows::AtomicBoolean>();
}

std::unique_ptr<AtomicUint32> ImplementationPlatform::CreateAtomicUint32(
    std::uint32_t value) {
  return absl::make_unique<windows::AtomicUint32>();
}

std::unique_ptr<CountDownLatch> ImplementationPlatform::CreateCountDownLatch(
    std::int32_t count) {
  return absl::make_unique<shared::CountDownLatch>(count);
}

std::unique_ptr<Mutex> ImplementationPlatform::CreateMutex(Mutex::Mode mode) {
  return absl::make_unique<windows::Mutex>(mode);
}

std::unique_ptr<ConditionVariable>
ImplementationPlatform::CreateConditionVariable(Mutex* mutex) {
  return absl::make_unique<windows::ConditionVariable>(mutex);
}

ABSL_DEPRECATED("This interface will be deleted in the near future.")
std::unique_ptr<InputFile> ImplementationPlatform::CreateInputFile(
    PayloadId payload_id, std::int64_t total_size) {
  std::string parent_folder("");
  std::string file_name(std::to_string(payload_id));
  return shared::IOFile::CreateInputFile(
      GetDownloadPath(parent_folder, file_name), total_size);
}

std::unique_ptr<InputFile> ImplementationPlatform::CreateInputFile(
    absl::string_view file_path, size_t size) {
  return shared::IOFile::CreateInputFile(file_path, size);
}

ABSL_DEPRECATED("This interface will be deleted in the near future.")
std::unique_ptr<OutputFile> ImplementationPlatform::CreateOutputFile(
    PayloadId payload_id) {
  std::string parent_folder("");
  std::string file_name(std::to_string(payload_id));
  return shared::IOFile::CreateOutputFile(
      GetDownloadPath(parent_folder, file_name));
}

std::unique_ptr<OutputFile> ImplementationPlatform::CreateOutputFile(
    absl::string_view file_path) {
  return shared::IOFile::CreateOutputFile(file_path);
}

// TODO(b/184975123): replace with real implementation.
std::unique_ptr<LogMessage> ImplementationPlatform::CreateLogMessage(
    const char* file, int line, LogMessage::Severity severity) {
  return absl::make_unique<windows::LogMessage>(file, line, severity);
}

std::unique_ptr<SubmittableExecutor>
ImplementationPlatform::CreateSingleThreadExecutor() {
  return absl::make_unique<windows::SubmittableExecutor>();
}

std::unique_ptr<SubmittableExecutor>
ImplementationPlatform::CreateMultiThreadExecutor(
    std::int32_t max_concurrency) {
  return absl::make_unique<windows::SubmittableExecutor>(max_concurrency);
}

std::unique_ptr<ScheduledExecutor>
ImplementationPlatform::CreateScheduledExecutor() {
  return absl::make_unique<windows::ScheduledExecutor>();
}

std::unique_ptr<BluetoothAdapter>
ImplementationPlatform::CreateBluetoothAdapter() {
  return absl::make_unique<windows::BluetoothAdapter>();
}

std::unique_ptr<BluetoothClassicMedium>
ImplementationPlatform::CreateBluetoothClassicMedium(
    nearby::api::BluetoothAdapter& adapter) {
  return absl::make_unique<windows::BluetoothClassicMedium>(adapter);
}

std::unique_ptr<BleMedium> ImplementationPlatform::CreateBleMedium(
    BluetoothAdapter& adapter) {
  return absl::make_unique<windows::BleMedium>(adapter);
}

// TODO(b/184975123): replace with real implementation.
std::unique_ptr<api::ble_v2::BleMedium>
ImplementationPlatform::CreateBleV2Medium(api::BluetoothAdapter& adapter) {
  return std::make_unique<windows::BleV2Medium>(adapter);
}

// TODO(b/184975123): replace with real implementation.
std::unique_ptr<ServerSyncMedium>
ImplementationPlatform::CreateServerSyncMedium() {
  return std::unique_ptr<windows::ServerSyncMedium>();
}

// TODO(b/184975123): replace with real implementation.
std::unique_ptr<WifiMedium> ImplementationPlatform::CreateWifiMedium() {
  return std::make_unique<windows::WifiMedium>();
}

std::unique_ptr<WifiLanMedium> ImplementationPlatform::CreateWifiLanMedium() {
  return absl::make_unique<windows::WifiLanMedium>();
}

std::unique_ptr<WifiHotspotMedium>
ImplementationPlatform::CreateWifiHotspotMedium() {
  return std::make_unique<windows::WifiHotspotMedium>();
}

// TODO(b/184975123): replace with real implementation.
std::unique_ptr<WebRtcMedium> ImplementationPlatform::CreateWebRtcMedium() {
  return absl::make_unique<windows::WebRtcMedium>();
}

}  // namespace api
}  // namespace nearby
}  // namespace location
