// Copyright 2020-2023 Google LLC
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

#ifndef PLATFORM_IMPL_LINUX_THREAD_POOL_H_
#define PLATFORM_IMPL_LINUX_THREAD_POOL_H_

#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <thread>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/runnable.h"

namespace nearby {
namespace linux {

class ThreadPool {
 public:
  virtual ~ThreadPool();
  static std::unique_ptr<ThreadPool> Create(int max_pool_size);

  // Runs a task on thread pool. The result indicates whether the task is put
  // into the thread pool.
  bool Run(Runnable task) ABSL_LOCKS_EXCLUDED(mutex_);

  // The thread pool is closed immediately if there is no outstanding work,
  // I/O, timer, or wait objects that are bound to the pool; otherwise, the
  // thread pool is released asynchronously after the outstanding objects are
  // freed.
  void ShutDown() ABSL_LOCKS_EXCLUDED(mutex_);

 private:
  ThreadPool(std::unique_ptr<std::vector<std::thread>> &thread_pool, int max_pool_size);

  void RunNextTask();

  // Starts each task and injects a function that removes the task when it is finished
  std::thread tasks_runner_;

  // Protects the access to tasks of the thread pool.
  mutable absl::Mutex mutex_;

  // The task queue of the thread pool. Thread pool will pick up task to run
  // when it is idle.
  std::queue<Runnable> tasks_ ABSL_GUARDED_BY(mutex_);

  // All the threads run in the pool
  std::unique_ptr<std::vector<std::thread>> thread_pool_ ABSL_GUARDED_BY(mutex_);

  // The maximum thread count in the thread pool
  int max_pool_size_ ABSL_GUARDED_BY(mutex_) = 0;

};

}  // namespace linux
}  // namespace nearby

#endif  //  PLATFORM_IMPL_LINUX_THREAD_POOL_H_
