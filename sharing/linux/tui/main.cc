#include <unistd.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include <string>

using namespace ftxui;

std::string OpenZenityFilePicker() {
  FILE* pipe = popen("zenity --file-selection 2>/dev/null", "r");
  if (!pipe) {
    return {};
  }

  char buffer[4096];
  std::string result;

  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result = buffer;
  }

  pclose(pipe);

  // Remove trailing newline.
  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }

  return result;
}

enum class Page {
  FilePicker,
  FileSelected,
};

int main() {
  auto screen = ScreenInteractive::Fullscreen();
  screen.TrackMouse(true);

  Page current_screen = Page::FilePicker;
  auto title_color = Color::RGB(0xab, 0xda, 0xc4);

  // clang-format off
  auto title = vbox({
    text("┏━┓╻ ╻╻┏━╸╻┏    ┏━┓╻ ╻┏━┓┏━┓┏━╸") | color(title_color),
    text("┃┓┃┃ ┃┃┃  ┣┻┓   ┗━┓┣━┫┣━┫┣┳┛┣╸ ") | color(title_color),
    text("┗┻┛┗━┛╹┗━╸╹ ╹   ┗━┛╹ ╹╹ ╹╹┗╸┗━╸") | color(title_color),
  });

  auto file_icon = vbox({
      text("    __________ ") | center | color(title_color),
      text("   / |       | ") | center | color(title_color),
      text("  /__|       | ") | center | color(title_color),
      text(" | --------- | ") | center | color(title_color),
      text(" | --------- | ") | center | color(title_color),
      text(" | --------- | ") | center | color(title_color),
      text(" | --------- | ") | center | color(title_color),
      text(" |___________| ") | center | color(title_color),
  });
  // clang-format on

  char hostname[256] = {};
  gethostname(hostname, sizeof(hostname));

  bool picker_hovered = false;
  std::string selected_file;

  Box picker_box;
  auto RenderFilePickerScreen = [&] {
    auto picker_card =
        vbox({
            file_icon,
            separatorEmpty(),
            text("Click to open file picker") | bold | center,
            separatorEmpty(),
            text("No file selected") | dim | center,
        }) |
        borderStyled(picker_hovered ? Color::White : title_color) |
        size(WIDTH, GREATER_THAN, 38) | size(HEIGHT, EQUAL, 15) |
        reflect(picker_box);

    return vbox({
               filler(),
               hbox({
                   filler(),
                   picker_card,
                   filler(),
               }),
               filler(),
           }) |
           flex;
  };

  auto RenderFileSelectedScreen = [&] {
    return vbox({
               text("File selected") | bold | center | color(title_color),
               separator(),
               paragraph(selected_file) | center,
               separatorEmpty(),
               text("Press Backspace to choose another file") | dim | center,
           }) |
           borderStyled(title_color) | flex;
  };

  auto component = Renderer([&] {
    auto visible_as_box =
        window(text(" Visible as "),
               vbox({
                   text(""),
                   text(std::string(hostname)) | bold | center,
                   text(""),
               })) |
        size(WIDTH, GREATER_THAN, 24) | size(HEIGHT, EQUAL, 5);
    auto main_panel = current_screen == Page::FilePicker
                          ? RenderFilePickerScreen()
                          : RenderFileSelectedScreen();
    auto picker_card =
        vbox({
            file_icon,
            separatorEmpty(),
            text("Click to open file picker") | bold | center,
            separatorEmpty(),
            paragraph(selected_file.empty() ? "No file selected"
                                            : selected_file) |
                dim | center,
        }) |
        borderStyled(picker_hovered ? Color::White : title_color) |
        size(WIDTH, GREATER_THAN, 38) | size(HEIGHT, EQUAL, 15) |
        reflect(picker_box);

    return vbox({
               title,
               separator(),
               hbox({
                   vbox({
                       visible_as_box,
                   }),

                   main_panel | flex,
               }) | flex,
           }) |
           size(WIDTH, GREATER_THAN, 80) | size(HEIGHT, GREATER_THAN, 24);
  });

  auto app = CatchEvent(component, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }

    if (!event.is_mouse()) {
      return false;
    }

    auto mouse = event.mouse();

    bool inside_picker =
        mouse.x >= picker_box.x_min && mouse.x <= picker_box.x_max &&
        mouse.y >= picker_box.y_min && mouse.y <= picker_box.y_max;

    picker_hovered = inside_picker;

    if (inside_picker && mouse.button == Mouse::Left &&
        mouse.motion == Mouse::Pressed) {
      std::string path = OpenZenityFilePicker();

      if (!path.empty()) {
        selected_file = path;
        current_screen = Page::FileSelected;
      }

      return true;
    }

    return false;
  });

  screen.Loop(app);

  return 0;
}
