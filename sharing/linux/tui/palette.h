#pragma once

#include "ftxui/screen/color.hpp"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

struct Palette {
  // === Core Typography & Brand ===
  // Primary light text for high-contrast readability against dark backgrounds
  inline static const Color primary = Color::RGB(0xF1, 0xF9, 0xF6); // Off-white mint tint

  // Secondary text, inactive states, or subtle accents
  inline static const Color secondary = Color::RGB(0x8A, 0xAF, 0xA4); // Muted sage gray

  // Main brand accent, focused element backgrounds, or prominent icons (Unchanged)
  inline static const Color accent = Color::RGB(0xA3, 0xD9, 0xC9); // Original Mint Leaf

  // === Surface & Structure ===
  // Active menu item highlights, selection cards, or focused component backgrounds
  inline static const Color active = Color::RGB(0x23, 0x3D, 0x34); // Deep forest green tint

  // Base application window background
  inline static const Color base = Color::RGB(0x12, 0x1D, 0x1A); // Near-black deep mint/charcoal

  // Explicit component surface background (e.g., sidebars, modals, or inactive cards)
  inline static const Color surface = Color::RGB(0x19, 0x2A, 0x25); // Mid-tone dark green

  // Standard UI borders, dividers, and subtle grid lines
  inline static const Color border = Color::RGB(0x32, 0x52, 0x47); // Defined slate green

  // Extremely muted text, placeholders, or disabled options
  inline static const Color disabled = Color::RGB(0x56, 0x73, 0x6B); // Ghostly sage green

  // === Functional / Status Elements ===
  // Critical errors, destructive actions, or alerts (toned down for dark mode)
  inline static const Color error = Color::RGB(0xE0, 0x7A, 0x7A); // Soft desaturated coral/red

  // Warnings, pending indicators, or high-priority notifications
  inline static const Color warning = Color::RGB(0xE6, 0xC2, 0x80); // Soft amber gold

  // Success messages, online badges, or completed operations
  inline static const Color success = Color::RGB(0x81, 0xC7, 0x9D); // Vibrant spring mint
};

}  // namespace nearby::sharing::linux_tui
