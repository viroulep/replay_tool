cmake_minimum_required (VERSION 3.6)
project (ReplayTool)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# FIXME: find directory
message("Configuring and building third parties")

set(third_party_dir "${CMAKE_BINARY_DIR}/third_party")
file(MAKE_DIRECTORY ${third_party_dir})

message("${CMAKE_SOURCE_DIR}")


execute_process(
  WORKING_DIRECTORY "${third_party_dir}"
  COMMAND ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}/lib
)

execute_process(
  WORKING_DIRECTORY "${third_party_dir}"
  COMMAND ${CMAKE_COMMAND} --build .
)

message("done")


add_subdirectory(src)
