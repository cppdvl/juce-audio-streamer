#!/bin/bash

#AUTHOR: Julian Andres Guarin Reyes - TCPPDVL.
#SCRIPT TO CODESIGN AN APP FOR MACOS.

#promp for architecture
echo "Enter the architecture of the app (x86_64, arm64):"
read architecture
if [ -z "$architecture" ]; then
    echo "Architecture is required"
    exit 1
fi

#prompt for plugin type 1: vst3 2: component

echo "Enter the plugin type 1: vst3, 2: component (1):"
read pluginType

if [ -z "$pluginType" ]; then
    pluginType=1
fi

#if plugintype is 1 ext is vst3 else is component
if [ $pluginType -eq 1 ]; then
    ext="vst3"
else
    ext="component"
fi


#prompt for keychain profile
echo "Enter the keychain profile name: "
read keychain_profile

#if no parameter with the full path of the app is provided, then the script will prompt for it.
if [ -z "$1" ]
then
    #prompt for the path.
    echo "Enter the FULL PATH fo your application:"
    echo "Example:\nmac-arm64-Release/AudioStreamPlugin_artefacts/Release/APP/DPlugin.vst3/Contents/MacOS/DPlugin"
    read APP_PATH 
else
    APP_PATH=$1
fi


#Prompt for the Developer ID Application certificate password
echo "Enter the name of the Developer ID Application certificate: (DAWn Audio LLC)"
read PROVIDER_NAME

#if provider name is empty
if [ -z "$PROVIDER_NAME" ]
then
    PROVIDER_NAME="DAWn Audio LLC"
    echo "you proider name is empty, using default value: $PROVIDER_NAME"
fi

echo "Enter WWDRTeamID: (974T9UA6P6)"
read WWDRTeamID
if [ -z "$WWDRTeamID" ]
then
    WWDRTeamID="974T9UA6P6"
    echo "you WWDRTeamID is empty, using default value: $WWDRTeamID"
fi

codesign -s "Developer ID Application: $PROVIDER_NAME ($WWDRTeamID)" $APP_PATH --force --timestamp 

#substr the APP path to get the parent directory
appnameapple=$(dirname $(dirname $(dirname $APP_PATH))) 
echo "APP Parent Directory: $APP_PARENT_DIR"
#extract the name
appname=$(basename $APP_PATH)

ditto -c -k --sequesterRsrc --keepParent $appnameapple tmp.zip 
xcrun notarytool submit tmp.zip --keychain-profile "$keychain_profile" --wait
#if the notarization was successful, then remove the zip file.
if [ $? -eq 0 ]
then
    echo "Notarization went ok" 
else
    echo "Notarization failed, please check the logs."
    exit 1
fi

#generate a variable with the format YYMMDDHHMM
#
DATE=$(date +"%y%m%d%H%M")
ditto -x -k tmp.zip product-$appnameapple-$architecture-$DATE
xcrun stapler staple product-$appnameapple-$architecture-$DATE/$appnameapple




