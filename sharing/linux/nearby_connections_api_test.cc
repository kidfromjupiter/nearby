#include "sharing/linux/nearby_connections_api.h"

#include <vector>

#include "gtest/gtest.h"

namespace nearby::sharing {
namespace {

TEST(NearbyConnectionsApiTest, PayloadFromBytesSetsFields) {
  NearbyConnectionsApi::Payload payload =
      NearbyConnectionsApi::Payload::FromBytes(42, {1, 2, 3});

  EXPECT_EQ(payload.id, 42);
  EXPECT_EQ(payload.type, NearbyConnectionsApi::PayloadType::kBytes);
  EXPECT_EQ(payload.bytes, (std::vector<uint8_t>{1, 2, 3}));
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
