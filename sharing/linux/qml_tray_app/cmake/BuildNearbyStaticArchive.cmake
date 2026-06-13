set(_output_static_lib "${OUTPUT_STATIC_LIB}")
set(_output_linkopts_file "${OUTPUT_LINKOPTS_FILE}")
set(_bazel_executable "${BAZEL_EXECUTABLE}")
set(_bazel_target "${BAZEL_TARGET}")
set(_bazel_build_options "${BAZEL_BUILD_OPTIONS}")
set(_bazel_env_cc "${BAZEL_ENV_CC}")
set(_bazel_env_cxx "${BAZEL_ENV_CXX}")
set(_repo_root "${REPO_ROOT}")

foreach(_var IN ITEMS _output_static_lib _output_linkopts_file _bazel_executable _bazel_target _bazel_build_options _bazel_env_cc _bazel_env_cxx _repo_root)
  string(REGEX REPLACE "^\"(.*)\"$" "\\1" ${_var} "${${_var}}")
endforeach()

if(EXISTS "${_output_static_lib}" AND EXISTS "${_output_linkopts_file}")
  message(STATUS "Using existing Bazel static archive: ${_output_static_lib}")
  return()
endif()

separate_arguments(_bazel_build_options_list NATIVE_COMMAND "${_bazel_build_options}")

message(STATUS "Building Bazel static archive ${_bazel_target}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
    "CC=${_bazel_env_cc}"
    "CXX=${_bazel_env_cxx}"
    "${_bazel_executable}" build ${_bazel_build_options_list} "${_bazel_target}"
  WORKING_DIRECTORY "${_repo_root}"
  RESULT_VARIABLE _bazel_build_result
)

if(NOT _bazel_build_result EQUAL 0)
  message(FATAL_ERROR "Bazel build failed for ${_bazel_target} (exit ${_bazel_build_result})")
endif()

if(NOT EXISTS "${_output_static_lib}")
  message(FATAL_ERROR "Expected static archive is missing: ${_output_static_lib}")
endif()

if(NOT EXISTS "${_output_linkopts_file}")
  message(FATAL_ERROR "Expected Bazel link options file is missing: ${_output_linkopts_file}")
endif()
