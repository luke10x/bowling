#!/bin/bash
today=$(date '+%Y-%m-%d, %a')
echo -e "\n## $today" >> "$0"
exit 0

## 2026-09-04, Fri

Revived generic iPhoneOS Debug build:

```sh
xcodebuild -quiet \
  -project ios-project/ios-project.xcodeproj \
  -scheme ios-project \
  -configuration Debug \
  -sdk iphoneos \
  -destination generic/platform=iOS \
  -derivedDataPath build/ios/DerivedData \
  CODE_SIGNING_ALLOWED=NO \
  build
```

Notes:

- Keep the build output under `build/ios/DerivedData` to avoid Xcode sandbox/cache permission noise.
- The old launch storyboard path broke `ibtool` with `iOS 26.2 Platform Not Installed`; the working setup has no `LaunchScreen.storyboard`, matching the older note below.
- The iOS target needs `GLM_ENABLE_EXPERIMENTAL` before GLM headers because current game code uses GLM GTX headers.
- Clay should come from `build/_deps/clay-src`; do not point iOS header search paths at an external `../clay` checkout.
- The modern sound stack needs `../eggsfm` and `../my-ym2612-plugin/build/_deps/ymfm-src/src` headers, plus `xfm_impl.cpp`, `sounds.cpp`, and ymfm implementation sources compiled into the iOS unity entry point.
- The revived unsigned build produces `build/ios/DerivedData/Build/Products/Debug-iphoneos/ios-project.app/ios-project` as a Mach-O arm64 executable with `MinimumOSVersion = 13.0`.

## 2026-01-09, Fri

Using Cmake to build SDL2 and Jolt, but not sure if they produce iOs build.

Important XCode configurations:

For header locations:
Target -> Build Settings -> Search Paths
-> System Header Search Paths

For static libraries and frameworks:
Target → Build Phases → Link Binary With Libraries:

OpenGLES.framework
UIKit.framework
Foundation.framework
QuartzCore.framework
AudioToolbox.framework
CoreMotion.framework

Add just main.mm
Project → Target → Build Phases → Compile Sources

Then there is a very tricky part to make sure no story boards are selected:
Target -> Properties -> Custom iOS Target Properties
