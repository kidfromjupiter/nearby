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

#ifndef LOCATION_NEARBY_ANALYTICS_CPP_LOGGING_EVENT_LOGGER_H_
#define LOCATION_NEARBY_ANALYTICS_CPP_LOGGING_EVENT_LOGGER_H_

namespace location::nearby::analytics::proto {
class ConnectionsLog;
}  // namespace location::nearby::analytics::proto

namespace nearby::sharing::analytics::proto {
class SharingLog;
}  // namespace nearby::sharing::analytics::proto

namespace nearby::analytics {

class EventLogger {
 public:
  virtual ~EventLogger() = default;

  virtual void Log(
      const location::nearby::analytics::proto::ConnectionsLog& message) = 0;
  virtual void Log(
      const nearby::sharing::analytics::proto::SharingLog& message) = 0;
};

}  // namespace nearby::analytics

#endif  // LOCATION_NEARBY_ANALYTICS_CPP_LOGGING_EVENT_LOGGER_H_

