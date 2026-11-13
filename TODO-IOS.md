# iOS release readiness

**Completion: 22% (2 of 9 checklist items verified complete).** Count only checked top-level items; incomplete and unverified items count as zero. An unsigned Debug build is evidence of a build path, not an App Store release.

## Checklist

- [ ] Xcode and iOS SDK release build — **unverified**
- [x] ARM64 device build output — **complete for Debug**
- [x] SDL2 static integration — **complete for Debug**
- [ ] Distribution signing and archive — **incomplete / unverified**
- [ ] App and SDK privacy manifests — **unverified**
- [ ] Privacy, ads and App Store declarations — **unverified**
- [ ] Lifecycle and device testing — **unverified**
- [ ] TestFlight distribution — **unverified**
- [ ] App Store release readiness — **unverified**

## Detailed items and verification criteria

### 1. Xcode and iOS SDK release build — unverified

- **Complete in local setup only:** `xcodebuild -version` reports **Xcode 26.2**. The Xcode project sets `SDKROOT = iphoneos` and an iOS **13.0** deployment target. `ios-project/IOS-JOURNAL.md` records a successful unsigned device Debug build.
- **Unverified:** A clean Release archive using the SDK required at submission time, deployment target support, and App Store validation.
- **Verify:** On the submission machine, inspect the selected Xcode/iOS SDK, perform a clean `Release` archive for `generic/platform=iOS`, and validate the archive in Xcode/App Store Connect.

### 2. ARM64 device output — complete for Debug

- **Complete:** `Makefile.ios` sets `CMAKE_OSX_ARCHITECTURES=arm64` for SDL2/Jolt. The local `build/ios/DerivedData/Build/Products/Debug-iphoneos/ios-project.app/ios-project` is a **Mach-O arm64** executable with `MinimumOSVersion` **13.0**.
- **Unverified:** ARM64 architecture and third-party binary slices in the final distribution archive.
- **Verify:** Inspect the Release archive executable and embedded libraries with `lipo -info` or `file`, then install its signed build on a supported iPhone.

### 3. SDL2 integration — complete for Debug

- **Complete:** `Makefile.ios` builds SDL2 as a static iOS library; `ios-project/ios-project.xcconfig` links `-lSDL2` and `-lSDL2main`. A local `build/ios/usr/lib/libSDL2.a` and unsigned Debug app are present.
- **Unverified:** Release archive linkage and device behaviour with the current SDL2 build.
- **Verify:** Rebuild SDL2 for the intended SDK and ARM64 target, archive Release, inspect the linked app and launch it on a supported device.

### 4. Distribution signing and archive — incomplete / unverified

- **Incomplete:** `Makefile.ios build` explicitly sets `CODE_SIGNING_ALLOWED=NO` and produces an unsigned Debug app. The Xcode project has automatic signing and a development team configured, but this does not prove distribution signing.
- **Unverified:** Distribution certificate, provisioning, bundle identifier registration, entitlements, signed `.xcarchive` and export.
- **Verify:** Select the correct App Store Connect team and bundle ID, create a signed Release archive, validate signing and entitlements, and export or upload it successfully.

### 5. App and SDK privacy manifests — unverified

- **Complete in local dependencies only:** The local Google Mobile Ads and User Messaging Platform frameworks contain `PrivacyInfo.xcprivacy` files.
- **Unverified:** Whether the game itself requires a privacy manifest, whether all SDK manifests are present in the final archive, and whether required-reason API declarations are accurate. No app-level `PrivacyInfo.xcprivacy` was found under `ios-project`.
- **Verify:** Audit app and SDK API usage against current Apple requirements; add an app manifest if needed, then inspect the signed archive's merged privacy report and resolve validation warnings.

### 6. Privacy, ads and App Store declarations — unverified

- **Complete in source only:** `ios-project/ios-project/Info.plist` contains AdMob application ID placeholders and a `SKAdNetworkItems` entry; `docs/ads-config.md` describes Debug sample IDs and Release production IDs.
- **Unverified:** Consent and tracking choices, App Tracking Transparency applicability, App Store privacy answers, privacy policy, age rating and production ad behaviour.
- **Verify:** Audit actual data flows and SDK behaviour in a Release-configured build, implement any required consent/permission flow, and match App Store Connect answers and the public privacy policy to observed behaviour.

### 7. Lifecycle and device testing — unverified

- **Complete in source only:** `game.cpp` handles SDL window focus loss/gain and audio suspension/resumption. The inspected `AppDelegate.m` and `SceneDelegate.m` do not provide test evidence for scene restoration.
- **Unverified:** Backgrounding, foregrounding, interruptions, calls, audio route changes, ad return, memory pressure, rendering context recovery and saved progress on supported iPhones/iPads.
- **Verify:** Run a signed device build through these cases, including termination/relaunch; confirm rendering, sound, controls and progress recover without crashes.

### 8. TestFlight — unverified

- **Unverified:** Upload, processing, internal/external tester access, feedback and crash review. No TestFlight or App Store Connect evidence is in this repository.
- **Verify:** Upload the signed archive, install from TestFlight on supported devices, complete a test matrix and resolve reported crashes and review issues.

### 9. App Store release readiness — unverified

- **Unverified:** Final metadata, screenshots, age rating, accessibility checks, export compliance, in-app purchase/ad disclosures where applicable, and App Review acceptance.
- **Verify:** Complete and review the App Store Connect record, validate the exact submitted build and its privacy report, then confirm review acceptance before marking this complete.
