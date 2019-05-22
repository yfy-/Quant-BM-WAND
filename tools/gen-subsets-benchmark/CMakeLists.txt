cmake_minimum_required(VERSION 2.8)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(OPT "-O0")
  message("Debug")
else()
  set(OPT "-O3")
  message("Release")
endif()

set(CMAKE_CXX_FLAGS "${OPT} -std=c++11 -Wno-write-strings -msse4.2 -DNDEBUG -fforce-addr -fomit-frame-pointer -funroll-loops -frerun-cse-after-loop -frerun-loop-opt -march=native")

add_executable(benchmark benchmark.cpp)
