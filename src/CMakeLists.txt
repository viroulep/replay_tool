

find_package(Threads REQUIRED)
#FIXME move kernels somewhere else
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(yaml-cpp REQUIRED)


include_directories(${PROJECT_SOURCE_DIR}/include ${YAML_CPP_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(tool test.cpp runtime.cpp kblas.cpp)

target_link_libraries(tool numa ${CMAKE_THREAD_LIBS_INIT} ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES} ${YAML_CPP_LIBRARIES})
