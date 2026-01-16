cmake_minimum_required(VERSION 3.16)
project(dicom_reader CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(DCMTK REQUIRED)

file(GLOB SOURCES
     ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/src/*.cc
)

add_executable(dicom_reader ${SOURCES})
target_include_directories(dicom_reader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${DCMTK_INCLUDE_DIRS})

target_link_libraries(dicom_reader PRIVATE ${DCMTK_LIBRARIES})
