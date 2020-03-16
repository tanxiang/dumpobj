
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(objmodea3)


add_library(NvStrip STATIC NvTriStrip.cpp NvTriStripObjects.cpp)


find_package(Assimp REQUIRED)


message(STATUS "Assimp status:")
message(STATUS "    libraries: ${ASSIMP_LIBRARIES}")


add_executable(objmodea3 code.cpp)
target_link_libraries(objmodea3 NvStrip "${ASSIMP_LIBRARIES}")
set_property(TARGET objmodea3 PROPERTY CXX_STANDARD 17)