# Android release readiness

**Completion: 64% (7 of 11 checklist items verified complete).** Count only checked top-level items; incomplete and unverified items count as zero. This is a repository and local artefact audit, not a Play Console approval.

## Checklist

- [x] API 36 compile and target configuration — **complete for benchmark build**
- [x] Android Gradle Plugin version configured — **complete**
- [x] NDK and native linker configuration — **complete for benchmark build**
- [x] `libmain.so` 16 KB ELF alignment — **complete for rebuilt benchmark**
- [x] `libSDL2.so` 16 KB ELF alignment — **complete for rebuilt benchmark**
- [x] Benchmark APK 16 KB ZIP alignment — **complete for the inspected APK**
- [x] ARM64 native output present — **complete for the inspected build**
- [ ] Signed release AAB and Play App Signing — **incomplete / unverified**
- [ ] Lifecycle and 16 KB device testing — **unverified**
- [ ] Privacy and Play declarations — **unverified**
- [ ] Play testing and production readiness — **unverified**

## Detailed items and verification criteria

### 1. API 36 compile and target configuration — complete for benchmark build

- **Complete in source:** `android-project/app/build.gradle` now sets both `compileSdkVersion` and `targetSdkVersion` to **36**. `minSdkVersion` remains 21.
- **Complete:** `:app:assembleBenchmark --offline` succeeded after the API 36 change.
- **Complete for benchmark on emulator:** The game and SDL activity launched and rendered on the API 36 16 KB ARM64 emulator (see item 9).
- **Unverified:** Ads dependency and permission behaviour on API 36, and runtime compatibility of a signed release build.
- **Verify:** Set both SDK values to 36, build and run the release variant, then inspect the merged manifest or final bundle for `targetSdkVersion=36`. Exercise affected platform behaviour on an API 36 device or emulator.

### 2. Android Gradle Plugin — complete for version configuration

- **Complete:** `android-project/build.gradle` declares AGP **8.12.3**; the wrapper uses Gradle **8.13**.
- **Unverified:** A fresh API 36 release build and Play-generated APKs have not been established by this configuration check.
- **Verify:** Build the updated release AAB after the SDK and native changes, then inspect generated APKs and Play Console warnings.

### 3. NDK and native linker — complete for benchmark build

- **Complete in source:** `android-project/app/CMakeLists.txt` now applies 16 KB page linker options to both `main` and `SDL2`.
- **Complete for benchmark:** The rebuilt native libraries have `0x4000` alignment.
- **Complete:** `android-project/app/build.gradle` pins NDK **27.0.12077973**, matching the local CMake caches. `:app:assembleBenchmark --offline` succeeds with that pin.
- **Verify:** Pin and use an NDK/toolchain that produces 16 KB compatible libraries, or apply appropriate linker options to **both** `main` and the actual SDL2 shared-library target. Clean native outputs, rebuild, and check every shipped ARM64 `.so` with `llvm-readelf -lW`.

### 4. `libmain.so` ELF alignment — complete for rebuilt benchmark

- **Complete for benchmark:** The earlier screenshot showed failure, but the rebuilt benchmark merged library now has **`0x4000`** alignment on every `LOAD` segment.
- **Verify:** Rebuild and inspect the **final packaged** ARM64 library; every `LOAD` segment must have alignment of at least **`0x4000`** (16 KB). Repeat for all release native libraries.

### 5. `libSDL2.so` ELF alignment — complete for rebuilt benchmark

- **Complete for benchmark:** The earlier screenshot showed failure, but the rebuilt benchmark merged library now has **`0x4000`** alignment on every `LOAD` segment. SDL2 required its own linker settings.
- **Verify:** Rebuild SDL2 with compatible linker settings and check its final packaged `LOAD` segments for at least **`0x4000`** alignment.

### 6. APK ZIP alignment — complete for the inspected benchmark APK

- **Complete:** Android Build Tools 36 `zipalign -c -P 16 -v 4 android-project/app/build/outputs/apk/benchmark/app-benchmark.apk` returned **“Verification successful”**. This checks ZIP placement, not ELF `LOAD` alignment.
- **Unverified:** ZIP alignment of APKs generated from a release AAB.
- **Verify:** Run the same check against release and Play-generated APKs after rebuilding. Inspect whether packaged native libraries are compressed and apply the appropriate packaging check.

### 7. ARM64 output — complete for the inspected build

- **Complete:** The benchmark variant specifies `abiFilters 'arm64-v8a'`, and local benchmark output contains both ARM64 native libraries.
- **Unverified:** The final release AAB's complete ABI contents and installability on an ARM64 device.
- **Verify:** Inspect the release AAB/generated APKs for `lib/arm64-v8a/` and install and launch on a physical ARM64 device.

### 8. Signed AAB and Play App Signing — incomplete / unverified

- **Incomplete:** `Makefile.android` builds an **unsigned release APK**; the benchmark uses **debug signing**. No signed release AAB or release signing configuration was found in the inspected project files.
- **Unverified:** Upload key, Play App Signing enrolment and bundle acceptance.
- **Verify:** Configure protected release signing, run `:app:bundleRelease`, verify the AAB signature and version code, and confirm Play Console accepts the bundle and produces installable APKs.

### 9. Lifecycle and 16 KB device testing — unverified

- **Complete in source only:** `TrackerActivity.onResume()` contains ad-return recovery; `game.cpp` handles SDL focus loss/gain and audio suspension/resumption.
- **Complete for benchmark on emulator (6 November 2026):** Installed the API 36 Google APIs 16 KB ARM64 system image and created `AngelBalls_API36_16K`. `adb -s emulator-5556 shell getconf PAGE_SIZE` returned **`16384`**. The benchmark APK installed, launched, and rendered the game. Sending the app to Home and reopening it kept the same process alive and rendered gameplay again.
- **Unverified:** Screen lock, rotation where applicable, ad return, audio, save state, EGL context recovery, extended play, and behaviour of a signed release build or physical 16 KB device. The emulator run alone does not complete the lifecycle checklist item.
- **Verify:** Run those cases on release-like builds, including a 16 KB page-size environment; check `adb shell getconf PAGE_SIZE` returns `16384`, monitor logcat and confirm play resumes without lost rendering or progress.

### 10. Privacy and Play declarations — unverified

- **Complete in source only:** The manifest declares vibration, and Gradle includes Google Mobile Ads; `docs/ads-config.md` describes ad ID configuration.
- **Unverified:** Current data collection, consent handling, privacy policy, Play Data safety form, ads declaration, target audience and permission disclosures. Source wiring alone does not verify these store-facing claims.
- **Verify:** Audit SDK and game data flows in a signed build, implement any required consent flow, publish the policy, then match Play Console declarations to the observed behaviour.

### 11. Play testing and production readiness — unverified

- **Unverified:** Internal/closed testing, pre-launch report, device coverage, production ads, crash/ANR review and release approval. No Play Console evidence is in this repository.
- **Verify:** Upload the signed AAB to a test track, install its Play-generated APKs, complete a device test matrix, resolve Console findings, and verify release configuration before promoting it.
