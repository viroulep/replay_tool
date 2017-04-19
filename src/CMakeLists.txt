
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(Threads)
#FIXME move kernels somewhere else
find_package(BLAS)
find_package(LAPACK)
add_executable(tool test.cpp runtime.cpp kblas.cpp)

target_link_libraries(tool ${CMAKE_THREAD_LIBS_INIT} ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
