# EvanScore's host and dock changes apply to the pinned Muse framework checkout.
# Reject drift rather than dropping part of the playback integration.
find_package(Git REQUIRED)
set(_evan_patch "${CMAKE_CURRENT_LIST_DIR}/../patches/evanscore-vdl-framework.patch")
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${_evan_patch}"
    WORKING_DIRECTORY "${MUSE_FRAMEWORK_PATH}" RESULT_VARIABLE _evan_already
    OUTPUT_QUIET ERROR_QUIET)
if(NOT _evan_already EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check "${_evan_patch}"
        WORKING_DIRECTORY "${MUSE_FRAMEWORK_PATH}" RESULT_VARIABLE _evan_check ERROR_VARIABLE _evan_error)
    if(NOT _evan_check EQUAL 0)
        message(FATAL_ERROR "EvanScore framework patch does not match the dependency: ${_evan_error}")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply "${_evan_patch}"
        WORKING_DIRECTORY "${MUSE_FRAMEWORK_PATH}" RESULT_VARIABLE _evan_apply ERROR_VARIABLE _evan_error)
    if(NOT _evan_apply EQUAL 0)
        message(FATAL_ERROR "Could not apply EvanScore framework patch: ${_evan_error}")
    endif()
endif()
