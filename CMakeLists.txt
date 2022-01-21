
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(objmodea3)


add_library(NvStrip STATIC NvTriStrip.cpp NvTriStripObjects.cpp)

find_package(assimp REQUIRED)

#message(STATUS "Assimp status:")
#message(STATUS "    libraries: ${ASSIMP_LIBRARIES}")
add_subdirectory(proto)

#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
find_package(Protobuf REQUIRED)

add_executable(objmodea3 code.cpp)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(objmodea3 NvStrip ${ASSIMP_LIBRARIES} ${PROTOBUF_LIBRARIES} proto)

set_property(TARGET objmodea3 PROPERTY CXX_STANDARD 17)