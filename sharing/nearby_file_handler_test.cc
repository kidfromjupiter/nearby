// Copyright 2022 Google LLC
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

#include "sharing/nearby_file_handler.h"

#include <atomic>
#include <cstdio>
#include <vector>

#include "gtest/gtest.h"
#include "absl/synchronization/notification.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "internal/base/file_path.h"
#include "internal/base/files.h"
#include "sharing/internal/api/mock_sharing_platform.h"

namespace nearby {
namespace sharing {
namespace {
using ::nearby::sharing::api::MockSharingPlatform;

bool CreateFile(FilePath& file_path) {
  std::FILE* file = std::fopen(file_path.GetPath().c_str(), "w+");
  if (file == nullptr) {
    return false;
  }
  std::fclose(file);
  return true;
}

TEST(NearbyFileHandler, OpenFiles) {
  MockSharingPlatform mock_platform;
  NearbyFileHandler nearby_file_handler(mock_platform);
  absl::Notification notification;
  std::vector<NearbyFileHandler::FileInfo> result;
  FilePath test_file =
      GetTemporaryDirectory()->append(FilePath("nearby_nfh_test_abc.jpg"));

  ASSERT_TRUE(CreateFile(test_file));
  nearby_file_handler.OpenFiles(
      {test_file}, [&result, &notification](
                       std::vector<NearbyFileHandler::FileInfo> file_infos) {
        result = file_infos;
        notification.Notify();
      });

  notification.WaitForNotificationWithTimeout(absl::Seconds(1));
  EXPECT_EQ(result.size(), 1);
  ASSERT_TRUE(RemoveFile(test_file.GetPath()));
}

TEST(NearbyFileHandler, DeleteAFileFromDisk) {
  MockSharingPlatform mock_platform;
  NearbyFileHandler nearby_file_handler(mock_platform);
  FilePath test_file =
      GetTemporaryDirectory()->append(FilePath("nearby_nfh_test_abc.jpg"));
  ASSERT_TRUE(CreateFile(test_file));
  std::vector<FilePath> file_paths;
  file_paths.push_back(test_file);
  nearby_file_handler.DeleteFilesFromDisk(file_paths, []() {});
  ASSERT_TRUE(FileExists(test_file.GetPath()));
  absl::SleepFor(absl::Seconds(2));
  ASSERT_FALSE(FileExists(test_file.GetPath()));
}

TEST(NearbyFileHandler, DeleteMultipleFilesFromDisk) {
  MockSharingPlatform mock_platform;
  NearbyFileHandler nearby_file_handler(mock_platform);
  FilePath test_file =
      GetTemporaryDirectory()->append(FilePath("nearby_nfh_test_abc.jpg"));
  FilePath test_file2 =
      GetTemporaryDirectory()->append(FilePath("nearby_nfh_test_def.jpg"));
  FilePath test_file3 =
      GetTemporaryDirectory()->append(FilePath("nearby_nfh_test_ghi.jpg"));
  std::vector<FilePath> file_paths;
  file_paths = {test_file, test_file2, test_file3};
  // Check it doesn't throw an exception.
  nearby_file_handler.DeleteFilesFromDisk(file_paths, []() {});
  ASSERT_FALSE(FileExists(test_file.GetPath()));
  ASSERT_FALSE(FileExists(test_file2.GetPath()));
  ASSERT_FALSE(FileExists(test_file3.GetPath()));
  absl::SleepFor(absl::Seconds(2));
  ASSERT_FALSE(FileExists(test_file.GetPath()));
  ASSERT_FALSE(FileExists(test_file2.GetPath()));
  ASSERT_FALSE(FileExists(test_file3.GetPath()));
}

TEST(NearbyFileHandler, TestCallback) {
  MockSharingPlatform mock_platform;
  std::atomic_bool received_callback = false;
  NearbyFileHandler nearby_file_handler(mock_platform);
  FilePath test_file =
      GetTemporaryDirectory()->append(FilePath("nearby_nfh_test_abc.jpg"));
  ASSERT_TRUE(CreateFile(test_file));
  std::vector<FilePath> file_paths;
  file_paths.push_back(test_file);
  nearby_file_handler.DeleteFilesFromDisk(
      file_paths, [&received_callback]() { received_callback = true; });
  ASSERT_FALSE(received_callback);
  ASSERT_TRUE(FileExists(test_file.GetPath()));
  absl::SleepFor(absl::Seconds(2));
  ASSERT_TRUE(received_callback);
  ASSERT_FALSE(FileExists(test_file.GetPath()));
}

}  // namespace
}  // namespace sharing
}  // namespace nearby
