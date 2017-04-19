cmake_minimum_required (VERSION 3.6)
project(third_party)
include(ExternalProject)

message(${CMAKE_SOURCE_DIR})
ExternalProject_Add(yaml-cpp
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/yaml-cpp"
  # FIXME: update to some release when there is one free of boost
  DOWNLOAD_COMMAND ""
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  CMAKE_ARGS -DYAML_CPP_BUILD_TOOLS:BOOL=OFF -DYAML_CPP_BUILD_CONTRIB:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)
