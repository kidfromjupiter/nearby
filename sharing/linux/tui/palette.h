#pragma once

#include "ftxui/screen/color.hpp"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

struct Palette {
  inline static const Color primary =
      Color::RGB(0xF1, 0xF9, 0xF6);  // Off-white mint tint

  inline static const Color secondary =
      Color::RGB(0x8A, 0xAF, 0xA4);  // Muted sage gray

  inline static const Color accent =
      Color::RGB(0xA3, 0xD9, 0xC9);  // Original Mint Leaf

  inline static const Color active =
      Color::RGB(0x23, 0x3D, 0x34);  // Deep forest green tint

  inline static const Color base =
      Color::RGB(0x12, 0x1D, 0x1A);  // Near-black deep mint/charcoal
};

}  // namespace nearby::sharing::linux_tui
