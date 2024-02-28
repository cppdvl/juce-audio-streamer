![DawnAudio](assets/images/dawnaudio.png)

Dawn Audio's Audio Stream Plugin.
This is forked from [Pamplejuce](https://github.com/sudara/pamplejuce) by Sudara.... 


To Build this Plugin (Useful for developing / compiling / debugging / testing workcycles) : 

* MAC && CLION: 

```
mkdir path-to-workspace
cd path-to-workspace
git clone --recursive git@github.com:DAWn-Audio/Plugin.git rpo
mkdir bld
cd bld
```

If using CLION:

BUILD -- DEBUG
```
cmake -DCMAKE_BUILD_TYPE=Debug -GNinja -DRTP_BACKEND=udpRTP -S ../rpo -B .
ninja -j 10
```

BUILD -- RELEASE
```
cmake -DCMAKE_BUILD_TYPE=Release -GNinja -DRTP_BACKEND=udpRTP -S ../rpo -B .
ninja -j 10
```

* MAC && XCODE

If using XCODE:

BUILD -- DEBUG
```
cmake -DCMAKE_BUILD_TYPE=Debug -S ../rpo -B . -GXcode -DRTP_BACKEND=udpRTP 
xcodebuild -project AudioStreamPlugin.xcodeproj -target "AudioStreamPlugin_VST3" -configuration Debug clean build

#At the time of writing this a ZERO CHECK error appears at the end of the Plugin build, meaning plugin is build but couldn't be installed. Manually copy it to ~/Library/Audio/Plug-Ins/VST3 to circumvent the issue. 
```


BUILD -- RELEASE
```
cmake -DCMAKE_BUILD_TYPE=Release -S ../rpo -B . -GXcode -DRTP_BACKEND=udpRTP
xcodebuild -project AudioStreamPlugin.xcodeproj -target "AudioStreamPlugin_VST3" -configuration Release clean build

#At the time of writing this a ZERO CHECK error appears at the end of the Plugin build, meaning plugin is build but couldn't be installed. Manually copy it to ~/Library/Audio/Plug-Ins/VST3 to circumvent the issue. 
```

BUILDING FOR MAC INTEL
In order to build for MAC INTEL to any of the cmake commands add the paramter  -DCMAKE_OSX_ARCHITECTURES=x86_64



# MAKING THE INSTALLER

## 1. CREATE A KEYCHAIN ACCESS PROFILE (ONCE MAYBE):
   - **Requirements:**
     1. Apple Developer ID.
     2. App-specific password.
     3. TeamID.
   - **Execute:** 
     ```
     credstore.sh
     ```
   - The script will prompt for a name for the KEYCHAIN ACCESS PROFILE; this profile name should be used in step 3, so take note of it. Step 3 will refer to it as variable `KPRF` (Keychain Profile).

## 2. BUILD THE APPLICATIONS:
   1. Execute the build application script. The script will prompt to build for `arm64`, `x86_64`, or `universal`.
   2. **Execute:** 
      ```
      builder.sh
      ```

## 3. CODE SIGNATURE, NOTARIZATION & STAPLING for each architecture made with the builder in STEP 2:
   - **Variables**
     1. The keychain profile produced in Step 1 `KPRF`.
     2. `ARCH` is either `arm64`, `x86_64`, or `universal`.
     3. `BUILD_TYPE` is either `Debug` or `Release`.
   - **Execute from the rpo folder:** 
     ```
     cd ..
     cd builder
     codesign -s "Developer ID Application: DAWn Audio LLC (974T9UA6P6)" mac-${ARCH}-${BUILD_TYPE}/AudioStreamPlugin_artefacts/${BUILD_TYPE}/VST3/DPlugin.vst3/Contents/MacOS/DPlugin —timestamp —force
     ditto -c -k —sequesterRsrc —keepParent mac-${ARCH}-${BUILD_TYPE}/AudioStreamPlugin_artefacts/${BUILD_TYPE}/VST3/DPlugin.vst3 DPlugin_m${ARCH}.zip
     xcrun notarytool submit DPlugin_${ARCH}.zip —keychain-profile "${KPRF}"
     ditto -x -k DPlugin_${ARCH}.zip product_${ARCH}/DPlugin.vst3
     xcrun stapler staple product_${ARCH}/DPlugin.vst3
     ```
   - The most sensitive step is the `xcrun notarytool submit`, if that fails the way to troubleshoot is:
     1. Take note of the submission id. It will be logged during `xcrun submit` in the command line screen, something like `ed146d6f-5aa4-4f97-ae15-618a54ae443b`.
     2. Execute the following command:
        ```
        xcrun notarytool log ed146d6f-5aa4-4f97-ae15-618a54ae443b --keychain-profile ${KPRF}
        ```
     3. Send the information to the development team in order to find out what the problem is.

## 4. CREATE THE DMG:
   1. **Execute the following:**
      1. `mkdir /tmp/DPluginDMGFolder`
      2. `mv -R product_${ARCH}/DPlugin.vst3 /tmp/DPluginDMGFolder/`
      3. `ln -s /Library/Audio/Plug-Ins/VST3 /tmp/DPluginDMGFolder/VST3_PLUGINS`
      4. `hdiutil create -volname "DPlugin Installer" -srcfolder /tmpDPluginDMGFolder -ov -format UDZO DPlugin.dmg`



