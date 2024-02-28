#!/bin/bash

#AUTHOR: Julian Andres Guarin Reyes - TCPPDVL.
#SCRIPT TO CODESIGN AN APP FOR MACOS.


#if no parameter with the full path of the app is provided, then the script will prompt for it.
if [ -z "$1" ]
then
    #prompt for the VST3 path.
    echo "Enter the FULL PATH fo your VST3 application:"
    echo "Example:\nmac-arm64-Release/AudioStreamPlugin_artefacts/Release/VST3/DPlugin.vst3/Contents/MacOS/DPlugin"
    read VST3_PATH 
else
    VST3_PATH=$1
fi

#Prompt for the Developer ID Application certificate password
echo "Enter the name of the Developer ID Application certificate:"
echo "Example:\nDeveloper ID Application: Acme LLC (D34D833F01)"
read CERTIFICATE_NAME
echo "Certificate Name: $CERTIFICATE_NAME"
echo "Application to sign: $VST3_PATH"

codesign -s "$CERTIFICATE_NAME" $VST3_PATH --force --timestamp 

#substr the VST3 path to get the parent directory
VST3_PARENT_DIR=$(dirname $(dirname $(dirname $VST3_PATH))) 
echo "VST3 Parent Directory: $VST3_PARENT_DIR"

#get architecture
root_dir=$(dirname $(dirname $(dirname $(dirname $VST3_PATH))))
echo "Root Directory: $root_dir"
#split root_dir using - as separator and use the 2nd element as the architecture
IFS='-' read -ra ADDR <<< "$root_dir"
ARCHITECTURE=${ADDR[1]}
#extract the name
appname=$(basename $VST3_PATH)



#strip properties
echo "Stripping properties from the VST3 application"
xattr -cr $VST3_PARENT_DIR

#notarization
echo "Notarizing the VST3 application"
ditto -c -k --sequesterRsrc --keepParent $VST3_PARENT_DIR $appname-$ARCHITECTURE.zip

#prompt for the keychain profile with the app password.
echo "Enter the name of the keychain profile with the app password:"
read KEYCHAIN_PROFILE

xcrun notarytool submit $appname-$ARCHITECTURE.zip --keychain-profile "$KEYCHAIN_PROFILE" --wait

#if result is successful, staple the ticket to the app.

if [ $? -eq 0 ]
then
    echo "Stapling the ticket to the VST3 application"
    ditto -x -k $appname-$ARCHITECTURE.zip product_$ARCHITECTURE/$appname.vst3
    xcrun stapler staple $product_$ARCHITECTURE/$appname.vst3
fi








