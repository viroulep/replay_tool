

find_package(Threads REQUIRED)
#FIXME move kernels specific stuff somewhere else
find_package(BLAS REQUIRED)
find_package(PAPI REQUIRED)
find_package(LAPACK REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Hwloc 1.11 REQUIRED)



include_directories(${PROJECT_SOURCE_DIR}/include ${YAML_CPP_INCLUDE_DIR} ${PAPI_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-rtti")

add_executable(tool main.cpp runtime.cpp kernel.cpp KernelsBlas.cpp KernelsCheck.cpp KernelsBandwidth.cpp)

target_link_libraries(tool numa ${CMAKE_THREAD_LIBS_INIT} ${Hwloc_LIBRARIES} ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES} ${YAML_CPP_LIBRARIES} ${PAPI_LIBRARIES})
