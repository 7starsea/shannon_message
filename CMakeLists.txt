cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(shannon.message.libs)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(ShannonCommon)
shannon_init_compiler_system()

shannon_init_boost_env()
find_package(Boost 1.57 COMPONENTS thread system REQUIRED)

message(status "**Boost_LIBRARIES: ${Boost_LIBRARIES}")
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/shannon/message  ${CMAKE_BINARY_DIR}/shannon_message)


# # we have create library shannon_message and we will use it in demo
FindMyBuildLib(shannon_message_lib ${CMAKE_SOURCE_DIR} shannon_message)
# # include shannon_message headers
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ) 
find_package(Boost 1.57 COMPONENTS thread system date_time timer filesystem random program_options REQUIRED)
add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/shannon_message_demo  ${CMAKE_BINARY_DIR}/shannon_message_demo)



