
option(USE_MKL " should be using MKL instead of OpenBLAS" OFF)
option(USE_ATLAS " should be using ATLAS instead of OpenBLAS" OFF)

find_package(Threads REQUIRED)
#FIXME move kernels specific stuff somewhere else


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-rtti")

add_executable(tool main.cpp runtime.cpp kernel.cpp KernelsBlas.cpp KernelsCheck.cpp)

if(USE_MKL)
  # TODO
  # find_package(MKL REQUIRED)
  set_target_properties(tool PROPERTIES LINK_FLAGS "-L${MKLROOT}/lib/intel64 -Wl,--no-as-needed -lmkl_intel_ilp64 -lmkl_sequential -lmkl_core -lpthread -lm -ldl")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMKL_ILP64 -m64 -I${MKLROOT}/include")
elseif(USE_ATLAS)
    set_target_properties(tool PROPERTIES LINK_FLAGS "-L/home/pviroule/install-atlas/lib -lcblas -latlas -llapack")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/home/pviroule/install-atlas/include")
else()
  find_package(BLAS REQUIRED)
  find_package(LAPACK REQUIRED)
  target_link_libraries(tool ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
endif()

find_package(PAPI REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Hwloc 1.11 REQUIRED)



include_directories(${PROJECT_SOURCE_DIR}/include ${YAML_CPP_INCLUDE_DIR} ${PAPI_INCLUDE_DIRS})

add_executable(tool main.cpp runtime.cpp kernel.cpp KernelsBlas.cpp KernelsCheck.cpp KernelsBandwidth.cpp)

target_link_libraries(tool numa ${CMAKE_THREAD_LIBS_INIT} ${Hwloc_LIBRARIES} ${YAML_CPP_LIBRARIES} ${PAPI_LIBRARIES})
