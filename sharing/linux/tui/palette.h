#pragma once

#include "ftxui/screen/color.hpp"

namespace nearby::sharing::linux_tui {

struct Palette {
  inline static const ftxui::Color primary =
      ftxui::Color::RGB(0xF1, 0xF9, 0xF6);  // Off-white mint tint

  inline static const ftxui::Color secondary =
      ftxui::Color::RGB(0x8A, 0xAF, 0xA4);  // Muted sage gray

  inline static const ftxui::Color accent =
      ftxui::Color::RGB(0xA3, 0xD9, 0xC9);  // Original Mint Leaf

  inline static const ftxui::Color active =
      ftxui::Color::RGB(0x23, 0x3D, 0x34);  // Deep forest green tint

  inline static const ftxui::Color base =
      ftxui::Color::RGB(0x12, 0x1D, 0x1A);  // Near-black deep mint/charcoal
};

}  // namespace nearby::sharing::linux_tui
