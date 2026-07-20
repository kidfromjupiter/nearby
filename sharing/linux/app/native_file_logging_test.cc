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

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "gtest/gtest.h"
#include "sharing/linux/platform/platform_util.h"

namespace nearby::sharing::linux {
namespace {

class TemporaryDirectory {
 public:
  TemporaryDirectory() {
    std::string path =
        (std::filesystem::temp_directory_path() /
         ("quickshare-logging-test-" + std::to_string(getpid()) + "-XXXXXX"))
            .string();
    path.push_back('\0');
    char* created = mkdtemp(path.data());
    EXPECT_NE(created, nullptr);
    if (created != nullptr) {
      path_ = created;
    }
  }

  ~TemporaryDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

class ScopedEnvironmentVariable {
 public:
  ScopedEnvironmentVariable(const char* name, std::optional<std::string> value)
      : name_(name) {
    const char* old_value = std::getenv(name);
    if (old_value != nullptr) {
      old_value_ = old_value;
    }
    if (value.has_value()) {
      setenv(name, value->c_str(), 1);
    } else {
      unsetenv(name);
    }
  }

  ~ScopedEnvironmentVariable() {
    if (old_value_.has_value()) {
      setenv(name_.c_str(), old_value_->c_str(), 1);
    } else {
      unsetenv(name_.c_str());
    }
  }

 private:
  std::string name_;
  std::optional<std::string> old_value_;
};

class CaptureSink final : public absl::LogSink {
 public:
  void Send(const absl::LogEntry& entry) override {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.emplace_back(entry.text_message_with_prefix_and_newline());
  }

  std::vector<std::string> records() {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_;
  }

 private:
  std::mutex mutex_;
  std::vector<std::string> records_;
};

void EnsureAbslLoggingInitialized() {
  static const bool initialized = [] {
    absl::InitializeLog();
    return true;
  }();
  static_cast<void>(initialized);
}

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  std::ostringstream contents;
  contents << stream.rdbuf();
  return contents.str();
}

int PermissionBits(const std::filesystem::path& path) {
  struct stat status{};
  EXPECT_EQ(stat(path.c_str(), &status), 0);
  return status.st_mode & 0777;
}

TEST(NativeFileLogSinkTest, PreservesOriginalAbseilRecord) {
  EnsureAbslLoggingInitialized();
  TemporaryDirectory temporary_directory;
  auto sink = NativeFileLogSink::Create(temporary_directory.path());
  ASSERT_NE(sink, nullptr);
  CaptureSink capture;
  absl::ScopedStderrThreshold suppress_stderr(
      absl::LogSeverityAtLeast::kInfinity);
  absl::AddLogSink(sink.get());
  absl::AddLogSink(&capture);

  LOG(INFO) << "native-record-sentinel";

  absl::RemoveLogSink(&capture);
  absl::RemoveLogSink(sink.get());
  const std::vector<std::string> records = capture.records();
  ASSERT_EQ(records.size(), size_t{1});
  EXPECT_EQ(ReadFile(sink->log_path()), records.front());
}

TEST(NativeFileLogSinkTest, RotatesAndRetainsFiveFiles) {
  EnsureAbslLoggingInitialized();
  TemporaryDirectory temporary_directory;
  auto sink = NativeFileLogSink::Create(temporary_directory.path(), 256, 5);
  ASSERT_NE(sink, nullptr);
  absl::ScopedStderrThreshold suppress_stderr(
      absl::LogSeverityAtLeast::kInfinity);
  absl::AddLogSink(sink.get());

  for (int index = 0; index < 8; ++index) {
    LOG(INFO) << "rotation-record-" << index << "-" << std::string(180, 'x');
  }

  absl::RemoveLogSink(sink.get());
  EXPECT_TRUE(std::filesystem::exists(sink->log_path()));
  for (int index = 1; index <= 4; ++index) {
    EXPECT_TRUE(std::filesystem::exists(sink->log_path().string() + "." +
                                        std::to_string(index)));
  }
  EXPECT_FALSE(std::filesystem::exists(sink->log_path().string() + ".5"));
}

TEST(NativeFileLogSinkTest, SerializesConcurrentRecords) {
  EnsureAbslLoggingInitialized();
  TemporaryDirectory temporary_directory;
  auto sink = NativeFileLogSink::Create(temporary_directory.path());
  ASSERT_NE(sink, nullptr);
  absl::ScopedStderrThreshold suppress_stderr(
      absl::LogSeverityAtLeast::kInfinity);
  absl::AddLogSink(sink.get());

  std::vector<std::thread> threads;
  for (int thread = 0; thread < 4; ++thread) {
    threads.emplace_back([thread] {
      for (int record = 0; record < 20; ++record) {
        LOG(INFO) << "concurrent-record-" << thread << "-" << record;
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }

  absl::RemoveLogSink(sink.get());
  const std::string contents = ReadFile(sink->log_path());
  size_t record_count = 0;
  size_t position = 0;
  while ((position = contents.find("concurrent-record-", position)) !=
         std::string::npos) {
    ++record_count;
    position += 18;
  }
  EXPECT_EQ(record_count, size_t{80});
}

TEST(NativeFileLogSinkTest, UsesPrivatePermissions) {
  TemporaryDirectory temporary_directory;
  const std::filesystem::path log_directory =
      temporary_directory.path() / "nested" / "logs";
  auto sink = NativeFileLogSink::Create(log_directory);
  ASSERT_NE(sink, nullptr);

  EXPECT_EQ(PermissionBits(log_directory), 0700);
  EXPECT_EQ(PermissionBits(sink->log_path()), 0600);
  EXPECT_EQ(PermissionBits(log_directory / ".quickshare.log.lock"), 0600);
}

TEST(NativeFileLogSinkTest, RejectsUnavailableDirectory) {
  TemporaryDirectory temporary_directory;
  const std::filesystem::path regular_file =
      temporary_directory.path() / "not-a-directory";
  {
    std::ofstream stream(regular_file);
    stream << "content";
  }

  EXPECT_EQ(NativeFileLogSink::Create(regular_file), nullptr);
}

TEST(LogPathTest, UsesAbsoluteXdgStateHome) {
  TemporaryDirectory temporary_directory;
  ScopedEnvironmentVariable state_home("XDG_STATE_HOME",
                                       temporary_directory.path().string());

  EXPECT_EQ(internal::GetQuickShareLogPath().ToString(),
            (temporary_directory.path() / "quickshare" / "logs").string());
}

TEST(LogPathTest, FallsBackToHomeForRelativeXdgStateHome) {
  TemporaryDirectory temporary_directory;
  ScopedEnvironmentVariable home("HOME", temporary_directory.path().string());
  ScopedEnvironmentVariable state_home("XDG_STATE_HOME", "relative/path");

  EXPECT_EQ(
      internal::GetQuickShareLogPath().ToString(),
      (temporary_directory.path() / ".local" / "state" / "quickshare" / "logs")
          .string());
}

TEST(LogPathTest, FallsBackToHomeWhenXdgStateHomeIsMissing) {
  TemporaryDirectory temporary_directory;
  ScopedEnvironmentVariable home("HOME", temporary_directory.path().string());
  ScopedEnvironmentVariable state_home("XDG_STATE_HOME", std::nullopt);

  EXPECT_EQ(
      internal::GetQuickShareLogPath().ToString(),
      (temporary_directory.path() / ".local" / "state" / "quickshare" / "logs")
          .string());
}

}  // namespace
}  // namespace nearby::sharing::linux
