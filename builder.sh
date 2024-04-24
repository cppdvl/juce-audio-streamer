#!/bin/bash

# Check if build type is provided as a command-line argument, default to Release if not.
BUILD_TYPE=${1:-Release}

# Welcome Message. DAWn-Audio Studio and Julian Guarin are the authors of this script.

echo "Welcome to the DAWn-Audio Studio Rogner build script."

# Prompt for plugin format
echo "Please enter the plugin format you would like to build (VST3, AU, or Standalone):"
echo "1. VST3"
echo "2. AU"
echo "3. Standalone"
read -p "Enter the number of the plugin format you would like to build: " plugin_format

# Set the build type based on the user input
case $plugin_format in
    1)
        PLUGIN_FORMAT="VST3"
        ;;
    2)
        PLUGIN_FORMAT="AU"
        ;;
    3)
        PLUGIN_FORMAT="Standalone"
        ;;
    *)
        echo "Invalid input. Defaulting to VST3."
        PLUGIN_FORMAT="VST3"
        ;;
esac


# Prompt for build type with a menu: 1. Release 2. Debug 3. MinSizeRel 4. RelWithDebInfo
echo "Please select the build type:"
echo "1. Release"
echo "2. Debug"
echo "3. MinSizeRel"
echo "4. RelWithDebInfo"
read -p "Enter the number of the build type: " build_type

# Set the build type based on the user's selection
case $build_type in
    1)
        BUILD_TYPE=Release
        ;;
    2)
        BUILD_TYPE=Debug
        ;;
    3)
        BUILD_TYPE=MinSizeRel
        ;;
    4)
        BUILD_TYPE=RelWithDebInfo
        ;;
    *)
        echo "Invalid selection. Defaulting to Release."
        BUILD_TYPE=Release
        ;;
esac

#function to build on mac using the $build_type variable as parameter
function build_mac {
    echo "Building for Mac $BUILD_TYPE"
    # Run the build script with the selected build type.
    cd ..
    # create a vector with strings arm64 and x86_64
    archs=("arm64" "x86_64" "arm64;x86_64")
    mkdir -p build
    for arch in "${archs[@]}"
    do
        pushd .
        #promp y/n to build for each architecture
        read -p "Do you want to build for $arch? (y/n): " build_arch
        if [ $build_arch == "n" ]; then
            continue
        fi
        case $arch in
            "arm64")
                archfolder="arm64"
                ;;
            "x86_64")
                archfolder="x86_64"
                ;;
            "arm64;x86_64")
                archfolder="universal"
                ;;
        esac

    
        build_folder=$PLUGIN_FORMAT-mac-$archfolder-$BUILD_TYPE
        mkdir -p build/$build_folder
        cd build/$build_folder
        rm -Rf * 
        case $arch in
            "arm64")
                echo "Building for arm64"
                cmake -S ../../rpo -B . -GNinja -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_OSX_ARCHITECTURES=arm64 -DRTP_BACKEND=udpRTP -DFORMATS=$PLUGIN_FORMAT
                ;;
            "x86_64")
                echo "Building for x86_64"
                cmake -S ../../rpo -B . -GNinja -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_OSX_ARCHITECTURES=x86_64 -DRTP_BACKEND=udpRTP -DFORMATS=$PLUGIN_FORMAT
                ;;
            "arm64;x86_64")
                echo "Building Universal"
                cmake -S ../../rpo -B . -GNinja -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DRTP_BACKEND=udpRTP -DFORMATS=$PLUGIN_FORMAT
                ;;
        esac

        #set a flag indicating things went wrong
        error_flag=0
        if [ $? -eq 0 ]; then
            ninja -j $(sysctl -n hw.logicalcpu_max)
            if [ $? -eq 0 ]; then
                echo "Build for $arch with build_type: $BUILD_TYPE completed successfully"
            else
                echo "Error building for $arch with build_type: $BUILD_TYPE"
                error_flag=1
                continue #skip to the next arch
            fi
        else
            echo "Error configuring for $arch with build_type: $BUILD_TYPE"
            error_flag=1
            continue #skip to the next arch
        fi
        popd
    done

}

#function to build on windows using the $build_type variable as parameter
function build_win {
    echo "Building for Windows"
    # Run the build script with the selected build type.
    cd ..
    mkdir -p build/win-$build_type
    cd build/win-$build_type
    rm -Rf * #clean the build folder
    cmake -DCMAKE_BUILD_TYPE=$build_type -S ../rpo -B . -G "Visual Studio 16 2019"
    #check for errors

    #set a flag indicating things went wrong
    error_flag=0
    if [ $? -eq 0 ]; then
        cmake --build . --config $build_type
        if [ $? -eq 0 ]; then
            echo "Build for $arch with build_type: $build_type completed successfully"
        else
            echo "Error building for $arch with build_type: $build_type"
            error_flag=1
        fi
    else
        echo "Error configuring for $arch with build_type: $build_type"
        error_flag=1
    fi

    return $error_flag

}

#determine the OS ammong Windows/Mac/Linux

#check if the OS is Mac
if [ "$(uname)" == "Darwin" ]; then
    echo $'Mac OS detected'
    build_mac
else
    #not supported
    echo "This script only supports Mac OS at the moment"
    exit 1
fi
