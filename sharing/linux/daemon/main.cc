#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/base/file_path.h"
#include "internal/flags/nearby_flags.h"
#include "sharing/advertisement.h"
#include "sharing/attachment_container.h"
#include "sharing/common/nearby_share_enums.h"
#include "sharing/file_attachment.h"
#include "sharing/linux/platform/linux_sharing_platform.h"
#include "sharing/linux/nearby_noop_analytics_recorder.h"
#include "sharing/flags/generated/nearby_sharing_feature_flags.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/nearby_sharing_service_factory.h"
#include "sharing/nearby_sharing_settings.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"
#include "sharing/proto/enums.pb.h"


namespace nearby::sharing::linux {

class DiscoveryCallback final : public ShareTargetDiscoveredCallback {
 public:
  explicit DiscoveryCallback() {}

  void OnShareTargetDiscovered(const ShareTarget& share_target) override {
    std::cout << "discovered target=\"" << share_target.device_name
              << "\" id=" << share_target.id << std::endl;
    if (share_target.receive_disabled) {
      return;
    }
  }

  void OnShareTargetLost(const ShareTarget& share_target) override {
    std::cout << "lost target=\"" << share_target.device_name
              << "\" id=" << share_target.id << std::endl;
  }

  void OnShareTargetUpdated(const ShareTarget& share_target) override {
    std::cout << "updated target=\"" << share_target.device_name
              << "\" id=" << share_target.id << std::endl;
    OnShareTargetDiscovered(share_target);
  }

};
}

int main() {
  auto analytics_recorder = nearby::sharing::linux::NoOpAnalyticsRecorder();
    auto linux_platform = nearby::sharing::linux::LinuxSharingPlatform("LinuxShare");
    auto service_ = nearby::sharing::NearbySharingServiceFactory::GetInstance()->CreateSharingService(
        linux_platform, &analytics_recorder, /*event_logger=*/nullptr,
        /*supports_file_sync=*/false);
    if (service_ == nullptr) {
      std::cerr << "failed to create NearbySharingService" << std::endl;
      return 1;
    }

}
