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

#include "internal/platform/implementation/linux/mutex.h"

#include <chrono>
#include <future>

#include "gtest/gtest.h"

TEST(Mutex, AllowsRecursiveLocking) {
  nearby::linux::Mutex mutex;
  mutex.Lock();
  mutex.Lock();
  mutex.Unlock();
  mutex.Unlock();
}

TEST(Mutex, WaitsForOtherThreadToUnlock) {
  nearby::linux::Mutex mutex;
  std::promise<void> started;
  auto started_future = started.get_future();
  mutex.Lock();
  auto waiter = std::async(std::launch::async, [&]() {
    started.set_value();
    mutex.Lock();
    mutex.Unlock();
    return true;
  });
  started_future.wait();
  EXPECT_EQ(waiter.wait_for(std::chrono::milliseconds(0)),
            std::future_status::timeout);
  mutex.Unlock();
  EXPECT_TRUE(waiter.get());
}
