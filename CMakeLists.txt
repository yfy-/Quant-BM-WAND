cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)
include(AppendCompilerFlags)
include(ExternalProject)

project(WAND CXX C)

INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/include
                    ${CMAKE_HOME_DIRECTORY}/external/fastpfor/headers/
                    ${CMAKE_BINARY_DIR}/external/sdsl-lite/include
                    ${CMAKE_BINARY_DIR}/external/sdsl-lite/external/libdivsufsort-2.0.1/include
                    )

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/external/sdsl-lite/lib)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(OPT "-O0")
  message("Debug")
else()
  set(OPT "-O3")
  message("Release")
endif()

append_cxx_compiler_flags("${OPT} -Wno-write-strings -msse4.2 -DNDEBUG -fforce-addr -fomit-frame-pointer -funroll-loops -frerun-cse-after-loop -frerun-loop-opt -march=native" "GCC" CMAKE_CXX_FLAGS)

ADD_SUBDIRECTORY(external/sdsl-lite)

ADD_LIBRARY(fastpfor_lib STATIC external/fastpfor/src/bitpacking.cpp
                                external/fastpfor/src/bitpackingaligned.cpp
                                external/fastpfor/src/bitpackingunaligned.cpp
                                external/fastpfor/src/simdunalignedbitpacking.cpp
                                external/fastpfor/src/simdbitpacking.cpp)

ADD_EXECUTABLE(search_index src/search_index.cpp src/compress_qmx.cpp src/lowerbound_threshold.cpp)

TARGET_LINK_LIBRARIES(search_index sdsl divsufsort divsufsort64 pthread fastpfor_lib)
