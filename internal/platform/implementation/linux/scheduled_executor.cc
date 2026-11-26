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

#include "internal/platform/implementation/linux/scheduled_executor.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <utility>

#include "absl/time/time.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {

ScheduledExecutor::ScheduledExecutor()
    : executor_(std::make_unique<nearby::linux::Executor>()),
      scheduler_state_(std::make_shared<SchedulerState>()),
      scheduler_thread_([this]() { RunScheduler(); }),
      shut_down_(false) {}

ScheduledExecutor::~ScheduledExecutor() {
  if (!shut_down_) {
    Shutdown();
  }
}

bool ScheduledExecutor::ScheduledTask::Cancel() {
  State expected = State::kPending;
  if (!state_.compare_exchange_strong(expected, State::kCancelled)) {
    return false;
  }
  if (std::shared_ptr<SchedulerState> state = scheduler_state_.lock()) {
    state->generation.fetch_add(1);
    state->condition.notify_one();
  }
  return true;
}

bool ScheduledExecutor::ScheduledTask::TryDispatch() {
  State expected = State::kPending;
  return state_.compare_exchange_strong(expected, State::kDispatched);
}

bool ScheduledExecutor::ScheduledTask::IsCancelled() const {
  return state_.load() == State::kCancelled;
}

void ScheduledExecutor::ScheduledTask::Run() {
  if (task_ != nullptr) {
    task_();
  }
}

// Cancelable is kept both in the executor context, and in the caller context.
// We want Cancelable to live until both caller and executor are done with it.
// Exclusive ownership model does not work for this case;
// using std:shared_ptr<> instead of std::unique_ptr<>.
std::shared_ptr<api::Cancelable> ScheduledExecutor::Schedule(
    Runnable&& runnable, absl::Duration duration) {
  if (shut_down_) {
    LOG(ERROR) << __func__ << ": Attempt to Schedule on a shut down executor.";

    return nullptr;
  }

  auto deadline = std::chrono::steady_clock::now();
  if (duration == absl::InfiniteDuration()) {
    deadline = std::chrono::steady_clock::time_point::max();
  } else if (duration > absl::ZeroDuration()) {
    deadline += absl::ToChronoNanoseconds(duration);
  }

  std::shared_ptr<ScheduledTask> task;
  {
    std::lock_guard<std::mutex> lock(scheduler_state_->mutex);
    if (scheduler_state_->shut_down) {
      return nullptr;
    }
    task = std::make_shared<ScheduledTask>(std::move(runnable), deadline,
                                           next_sequence_++, scheduler_state_);
    scheduled_tasks_.push(task);
    scheduler_state_->generation.fetch_add(1);
  }
  scheduler_state_->condition.notify_one();
  return task;
}

bool ScheduledExecutor::Execute(Runnable&& runnable) {
  if (shut_down_) {
    LOG(ERROR) << __func__ << ": Attempt to Execute on a shut down executor.";
    return false;
  }

  return executor_->Execute(std::move(runnable));
}

void ScheduledExecutor::Shutdown() {
  bool expected = false;
  if (shut_down_.compare_exchange_strong(expected, true)) {
    {
      std::lock_guard<std::mutex> lock(scheduler_state_->mutex);
      scheduler_state_->shut_down = true;
      while (!scheduled_tasks_.empty()) {
        scheduled_tasks_.top()->Cancel();
        scheduled_tasks_.pop();
      }
      scheduler_state_->generation.fetch_add(1);
    }
    scheduler_state_->condition.notify_all();
    if (scheduler_thread_.joinable()) {
      scheduler_thread_.join();
    }
    executor_->Shutdown();
    return;
  }
  LOG(ERROR) << __func__ << ": Attempt to Shutdown on a shut down executor.";
}

void ScheduledExecutor::RunScheduler() {
  while (true) {
    std::shared_ptr<ScheduledTask> task;
    {
      std::unique_lock<std::mutex> lock(scheduler_state_->mutex);
      while (!scheduler_state_->shut_down) {
        if (scheduled_tasks_.empty()) {
          scheduler_state_->condition.wait(lock);
          continue;
        }

        task = scheduled_tasks_.top();
        if (task->IsCancelled()) {
          scheduled_tasks_.pop();
          task.reset();
          continue;
        }

        if (task->deadline() > std::chrono::steady_clock::now()) {
          uint64_t generation = scheduler_state_->generation.load();
          scheduler_state_->condition.wait_until(
              lock, task->deadline(), [this, generation]() {
                return scheduler_state_->shut_down ||
                       scheduler_state_->generation.load() != generation;
              });
          task.reset();
          continue;
        }

        scheduled_tasks_.pop();
        if (!task->TryDispatch()) {
          task.reset();
          continue;
        }
        break;
      }

      if (scheduler_state_->shut_down) {
        return;
      }
    }

    executor_->Execute([task = std::move(task)]() { task->Run(); });
  }
}
}  // namespace linux
}  // namespace nearby
