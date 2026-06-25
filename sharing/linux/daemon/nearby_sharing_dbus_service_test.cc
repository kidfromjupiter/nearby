#include "sharing/linux/daemon/nearby_sharing_dbus_service.h"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "sharing/attachment_container.h"
#include "sharing/common/nearby_share_enums.h"
#include "sharing/share_target.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_metadata_builder.h"

namespace nearby::sharing::linux {
namespace {

ShareTarget MakeShareTarget() {
  ShareTarget target;
  target.id = 42;
  target.device_name = "Pixel";
  target.type = ShareTargetType::kPhone;
  target.is_incoming = true;
  target.is_known = true;
  target.device_id = "device-id";
  target.for_self_share = true;
  target.vendor_id = 7;
  target.receive_disabled = true;
  return target;
}

TEST(NearbySharingDbusServiceTest, ShareTargetToDbusMapsNearbyFields) {
  DbusDictionary result = ShareTargetToDbus(MakeShareTarget());

  EXPECT_EQ(result.at("id").get<int64_t>(), 42);
  EXPECT_EQ(result.at("device_name").get<std::string>(), "Pixel");
  EXPECT_EQ(result.at("type").get<int32_t>(),
            static_cast<int32_t>(ShareTargetType::kPhone));
  EXPECT_TRUE(result.at("is_incoming").get<bool>());
  EXPECT_TRUE(result.at("is_known").get<bool>());
  EXPECT_EQ(result.at("device_id").get<std::string>(), "device-id");
  EXPECT_TRUE(result.at("for_self_share").get<bool>());
  EXPECT_EQ(result.at("vendor_id").get<int32_t>(), 7);
  EXPECT_TRUE(result.at("receive_disabled").get<bool>());
}

TEST(NearbySharingDbusServiceTest, TransferMetadataToDbusMapsNearbyFields) {
  AttachmentContainer::Builder attachments_builder;
  auto attachments = attachments_builder.Build();
  TransferMetadata metadata =
      TransferMetadataBuilder()
          .set_status(TransferMetadata::Status::kInProgress)
          .set_progress(25)
          .set_token(std::string("1234"))
          .set_is_self_share(true)
          .set_transferred_bytes(1024)
          .set_transfer_speed(256)
          .set_estimated_time_remaining(4)
          .set_total_attachments_count(2)
          .set_transferred_attachments_count(1)
          .set_in_progress_attachment_id(99)
          .set_in_progress_attachment_transferred_bytes(512)
          .set_in_progress_attachment_total_bytes(2048)
          .set_binding_id("binding")
          .build();

  DbusDictionary result = TransferMetadataToDbus(metadata, *attachments);

  EXPECT_EQ(result.at("status").get<std::string>(), "kInProgress");
  EXPECT_EQ(result.at("progress").get<double>(), 25);
  EXPECT_EQ(result.at("transferred_bytes").get<int64_t>(), 1024);
  EXPECT_EQ(result.at("total_bytes").get<int64_t>(), 0);
  EXPECT_EQ(result.at("transfer_speed").get<int64_t>(), 256);
  EXPECT_EQ(result.at("estimated_time_remaining").get<int64_t>(), 4);
  EXPECT_EQ(result.at("total_attachments_count").get<int>(), 2);
  EXPECT_EQ(result.at("transferred_attachments_count").get<int>(), 1);
  EXPECT_FALSE(result.at("is_final_status").get<bool>());
  EXPECT_TRUE(result.at("is_self_share").get<bool>());
  EXPECT_EQ(result.at("binding_id").get<std::string>(), "binding");
  EXPECT_EQ(result.at("token").get<std::string>(), "1234");
  EXPECT_EQ(result.at("in_progress_attachment_id").get<int64_t>(), 99);
  EXPECT_EQ(
      result.at("in_progress_attachment_transferred_bytes").get<int64_t>(),
      512);
  EXPECT_EQ(result.at("in_progress_attachment_total_bytes").get<int64_t>(),
            2048);
}

}  // namespace
}  // namespace nearby::sharing::linux
