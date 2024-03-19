cmake_minimum_required(VERSION 3.24.1)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)
include (FetchContent)

set(PROJECT_NAME "AudioStreamPlugin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build for arm64")
if (APPLE)
    message(STATUS "Apple detected")
    message(STATUS "System ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Processor ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "Cmake Version ${CMAKE_SYSTEM_VERSION}")

    if (CMAKE_OSX_ARCHITECTURES)
        message(STATUS "CMAKE_OSX_ARCHITECTURES is set to ${CMAKE_OSX_ARCHITECTURES}")
    elseif(CMAKE_SYSTEM_PROCESSOR)
        message(STATUS "CMAKE_OSX_ARCHITECTURES is not set defaulting to ${CMAKE_SYSTEM_PROCESSOR}")
        set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR})
    else()
        messsage(STATUS "CMAKE_OSX_ARCHITECTURES and CMAKE_SYSTEM_PROCESSOR are not set defaulting to arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}")
    set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}")

else()
    message(STATUS "Not Apple")
endif()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
if (NOT DEFINED RTP_BACKEND)
    set(RTP_BACKEND "udpRTP")
endif()
if (RTP_BACKEND STREQUAL "udpRTP")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DudpRTP")
elseif (RTP_BACKEND STREQUAL "uvgrtp")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Duvgrtp")
else()
    message(FATAL_ERROR "RTP_BACKEND must be either udpRTP or uvgrtp")
endif()


set(PRODUCT_NAME "DPlugin")
set(COMPANY_NAME "Dawn Audio")
set(BUNDLE_ID "com.dawnaudio.audiostreamplugin")

# Change me! Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone


set(FORMATS VST3)
option(NEEDS_STANDALONE "Build Standalone" OFF)
if (NEEDS_STANDALONE)
    list(APPEND FORMATS Standalone)
endif()

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# This adds the melatonin inspector module
# add_subdirectory (modules/melatonin_inspector)
# add_subdirectory (source/RTPWrapper)
# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
        # Icons for the standalone app
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

        # Change me!
        COMPANY_NAME "${COMPANY_NAME}"
        BUNDLE_ID "com.dawnaudio.audiostreamplugin"

        # On MacOS, plugin is copied to ~/Users/yourname/Library/Audio/Plug-Ins/
        COPY_PLUGIN_AFTER_BUILD FALSE

        # Change me!
        # A unique four-character plugin id
        # Note: this must have at least one upper-case character
        # I figured Plugin Dawn Audio Zero  PD00, then PD01, then PD02, ..., PD08, PD09, PD0A...., PD0Z, PD10, PD11, ...., PDZZ.
        PLUGIN_CODE PD00
        FORMATS "${FORMATS}"

        IS_ARA_EFFECT TRUE

        # The name of your final executable
        # This is how it's listed in the DAW
        # This can be different from PROJECT_NAME and can have spaces!
        # You might want to use v${MAJOR_VERSION} here once you go to v2...
        PRODUCT_NAME "${PRODUCT_NAME}")

# This sets the ARA SDK path
juce_set_ara_sdk_path("${CMAKE_CURRENT_SOURCE_DIR}/ARA_SDK")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

# Disable JUCE ASSERTIONS
target_compile_definitions(SharedCode INTERFACE JUCE_DISABLE_ASSERTIONS)

# C++20, please
# Use cxx_std_23 for C++23 (as of CMake v 3.20)
target_compile_features(SharedCode INTERFACE cxx_std_20)

# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)

file(GLOB SourceFiles CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/RTPWrapper/backends/${RTP_BACKEND}/${RTP_BACKEND}*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/PlayBackController/DocumentController.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/PlayBackController/DocumentController.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/PlayBackController/PlaybackRenderer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/PlayBackController/PlaybackRenderer.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Network/xlet/xlet.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Network/xlet/udp.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Network/xlet/uds.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Network/xlet/xlet.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/RTPWrapper/common/RTPWrap.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Configuration/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Configuration/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/OpusWrapper/opusImpl.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Buffer/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/Buffer/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/SessionManager/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/SessionManager/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/Utilities/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/GUI/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/GUI/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)

