#include "sharing/linux/tui/app.h"

int main() {
  nearby::sharing::linux_tui::TuiApp app;
  return app.Run();
}
