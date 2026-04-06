#include "sharing/linux/nearby_connections_api.h"

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "internal/platform/pipe.h"

namespace nearby::sharing {
namespace {

TEST(NearbyConnectionsApiTest, PayloadFromBytesSetsFields) {
  NearbyConnectionsApi::Payload payload =
      NearbyConnectionsApi::Payload::FromBytes(42, {1, 2, 3});

  EXPECT_EQ(payload.id, 42);
  EXPECT_EQ(payload.type, NearbyConnectionsApi::PayloadType::kBytes);
  EXPECT_EQ(payload.bytes, (std::vector<uint8_t>{1, 2, 3}));
  EXPECT_TRUE(payload.stream_bytes.empty());
  EXPECT_TRUE(payload.file_path.empty());
}

TEST(NearbyConnectionsApiTest, PayloadFromFileSetsFields) {
  NearbyConnectionsApi::Payload payload =
      NearbyConnectionsApi::Payload::FromFile(7, "/tmp/test.txt", "tmp");

  EXPECT_EQ(payload.id, 7);
  EXPECT_EQ(payload.type, NearbyConnectionsApi::PayloadType::kFile);
  EXPECT_EQ(payload.file_path, "/tmp/test.txt");
  EXPECT_EQ(payload.parent_folder, "tmp");
  EXPECT_TRUE(payload.bytes.empty());
  EXPECT_TRUE(payload.stream_bytes.empty());
}

TEST(NearbyConnectionsApiTest, PayloadFromStreamSetsFields) {
  NearbyConnectionsApi::Payload payload =
      NearbyConnectionsApi::Payload::FromStream(9, {4, 5, 6});

  EXPECT_EQ(payload.id, 9);
  EXPECT_EQ(payload.type, NearbyConnectionsApi::PayloadType::kStream);
  EXPECT_EQ(payload.stream_bytes, (std::vector<uint8_t>{4, 5, 6}));
  EXPECT_TRUE(payload.bytes.empty());
  EXPECT_TRUE(payload.file_path.empty());
}

TEST(NearbyConnectionsApiTest, PayloadFromInputStreamSetsFields) {
  auto [input, output] = CreatePipe();
  (void)output;

  NearbyConnectionsApi::Payload payload =
      NearbyConnectionsApi::Payload::FromInputStream(
          10, std::shared_ptr<nearby::InputStream>(std::move(input)));

  EXPECT_EQ(payload.id, 10);
  EXPECT_EQ(payload.type, NearbyConnectionsApi::PayloadType::kStream);
  EXPECT_TRUE(payload.stream_bytes.empty());
  EXPECT_NE(payload.stream_input, nullptr);
  EXPECT_TRUE(payload.bytes.empty());
  EXPECT_TRUE(payload.file_path.empty());
}

TEST(NearbyConnectionsApiTest, StatusCodeToStringCoversRepresentativeValues) {
  EXPECT_EQ(NearbyConnectionsApi::StatusCodeToString(
                NearbyConnectionsApi::StatusCode::kSuccess),
            "Success");
  EXPECT_EQ(NearbyConnectionsApi::StatusCodeToString(
                NearbyConnectionsApi::StatusCode::kConnectionRejected),
            "ConnectionRejected");
  EXPECT_EQ(NearbyConnectionsApi::StatusCodeToString(
                NearbyConnectionsApi::StatusCode::kUnknown),
            "Unknown");
}

}  // namespace
}  // namespace nearby::sharing
