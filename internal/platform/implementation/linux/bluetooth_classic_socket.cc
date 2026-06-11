// Copyright 2023 Google LLC
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

#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>

#include "internal/platform/byte_array.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/linux/bluetooth_classic_socket.h"
#include "internal/platform/logging.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <algorithm>

namespace nearby {
namespace linux {

// This method blocks until input data is available, end of file is detected, or an exception is thrown.
ExceptionOr<ByteArray> BluetoothInputStream::Read(std::int64_t size) {
  // Sanity: avoid negative / zero sizes
  if (size <= 0) return ExceptionOr{ByteArray(std::string())};


  std::vector<char> buffer(size);

  // fd returned from bluez assumed to be stream type always

  pollfd pfds[1];
  pfds[0].fd = fd_raw_->get();
  pfds[0].events = POLLIN;
  ssize_t rcvd = 0;

  while (rcvd < size) {
    int r = poll(pfds, 1, -1);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      return Exception{Exception::kIo};
    }
    if (pfds[0].revents & POLLIN) {
        while (rcvd < size) {

          auto r = recv(fd_raw_->get(), buffer.data() + rcvd, size - rcvd, 0);
          if (r > 0) {
            rcvd += r;
            continue;
          }
          if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {continue;}

          LOG(ERROR) << __func__
                     << ": error reading from fd: " << std::strerror(errno);
          return {Exception::kIo};
        }
    }
  }


  return ExceptionOr{ByteArray(std::string(buffer.begin(), buffer.end()))};
}

Exception BluetoothInputStream::Close() {
  if (!fd_raw_->isValid()) return {Exception::kSuccess};
  fd_raw_ -> reset();
    return {Exception::kSuccess};
}

Exception BluetoothOutputStream::Write(absl::string_view data) {
  pollfd pfds[1];
  pfds[0].fd = fd_raw_->get();
  pfds[0].events = POLLOUT;
  ssize_t sent = 0;

  while (sent < data.size()) {
    int r = poll(pfds, 1, -1);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      return Exception{Exception::kIo};
    }
    if (pfds[0].revents & POLLOUT) {
      while (sent < data.size()) {
        auto r = send(fd_raw_->get(), data.data() + sent, data.size(), 0);
        if (r > 0) {
          sent += r;
          continue;
        }
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {continue;}

        LOG(ERROR) << __func__
                   << ": error reading from fd: " << std::strerror(errno);
        return {Exception::kIo};
      }
    }
  }
  return {Exception::kSuccess};
}

Exception BluetoothOutputStream::Close() {
  if (!fd_raw_->isValid()) return {Exception::kSuccess};
  fd_raw_ -> reset();
  return {Exception::kSuccess};
}

}  // namespace linux
}  // namespace nearby
