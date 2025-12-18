cmake_minimum_required(VERSION 3.16)
project(dicom_reader CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(DCMTK REQUIRED)

set(SOURCES src/RtPlanReader.cpp)

add_executable(dicom_reader src/main.cc src/RtPlanReader.cpp)
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(dicom_reader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${DCMTK_INCLUDE_DIRS})

target_link_libraries(dicom_reader PRIVATE ${DCMTK_LIBRARIES})
