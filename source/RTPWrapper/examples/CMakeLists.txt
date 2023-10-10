cmake_minimum_required(VERSION 3.15)
project(Examples)

# Print Backend 

add_executable(${RTP_BACKEND}_example ${RTP_BACKEND}example.cpp)
target_sources(${RTP_BACKEND}_example PRIVATE ${BACKEND_DIRECTORY}/${RTP_BACKEND}impl.cpp)
target_include_directories(${RTP_BACKEND}_example PRIVATE ${BACKEND_DIRECTORY})

if (RTP_BACKEND STREQUAL "uvgRTP")
    target_link_libraries(${RTP_BACKEND}_example PRIVATE ${uvgrtp_BINARY_DIR}/libuvgrtp.a)
endif()

message(STATUS "RTP_BACKEND: ${RTP_BACKEND}")
message(STATUS "BACKENDS FOLDER ${BACKEND_DIRECTORY}")
message(STATUS "BACKENDS SOURCES ${BACKEND_DIRECTORY}/${RTP_BACKEND}impl.cpp")
