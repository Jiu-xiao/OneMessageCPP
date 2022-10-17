cmake_minimum_required(VERSION 3.11)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/om/src)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

target_sources(OneMessage PUBLIC ${${PROJECT_NAME}_SOURCES})

target_include_directories(
  OneMessage
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

project(OneMessageCPP)
