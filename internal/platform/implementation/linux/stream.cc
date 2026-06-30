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
#include <cerrno>
#include <cstdint>
#include <cstring>

#include "absl/strings/escaping.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/linux/stream.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {

ExceptionOr<ByteArray> InputStream::Read(std::int64_t size) {
  if (size <= 0) {
    return ExceptionOr<ByteArray>(ByteArray(std::string()));
  }

  if (closed_ || fd_ < 0) {
    return {Exception::kIo};
  }

  std::string buffer;
  buffer.resize(size);

  while (true) {
    pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

    int poll_result = poll(&pfd, 1, -1);

    if (poll_result < 0) {
      if (errno == EINTR) {
        continue;
      }

      LOG(ERROR) << __func__ << ": poll failed: " << std::strerror(errno);
      return {Exception::kIo};
    }

    if (pfd.revents & POLLNVAL || pfd.revents & POLLERR) {
      LOG(ERROR) << __func__ << ": Error reading from BluetoothSocket: "
                 << std::strerror(errno);
      return {Exception::kIo};
    }

    if (pfd.revents & (POLLIN | POLLHUP)) {
      ssize_t bytes_read = recv(fd_, buffer.data(), buffer.size(), 0);

      if (bytes_read > 0) {
        buffer.resize(static_cast<std::size_t>(bytes_read));
        return ExceptionOr<ByteArray>(ByteArray(std::move(buffer)));
      }

      if (bytes_read == 0) {
        // EOF / peer closed.
        return ExceptionOr<ByteArray>(ByteArray(std::string()));
      }

      if (errno == EINTR) {
        continue;
      }

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Non-blocking fd had no data by the time recv() ran.
        // Go back to poll.
        continue;
      }

      LOG(ERROR) << __func__ << ": recv failed: " << std::strerror(errno);
      return {Exception::kIo};
    }
  }
}

Exception InputStream::Close() {
  if (closed_ || fd_ < 0) return Exception{Exception::kSuccess};
  closed_ = true;
  shutdown(fd_, SHUT_RD);
  return Exception{Exception::kSuccess};
}

Exception OutputStream::Write(absl::string_view data) {
  if (closed_ || fd_ < 0) {
    return {Exception::kIo};
  }

  const int fd = fd_;
  size_t sent = 0;

  while (sent < data.size()) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLOUT;

    int poll_result;
    do {
      poll_result = poll(&pfd, 1, -1);
    } while (poll_result < 0 && errno == EINTR);

    if (poll_result < 0) {
      LOG(ERROR) << __func__ << ": poll failed: " << std::strerror(errno);
      return {Exception::kIo};
    }

    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
      LOG(ERROR) << __func__
                 << ": fd error/hangup during write, revents=" << pfd.revents;
      return {Exception::kIo};
    }

    if (!(pfd.revents & POLLOUT)) {
      continue;
    }

    ssize_t n = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);

    if (n > 0) {
      sent += static_cast<size_t>(n);
      continue;
    }

    if (n == 0) {
      LOG(ERROR) << __func__ << ": send returned 0";
      return {Exception::kIo};
    }

    if (errno == EINTR) {
      continue;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Socket became not writable after poll said it was writable.
      // Normal for non-blocking FDs. Go back to poll().
      continue;
    }

    LOG(ERROR) << __func__ << ": error writing to fd: " << std::strerror(errno);
    return {Exception::kIo};
  }

  return {Exception::kSuccess};
}

Exception OutputStream::Flush() {
  return Exception{Exception::kSuccess};
}

Exception OutputStream::Close() {
  if (closed_ || fd_ < 0) return Exception{Exception::kSuccess};
  closed_ = true;
  shutdown(fd_, SHUT_WR);
  return Exception{Exception::kSuccess};
}

}  // namespace linux
}  // namespace nearby
