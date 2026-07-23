#ifndef SHARING_LINUX_APP_NATIVE_FILE_LOGGING_H_
#define SHARING_LINUX_APP_NATIVE_FILE_LOGGING_H_

#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>

#include "absl/base/log_severity.h"
#include "absl/log/log_entry.h"
#include "absl/log/log_sink.h"

namespace nearby::sharing::linux {

class NativeFileLogSink final : public absl::LogSink {
 public:
  static constexpr size_t kDefaultMaxFileSize = 10 * 1024 * 1024;
  static constexpr int kDefaultFileCount = 5;

  static std::unique_ptr<NativeFileLogSink> Create(
      std::filesystem::path log_directory,
      size_t max_file_size = kDefaultMaxFileSize,
      int file_count = kDefaultFileCount);

  NativeFileLogSink(const NativeFileLogSink&) = delete;
  NativeFileLogSink& operator=(const NativeFileLogSink&) = delete;
  ~NativeFileLogSink() override;

  void Send(const absl::LogEntry& entry) override;

  const std::filesystem::path& log_path() const { return log_path_; }

 private:
  NativeFileLogSink(std::filesystem::path log_directory, int lock_fd,
                    size_t max_file_size, int file_count);

  bool RotateIfNeeded(size_t incoming_size);
  bool WriteRecord(const char* data, size_t size, bool flush);
  void WriteFallback(const char* data, size_t size);

  std::filesystem::path log_path_;
  int lock_fd_;
  size_t max_file_size_;
  int file_count_;
  std::mutex mutex_;
};

// Owns the process-wide Abseil sink registration. Initialize() must be called
// exactly once, before any application components start logging.
class NativeFileLogging final {
 public:
  static std::unique_ptr<NativeFileLogging> Initialize();

  NativeFileLogging(const NativeFileLogging&) = delete;
  NativeFileLogging& operator=(const NativeFileLogging&) = delete;
  ~NativeFileLogging();

 private:
  NativeFileLogging(std::unique_ptr<NativeFileLogSink> sink,
                    absl::LogSeverityAtLeast previous_stderr_threshold);

  std::unique_ptr<NativeFileLogSink> sink_;
  absl::LogSeverityAtLeast previous_stderr_threshold_;
};

}  // namespace nearby::sharing::linux

#endif  // SHARING_LINUX_APP_NATIVE_FILE_LOGGING_H_
