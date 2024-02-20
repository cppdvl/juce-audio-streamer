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

```
cmake -S ../rpo -B . -GXcode
xcodebuild -project project.xcodeproj -scheme "DAWNAUDIOPLUGIN" -target "DAWNAUDIO_VST3" -configuration Debug build
```



