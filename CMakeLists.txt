cmake_minimum_required(VERSION 3.1)

project(rtt_dot_service)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OROCOS-RTT REQUIRED ${RTT_HINTS})
include(${OROCOS-RTT_USE_FILE_PATH}/UseOROCOS-RTT.cmake)

orocos_service(rtt_dot_service src/rtt_dot_service.cpp)
orocos_generate_package(
  DEPENDS_TARGETS rtt
)

