set(_output_so "${OUTPUT_SO}")
set(_bazel_executable "${BAZEL_EXECUTABLE}")
set(_bazel_target "${BAZEL_TARGET}")
set(_bazel_build_options "${BAZEL_BUILD_OPTIONS}")
set(_repo_root "${REPO_ROOT}")
set(_rebuild_inputs "${REBUILD_INPUTS}")

# Values passed via -D can arrive wrapped in literal quotes when emitted from
# a custom command. Strip one outer quote pair if present.
foreach(_var IN ITEMS _output_so _bazel_executable _bazel_target _bazel_build_options _repo_root)
  string(REGEX REPLACE "^\"(.*)\"$" "\\1" ${_var} "${${_var}}")
endforeach()

set(_needs_rebuild TRUE)
if(EXISTS "${_output_so}")
  # Rebuild if any tracked input file is newer than the output.
  set(_needs_rebuild FALSE)
  foreach(_input IN LISTS _rebuild_inputs)
    if(EXISTS "${_input}")
      if("${_input}" IS_NEWER_THAN "${_output_so}")
        set(_needs_rebuild TRUE)
        message(STATUS "Input changed since last Bazel build: ${_input}")
        break()
      endif()
    endif()
  endforeach()

  if(NOT _needs_rebuild)
  # Reuse an existing .so when it already exports the Qt facade symbols.
  # This avoids stale-cache link failures after facade changes.
    find_program(_nm_program nm)
    if(_nm_program)
      execute_process(
        COMMAND "${_nm_program}" -D -C "${_output_so}"
        RESULT_VARIABLE _nm_result
        OUTPUT_VARIABLE _nm_output
        ERROR_QUIET
      )
      if(_nm_result EQUAL 0)
        string(FIND "${_nm_output}" "nearby::sharing::linux::NearbyConnectionsQtFacade::NearbyConnectionsQtFacade()" _facade_ctor_idx)
        string(FIND "${_nm_output}" " U nearby::api::ImplementationPlatform::CreateScheduledExecutor()" _undef_platform_idx)
        string(FIND "${_nm_output}" " U nearby::SystemClock::ElapsedRealtime()" _undef_clock_idx)
        string(FIND "${_nm_output}" " U nearby::Crypto::Sha256(" _undef_crypto_idx)
        if(NOT _facade_ctor_idx EQUAL -1
           AND _undef_platform_idx EQUAL -1
           AND _undef_clock_idx EQUAL -1
           AND _undef_crypto_idx EQUAL -1)
          set(_needs_rebuild FALSE)
        else()
          set(_needs_rebuild TRUE)
        endif()
      else()
        set(_needs_rebuild TRUE)
      endif()
    endif()
  endif()

  if(NOT _needs_rebuild)
    message(STATUS "Using existing Bazel library: ${_output_so}")
    return()
  endif()
  message(STATUS "Existing Bazel library is stale/incompatible, rebuilding: ${_output_so}")
endif()

separate_arguments(_bazel_build_options_list NATIVE_COMMAND "${_bazel_build_options}")

message(STATUS "Bazel library not found, building ${_bazel_target}")
execute_process(
  COMMAND "${_bazel_executable}" build ${_bazel_build_options_list} "${_bazel_target}"
  WORKING_DIRECTORY "${_repo_root}"
  RESULT_VARIABLE BAZEL_BUILD_RESULT
)

if(NOT BAZEL_BUILD_RESULT EQUAL 0)
  message(FATAL_ERROR "Bazel build failed for ${_bazel_target} (exit ${BAZEL_BUILD_RESULT})")
endif()

if(NOT EXISTS "${_output_so}")
  message(FATAL_ERROR "Bazel build completed but expected output is missing: ${_output_so}")
endif()
