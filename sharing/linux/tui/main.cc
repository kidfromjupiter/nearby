#include "nearby_sharing_dbus_client.h"
#include <sdbus-c++/IConnection.h>

int main() {
  auto sharing =
      NearbySharingService(sdbus::ServiceName("com.google.nearby.sharing"),
                           sdbus::ObjectPath("/com/google/nearby/sharing"));

  sharing.StartReceive();
  sleep(10);
  sharing.StopReceive();
  //  nearby::sharing::linux_tui::TuiApp app;
  // return app.Run();
  return 0;
}
