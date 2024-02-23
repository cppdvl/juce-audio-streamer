![DawnAudio](assets/images/dawnaudio.png)

Dawn Audio's Audio Stream Plugin.
This is forked from [Pamplejuce](https://github.com/sudara/pamplejuce) by Sudara.... 


To Build this Plugin: 

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




