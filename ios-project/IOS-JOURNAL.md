#!/bin/bash
today=$(date '+%Y-%m-%d, %a')
echo -e "\n## $today" >> "$0"
exit 0

## 2026-01-09, Fri

Using Cmake to build SDL2 and Jolt, but not sure if they produce iOs build.
Later I include Source files like:

should I copy: game/ios-project/libs/libJolt.a
Target → Build Settings → Header Search Paths

Target → Build Phases → Link Binary With Libraries:

OpenGLES.framework
UIKit.framework
Foundation.framework
QuartzCore.framework
AudioToolbox.framework
CoreMotion.framework

Target → Build Phases → Link Binary With Libraries → +

	1.	In Xcode, select your project → Target → Build Phases → Compile Sources
	2.	Click + → “Add Files…” → Navigate to bowling/ root and select:
	•	game.cpp
	•	sidecar.cpp
	•	physics/physics.cpp
	•	framework/boot.cpp
	3.	Make sure “Copy items if needed” is UNCHECKED
	•	This keeps your shared code in the root and not duplicated into the Xcode project.

    4. Add header search paths
	1.	Target → Build Settings → Header Search Paths
	2.	Add these (recursive where needed):

    Add them to Xcode:
	1.	Target → Build Phases → Link Binary With Libraries → + → “Add Other…” → choose the .a files
	2.	Make sure libSDL2.a is first if SDL2main is needed
	3.	Add system frameworks for iOS:

    OpenGLES.framework
UIKit.framework
Foundation.framework
CoreGraphics.framework
QuartzCore.framework


6. Set Build Settings for iOS
	•	C++ Language Standard → C++17
	•	Architectures → arm64 (or $(ARCHS_STANDARD))
	•	Bitcode → No (unless you enabled it in SDL build)
	•	Enable Modules (C and Objective-C) → Yes (needed for UIKit, etc.)