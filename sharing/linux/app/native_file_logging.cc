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

#include "sharing/linux/app/native_file_logging.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <system_error>
#include <utility>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log_sink_registry.h"
#include "absl/strings/string_view.h"
#include "sharing/linux/platform/platform_util.h"

namespace nearby::sharing::linux {
namespace {

constexpr char kLogFileName[] = "quickshare.log";
constexpr char kLockFileName[] = ".quickshare.log.lock";

bool WriteAll(int fd, const char* data, size_t size) {
  while (size > 0) {
    const ssize_t written = write(fd, data, size);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    data += written;
    size -= static_cast<size_t>(written);
  }
  return true;
}

void WriteToStderr(absl::string_view message) {
  static_cast<void>(WriteAll(STDERR_FILENO, message.data(), message.size()));
}

}  // namespace

std::unique_ptr<NativeFileLogSink> NativeFileLogSink::Create(
    std::filesystem::path log_directory, size_t max_file_size, int file_count) {
  if (max_file_size == 0 || file_count < 1) {
    return nullptr;
  }

  std::error_code error;
  std::filesystem::create_directories(log_directory, error);
  if (error) {
    return nullptr;
  }
  std::filesystem::permissions(log_directory, std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace, error);
  if (error) {
    return nullptr;
  }

  const std::filesystem::path lock_path = log_directory / kLockFileName;
  const int lock_fd =
      open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (lock_fd < 0) {
    return nullptr;
  }
  if (fchmod(lock_fd, 0600) != 0) {
    close(lock_fd);
    return nullptr;
  }

  const std::filesystem::path log_path = log_directory / kLogFileName;
  const int log_fd =
      open(log_path.c_str(),
           O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (log_fd < 0 || fchmod(log_fd, 0600) != 0) {
    if (log_fd >= 0) {
      close(log_fd);
    }
    close(lock_fd);
    return nullptr;
  }
  close(log_fd);

  return std::unique_ptr<NativeFileLogSink>(new NativeFileLogSink(
      std::move(log_directory), lock_fd, max_file_size, file_count));
}

NativeFileLogSink::NativeFileLogSink(std::filesystem::path log_directory,
                                     int lock_fd, size_t max_file_size,
                                     int file_count)
    : log_path_(std::move(log_directory) / kLogFileName),
      lock_fd_(lock_fd),
      max_file_size_(max_file_size),
      file_count_(file_count) {}

NativeFileLogSink::~NativeFileLogSink() {
  close(lock_fd_);
}

void NativeFileLogSink::Send(const absl::LogEntry& entry) {
  const absl::string_view message =
      entry.text_message_with_prefix_and_newline();
  const bool flush = entry.log_severity() >= absl::LogSeverity::kError;

  std::lock_guard<std::mutex> lock(mutex_);
  if (flock(lock_fd_, LOCK_EX) != 0) {
    WriteFallback(message.data(), message.size());
    return;
  }

  const bool success = RotateIfNeeded(message.size()) &&
                       WriteRecord(message.data(), message.size(), flush);
  static_cast<void>(flock(lock_fd_, LOCK_UN));
  if (!success) {
    WriteFallback(message.data(), message.size());
  }
}

bool NativeFileLogSink::RotateIfNeeded(size_t incoming_size) {
  struct stat status{};
  if (stat(log_path_.c_str(), &status) != 0) {
    return errno == ENOENT;
  }
  const size_t current_size =
      status.st_size > 0 ? static_cast<size_t>(status.st_size) : 0;
  if (current_size == 0 || (incoming_size <= max_file_size_ &&
                            current_size <= max_file_size_ - incoming_size)) {
    return true;
  }

  for (int index = file_count_ - 1; index >= 1; --index) {
    const std::filesystem::path destination =
        log_path_.string() + "." + std::to_string(index);
    const std::filesystem::path source =
        index == 1 ? log_path_
                   : std::filesystem::path(log_path_.string() + "." +
                                           std::to_string(index - 1));
    if (rename(source.c_str(), destination.c_str()) != 0 && errno != ENOENT) {
      return false;
    }
  }
  return true;
}

bool NativeFileLogSink::WriteRecord(const char* data, size_t size, bool flush) {
  const int fd =
      open(log_path_.c_str(),
           O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) {
    return false;
  }

  bool success = fchmod(fd, 0600) == 0 && WriteAll(fd, data, size);
  if (success && flush) {
    success = fsync(fd) == 0;
  }
  if (close(fd) != 0) {
    success = false;
  }
  return success;
}

void NativeFileLogSink::WriteFallback(const char* data, size_t size) {
  WriteToStderr(absl::string_view(data, size));
}

std::unique_ptr<NativeFileLogging> NativeFileLogging::Initialize() {
  std::unique_ptr<NativeFileLogSink> sink =
      NativeFileLogSink::Create(internal::GetQuickShareLogPath().ToString());
  absl::InitializeLog();
  if (sink == nullptr) {
    WriteToStderr(
        "QuickShare: unable to initialize file logging; using "
        "stderr.\n");
    return nullptr;
  }

  absl::AddLogSink(sink.get());
  const absl::LogSeverityAtLeast previous_threshold = absl::StderrThreshold();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
  return std::unique_ptr<NativeFileLogging>(
      new NativeFileLogging(std::move(sink), previous_threshold));
}

NativeFileLogging::NativeFileLogging(
    std::unique_ptr<NativeFileLogSink> sink,
    absl::LogSeverityAtLeast previous_stderr_threshold)
    : sink_(std::move(sink)),
      previous_stderr_threshold_(previous_stderr_threshold) {}

NativeFileLogging::~NativeFileLogging() {
  absl::SetStderrThreshold(previous_stderr_threshold_);
  absl::RemoveLogSink(sink_.get());
}

}  // namespace nearby::sharing::linux
