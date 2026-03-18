// Copyright 2026

#include "internal/platform/system_clock.h"

#include <chrono>

#include "absl/time/clock.h"
#include "absl/time/time.h"

namespace nearby {

void SystemClock::Init() {}

absl::Time SystemClock::ElapsedRealtime() {
  return absl::FromUnixNanos(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

Exception SystemClock::Sleep(absl::Duration duration) {
  absl::SleepFor(duration);
  return {Exception::kSuccess};
}

}  // namespace nearby
