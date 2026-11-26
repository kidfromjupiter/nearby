// Copyright 2020 Google LLC
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

#ifndef PLATFORM_IMPL_LINUX_SCHEDULED_EXECUTOR_H_
#define PLATFORM_IMPL_LINUX_SCHEDULED_EXECUTOR_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "absl/time/time.h"
#include "internal/platform/implementation/cancelable.h"
#include "internal/platform/implementation/linux/executor.h"
#include "internal/platform/implementation/scheduled_executor.h"

namespace nearby {
namespace linux {

#define TIMER_NAME_BUFFER_SIZE 64

// An Executor that can schedule commands to run after a given delay, or to
// execute periodically.
//
// https://docs.oracle.com/javase/8/docs/api/java/util/concurrent/ScheduledExecutorService.html
class ScheduledExecutor : public api::ScheduledExecutor {
 public:
  ScheduledExecutor();

  ~ScheduledExecutor() override;

  // Cancelable is kept both in the executor context, and in the caller context.
  // We want Cancelable to live until both caller and executor are done with it.
  // Exclusive ownership model does not work for this case;
  // using std:shared_ptr<> instead if std::unique_ptr<>.
  std::shared_ptr<api::Cancelable> Schedule(Runnable&& runnable,
                                            absl::Duration duration) override;

  // Executes the runnable task immedately.
  bool Execute(Runnable&& runnable) override;

  // Shutdowns the executor, all scheduled task will be cancelled.
  void Shutdown() override;

 private:
  struct SchedulerState {
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<uint64_t> generation = 0;
    bool shut_down = false;
  };

  class ScheduledTask : public api::Cancelable {
   public:
    ScheduledTask(Runnable&& task,
                  std::chrono::steady_clock::time_point deadline,
                  uint64_t sequence,
                  std::weak_ptr<SchedulerState> scheduler_state)
        : task_(std::move(task)),
          deadline_(deadline),
          sequence_(sequence),
          scheduler_state_(std::move(scheduler_state)) {}

    bool Cancel() override;
    bool TryDispatch();
    bool IsCancelled() const;
    void Run();

    std::chrono::steady_clock::time_point deadline() const { return deadline_; }
    uint64_t sequence() const { return sequence_; }

   private:
    enum class State { kPending, kDispatched, kCancelled };

    Runnable task_;
    const std::chrono::steady_clock::time_point deadline_;
    const uint64_t sequence_;
    std::weak_ptr<SchedulerState> scheduler_state_;
    std::atomic<State> state_{State::kPending};
  };

  struct ScheduledTaskCompare {
    bool operator()(const std::shared_ptr<ScheduledTask>& lhs,
                    const std::shared_ptr<ScheduledTask>& rhs) const {
      if (lhs->deadline() == rhs->deadline()) {
        return lhs->sequence() > rhs->sequence();
      }
      return lhs->deadline() > rhs->deadline();
    }
  };

  void RunScheduler();

  std::unique_ptr<nearby::linux::Executor> executor_ = nullptr;
  std::shared_ptr<SchedulerState> scheduler_state_;
  std::priority_queue<std::shared_ptr<ScheduledTask>,
                      std::vector<std::shared_ptr<ScheduledTask>>,
                      ScheduledTaskCompare>
      scheduled_tasks_;
  std::thread scheduler_thread_;
  uint64_t next_sequence_ = 0;
  std::atomic_bool shut_down_ = false;
};

}  // namespace linux
}  // namespace nearby

#endif  // PLATFORM_IMPL_LINUX_SCHEDULED_EXECUTOR_H_
