cmake_minimum_required(VERSION 3.21)

project(nearby_qml_tray_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2)
include(GNUInstallDirs)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
get_filename_component(REPO_ROOT "${REPO_ROOT}" ABSOLUTE)
set(NEARBY_INCLUDE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/nearby_include_root")
file(MAKE_DIRECTORY "${NEARBY_INCLUDE_ROOT}")

# Avoid exposing the full repo root as an include directory (which can make IDEs
# index unrelated trees like the Bazel 'external' symlink). We create a narrow
# include root that only links directories needed by headers used in this app.
set(NEARBY_INCLUDE_DIRS
  sharing
)
foreach(dir_name IN LISTS NEARBY_INCLUDE_DIRS)
  if(EXISTS "${REPO_ROOT}/${dir_name}")
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E create_symlink
              "${REPO_ROOT}/${dir_name}"
              "${NEARBY_INCLUDE_ROOT}/${dir_name}"
      RESULT_VARIABLE LINK_RESULT
      ERROR_VARIABLE LINK_ERROR
    )
    if(NOT LINK_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to create include symlink for ${dir_name}: ${LINK_ERROR}")
    endif()
  endif()
endforeach()

set(BAZEL_EXECUTABLE "bazel" CACHE STRING "Path to bazel executable")
set(
  BAZEL_NEARBY_TARGET
  "//sharing/linux:nearby_connections_service_linux_shared"
  CACHE STRING
  "Bazel target that produces a shared library for nearby_connections_service_linux"
)
set(BAZEL_BUILD_OPTIONS
  -s
  --check_visibility=false
  --spawn_strategy=standalone
  --verbose_failures
  --cxxopt=-std=c++20
  --host_cxxopt=-std=c++20
)
string(JOIN " " BAZEL_BUILD_OPTIONS_STRING ${BAZEL_BUILD_OPTIONS})

execute_process(
  COMMAND "${BAZEL_EXECUTABLE}" info bazel-bin
  WORKING_DIRECTORY "${REPO_ROOT}"
  RESULT_VARIABLE BAZEL_INFO_RESULT
  OUTPUT_VARIABLE BAZEL_BIN_DIR
  ERROR_VARIABLE BAZEL_INFO_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT BAZEL_INFO_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to query bazel-bin with '${BAZEL_EXECUTABLE} info bazel-bin': ${BAZEL_INFO_ERROR}")
endif()

set(BAZEL_NEARBY_SO "${BAZEL_BIN_DIR}/sharing/linux/libnearby_connections_service_linux_shared.so")
set(
  BAZEL_BUILD_IF_MISSING_SCRIPT
  "${CMAKE_CURRENT_LIST_DIR}/cmake/BuildNearbySoIfMissing.cmake"
)
set(BAZEL_REBUILD_INPUTS
  "${REPO_ROOT}/sharing/linux/BUILD"
  "${REPO_ROOT}/sharing/linux/nearby_connections_qt_facade.h"
  "${REPO_ROOT}/sharing/linux/nearby_connections_qt_facade.cc"
  "${REPO_ROOT}/sharing/linux/nearby_connections_service_linux.h"
  "${REPO_ROOT}/sharing/linux/nearby_connections_service_linux.cc"
  "${REPO_ROOT}/internal/platform/implementation/linux/crypto.cc"
  "${REPO_ROOT}/internal/platform/uuid.cc"
)

add_custom_target(
  bazel_nearby_connections_service_linux
  COMMAND "${CMAKE_COMMAND}"
          -DOUTPUT_SO=${BAZEL_NEARBY_SO}
          -DBAZEL_EXECUTABLE=${BAZEL_EXECUTABLE}
          -DBAZEL_TARGET=${BAZEL_NEARBY_TARGET}
          -DBAZEL_BUILD_OPTIONS=${BAZEL_BUILD_OPTIONS_STRING}
          -DREPO_ROOT=${REPO_ROOT}
          -DREBUILD_INPUTS=${BAZEL_REBUILD_INPUTS}
          -P "${BAZEL_BUILD_IF_MISSING_SCRIPT}"
  BYPRODUCTS "${BAZEL_NEARBY_SO}"
  COMMENT "Checking/building ${BAZEL_NEARBY_TARGET} with Bazel"
  VERBATIM
)

add_library(nearby_connections_service_linux_bazel SHARED IMPORTED GLOBAL)
set_target_properties(
  nearby_connections_service_linux_bazel
  PROPERTIES
    IMPORTED_LOCATION "${BAZEL_NEARBY_SO}"
    INTERFACE_INCLUDE_DIRECTORIES "${NEARBY_INCLUDE_ROOT}"
)
add_dependencies(nearby_connections_service_linux_bazel bazel_nearby_connections_service_linux)

qt_add_executable(nearby_qml_tray_app
  main.cpp
  nearby_tray_controller.cc
  nearby_tray_controller.h
  resources.qrc
)

target_include_directories(nearby_qml_tray_app PRIVATE
  "${NEARBY_INCLUDE_ROOT}"
)
target_link_libraries(nearby_qml_tray_app PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
  nearby_connections_service_linux_bazel
)
add_dependencies(nearby_qml_tray_app bazel_nearby_connections_service_linux)

set_target_properties(nearby_qml_tray_app PROPERTIES
  BUILD_RPATH "${BAZEL_BIN_DIR}/sharing/linux"
  INSTALL_RPATH "$ORIGIN"
)

install(TARGETS nearby_qml_tray_app
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES "${BAZEL_NEARBY_SO}"
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

qt_generate_deploy_qml_app_script(
  TARGET nearby_qml_tray_app
  OUTPUT_SCRIPT nearby_qml_tray_app_deploy_script
)
install(SCRIPT "${nearby_qml_tray_app_deploy_script}")

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "nearby_qml_tray_app")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