# Print the whole set of sourceFiles using a loop
message(STATUS "Source files:")
foreach( file ${SourceFiles} )
    message(STATUS "Source file: ${file}")
endforeach()
target_sources(SharedCode INTERFACE ${SourceFiles})
add_subdirectory(source/RTPWrapper)


####### NLOHMANN JSON ########
# Nlohmann JSON
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG v3.11.2)
FetchContent_MakeAvailable(json)
include_directories(${json_SOURCE_DIR}/single_include)

####### OPUS CODEC ########
# Opus Codec
FetchContent_Declare(
        opuscodec
        GIT_REPOSITORY  git@github.com:xiph/opus.git
        GIT_TAG         v1.4
)

FetchContent_GetProperties(opuscodec)
if(NOT opuscodec_POPULATED)
    FetchContent_Populate(opuscodec)
    set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Opus Static Library" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Opus Static Library" FORCE)
    add_subdirectory(${opuscodec_SOURCE_DIR} ${opuscodec_BINARY_DIR})
endif()
#FetchContent_MakeAvailable(opuscodec)

message (STATUS "opuscodec: " ${opuscodec_SOURCE_DIR}/include)
message (STATUS "opuscodec: " ${opuscodec_BINARY_DIR})

####### SIGNAL AND SLOTS ########
FetchContent_Declare(
        signalsslots
        GIT_REPOSITORY https://github.com/cppdvl/signal_n_slots.git
        GIT_TAG        patriarchal-master
)
FetchContent_MakeAvailable(signalsslots)
include_directories(${signalsslots_SOURCE_DIR})

if (RTP_BACKEND STREQUAL "uvgRTP")
    target_include_directories(SharedCode INTERFACE ${CMAKE_BINARY_DIR}/_deps/uvgrtp-src/include)
    target_link_libraries(SharedCode INTERFACE ${CMAKE_BINARY_DIR}/_deps/uvgrtp-build/libuvgrtp.a)
endif()


####### DOOB ########
FetchContent_Declare(
        DOOB
        GIT_REPOSITORY  git@github.com:DAWn-Audio/DAWN-RELAY.git
        GIT_TAG         websocket_develop
)
FetchContent_MakeAvailable(DOOB)
message(STATUS "DOOB: " ${DOOB_INCLUDE_DIR})

target_include_directories(SharedCode INTERFACE ${DOOB_SOURCE_DIR})
target_link_libraries(SharedCode INTERFACE DOOB)

target_include_directories(SharedCode INTERFACE
        source
        source/Utilities
        source/OpusWrapper
        source/RTPWrapper/common
        ${DOOB_SOURCE_DIR}
        source/Utilities/Network/xlet
        source/RTPWrapper/backends/${RTP_BACKEND}
        ${CMAKE_BINARY_DIR}/_deps/opuscodec-src/include)

target_link_libraries(SharedCode INTERFACE opus)


# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
        INTERFACE

        # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
        JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0

        # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
        # JUCE_DISPLAY_SPLASH_SCREEN=0

        # lets the app known if we're Debug or Release
        CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
        VERSION="${CURRENT_VERSION}"

        # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
        PRODUCT_NAME_WITHOUT_VERSION="DAWn AUDIO's AudioStream Plugin"
        )

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
        INTERFACE
        Assets
        #melatonin_inspector
        juce_audio_utils
        juce_audio_processors
        #juce_dsp
        juce_gui_basics
        juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

option(BUILD_BENCHMARKS "Benchmarks Option" OFF)
option(BUILD_TESTS "Build Tests Option" OFF)
if (BUILD_TESTS)
    message(STATUS "Building tests.......")
    add_subdirectory(tests)
endif()
if (BUILD_BENCHMARKS)
    message(STATUS "Building Benchmarks...")
endif()

option(LIST_VARIABLES "List Variables" OFF) # OFF by default
if (LIST_VARIABLES)
    message(STATUS "Listing Variables....")
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endif()
# Pass some config to GA (like our PRODUCT_NAME)
# include(GitHubENV)

