#!/bin/bash
today=$(date '+%Y-%m-%d, %a')
echo -e "\n## $today" >> "$0"
exit 0

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

