cmake_minimum_required(VERSION 3.15)
project(RTPWrapper)

# List of available backends
set(VALID_BACKENDS "mockRTP" "uvgRTP")


#include fetch content
include(FetchContent)


#Set an option to build examples (by default off)
option(BUILD_EXAMPLES "Build examples" OFF)

message(STATUS "RTP BACKEND is ${RTP_BACKEND}")


# Set the default backend to mockRTP if not parameter specified
if(NOT DEFINED RTP_BACKEND)
    set(RTP_BACKEND "mockRTP" CACHE STRING "Default RTP backend to use")
elseif(NOT RTP_BACKEND IN_LIST VALID_BACKENDS)
    message(FATAL_ERROR "RTP_BACKEND must be one of the following: ${VALID_BACKENDS}")
endif()

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Fetch library uvgRTP if RTP_BACKEND is uvgRTP
if(RTP_BACKEND STREQUAL "uvgRTP")
    FetchContent_Declare(
            uvgRTP
            GIT_REPOSITORY  git@github.com:ultravideo/uvgRTP.git
            GIT_TAG         master
    )
    FetchContent_MakeAvailable(uvgRTP)
    include_directories(${uvgrtp_SOURCE_DIR}/include)
    message (STATUS "uvgRTP include folder: " ${uvgrtp_SOURCE_DIR}/include)
    message (STATUS "uvgRTP library folder: " ${uvgrtp_BINARY_DIR})
endif()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)


# Include the selected backend's CMake file 
set(BACKEND_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/backends/${RTP_BACKEND}")

get_cmake_property(_variableNames VARIABLES)
list(SORT _variableNames)


if(LIST_DEBUG)
    foreach(_variableName ${_variableNames})
        message("${_variableName}=${${_variableName}}")
    endforeach()
endif()



if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()



