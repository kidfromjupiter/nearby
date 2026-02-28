cmake_minimum_required(VERSION 3.21)

project(nearby_qml_tray_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2)
include(GNUInstallDirs)

set(NEARBY_INSTALL_PREFIX "/usr/local" CACHE PATH "Prefix where Nearby facade header and shared library are installed")
set(NEARBY_INCLUDE_ROOT "${NEARBY_INSTALL_PREFIX}/include" CACHE PATH "Nearby include root")
set(NEARBY_LIBRARY_DIR "${NEARBY_INSTALL_PREFIX}/lib" CACHE PATH "Nearby shared library directory")

find_path(
  NEARBY_FACADE_INCLUDE_ROOT
  NAMES sharing/linux/nearby_connections_qt_facade.h
  HINTS "${NEARBY_INCLUDE_ROOT}"
)

find_library(
  NEARBY_CONNECTIONS_SHARED_LIB
  NAMES nearby_connections_service_linux_shared
  HINTS "${NEARBY_LIBRARY_DIR}"
)

if(NOT NEARBY_FACADE_INCLUDE_ROOT)
  message(FATAL_ERROR
    "Could not find sharing/linux/nearby_connections_qt_facade.h. "
    "Install it first (for example with sharing/linux/install_nearby_connections_service.sh) "
    "or set -DNEARBY_INCLUDE_ROOT=<prefix>/include.")
endif()

if(NOT NEARBY_CONNECTIONS_SHARED_LIB)
  message(FATAL_ERROR
    "Could not find libnearby_connections_service_linux_shared.so. "
    "Install it first (for example with sharing/linux/install_nearby_connections_service.sh) "
    "or set -DNEARBY_LIBRARY_DIR=<prefix>/lib.")
endif()

add_library(nearby_connections_service_linux_installed SHARED IMPORTED GLOBAL)
set_target_properties(
  nearby_connections_service_linux_installed
  PROPERTIES
    IMPORTED_LOCATION "${NEARBY_CONNECTIONS_SHARED_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${NEARBY_FACADE_INCLUDE_ROOT}"
)

qt_add_executable(nearby_qml_file_tray_app
  file_share_tray_main.cpp
  file_share_tray_controller.cc
  file_share_tray_controller.h
  resources_file_share.qrc
)

target_include_directories(nearby_qml_file_tray_app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}"
  "${NEARBY_FACADE_INCLUDE_ROOT}"
)
target_link_libraries(nearby_qml_file_tray_app PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
  nearby_connections_service_linux_installed
)

get_filename_component(NEARBY_CONNECTIONS_SHARED_LIB_DIR "${NEARBY_CONNECTIONS_SHARED_LIB}" DIRECTORY)

set_target_properties(nearby_qml_file_tray_app PROPERTIES
  BUILD_RPATH "${NEARBY_CONNECTIONS_SHARED_LIB_DIR}"
  INSTALL_RPATH "$ORIGIN"
)

install(TARGETS nearby_qml_file_tray_app
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES "${NEARBY_CONNECTIONS_SHARED_LIB}"
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# Convenience build target for the file tray app.
add_custom_target(build_file_tray_app
  DEPENDS nearby_qml_file_tray_app
)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "nearby_qml_tray_app")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
