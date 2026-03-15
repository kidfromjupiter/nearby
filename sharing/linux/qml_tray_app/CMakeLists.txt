cmake_minimum_required(VERSION 3.21)
project(nearby_qml_tray_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 DBus)

# Where the Bazel-built Nearby .so and header were installed
set(NEARBY_PREFIX "/usr/local" CACHE PATH "Install prefix for the Nearby shared library")

find_library(NEARBY_SHARING_LIB nearby_sharing_api_shared
  HINTS "${NEARBY_PREFIX}/lib" REQUIRED)

find_path(NEARBY_SHARING_INCLUDE sharing/linux/nearby_sharing_api.h
  HINTS "${NEARBY_PREFIX}/include" REQUIRED)

find_library(QRENCODE_LIB
  NAMES qrencode libqrencode.so.4
  HINTS /usr/lib64 /usr/lib /usr/local/lib
  REQUIRED)

qt_add_executable(nearby_qml_file_tray_app
  file_share_tray_main.cpp
  file_share_tray_controller.cc
  file_share_tray_controller.h
  notification_manager.cpp
  notification_manager.h
  third_party/libqrencode/qrencode_compat.h
  resources_file_share.qrc
)

target_include_directories(nearby_qml_file_tray_app PRIVATE "${NEARBY_SHARING_INCLUDE}")

target_link_libraries(nearby_qml_file_tray_app PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus
  "${QRENCODE_LIB}"
  "${NEARBY_SHARING_LIB}"
)

set_target_properties(nearby_qml_file_tray_app PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
)

include(GNUInstallDirs)

install(TARGETS nearby_qml_file_tray_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${NEARBY_SHARING_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
