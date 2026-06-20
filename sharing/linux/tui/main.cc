#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/screen.hpp"
#include <iostream>

int main() {
  using namespace ftxui;
  auto document = text(L"Hello, FTXUI 6.1.9!") | border;
  auto screen = Screen::Create(Dimension::Fit(document));

  Render(screen, document);
  screen.Print();

  // Wait for user input before exiting.
  std::cout << '\n' << "Press Enter to exit..." << std::flush;
  std::cin.ignore(9999, '\n');
  std::cin.clear();

  return 0;
}
