# Angel Balls store accounts and uploads

Tick each box after confirming it in the relevant account. Store account ownership and approvals have not been verified from this repository.

## When you pay

- **Google Play:** pay **US$25 once** during Play Console developer account registration, unless you already have that account. [Google's fee guide](https://support.google.com/googleplay/android-developer/answer/6112435).
- **Apple:** pay **US$99 per membership year** when enrolling in the Apple Developer Program, unless your membership is already active or you qualify for a waiver. Local-currency pricing may vary. [Apple's enrolment guide](https://developer.apple.com/help/account/membership/program-enrollment).
- **AdSense:** account participation is free. [Google's AdSense fee guide](https://support.google.com/adsense/answer/32850). AdMob sign-up asks for payment details so Google can pay you; that is not a store registration fee. [AdMob sign-up guide](https://support.google.com/admob/answer/7356219).
- **Uploads:** the Play AAB and App Store Connect build uploads occur after the relevant developer membership is set up; the checklist has no separate upload-fee step. A browser host may charge for hosting, depending on the host you choose.

## Android — Google Play

- [ ] **PAY HERE — US$25 once if registering:** Create or confirm a [Google Play Console developer account](https://play.google.com/console/signup). Complete identity, contact and payment-profile setup. [Google's account guide](https://support.google.com/googleplay/android-developer/answer/6112435).
- [ ] In [Play Console](https://play.google.com/console/), create the app with Android package `dev.luke10x.angelballs`. Confirm the package name before the first upload. [Create-app guide](https://support.google.com/googleplay/android-developer/answer/9859152).
- [ ] Complete the store listing, contact details, app content, privacy policy, data safety and ads declarations.
- [ ] Create and protect an upload key, enrol in Play App Signing, and build a signed release **AAB**. This is still incomplete in [TODO-ANDROID.md](TODO-ANDROID.md).
- [ ] Upload the `.aab` in **Play Console → Test and release → Testing → Internal testing**. Install the Play-distributed build and review the pre-launch results. [Testing guide](https://support.google.com/googleplay/android-developer/answer/9845334).
- [ ] Meet any testing requirements shown for this account, then submit a release in **Play Console → Test and release → Production**. [Release guide](https://support.google.com/googleplay/android-developer/answer/9859348).

## iPhone and iPad — App Store

- [ ] **PAY WHEN ENROLLING — US$99 per year:** Create or confirm an [Apple Account](https://account.apple.com/) and enrol in the [Apple Developer Program](https://developer.apple.com/programs/enroll/). Creating the Apple Account itself is free.
- [x] Set the final iOS bundle identifier to `dev.luke10x.angelball` in both Debug and Release, matching the Angel Ball store name and Luke10x developer namespace. Confirm the ID is available in your Apple Developer account when registering it.
- [ ] In [App Store Connect](https://appstoreconnect.apple.com/), create an app record with the chosen bundle ID. Apple requires the record before a build upload. [App-record guide](https://developer.apple.com/help/app-store-connect/create-an-app-record/add-a-new-app).
- [ ] Set up distribution signing, make a signed Release archive in Xcode, and complete the App Store privacy and listing details.
- [ ] Upload via **Xcode Organizer → Distribute App → App Store Connect**. Select the processed build in **App Store Connect → TestFlight**. [Upload guide](https://developer.apple.com/help/app-store-connect/manage-builds/upload-builds/).
- [ ] Test through TestFlight, resolve review or crash findings, then submit the selected build for **App Review**. [Workflow guide](https://developer.apple.com/help/app-store-connect/get-started/app-store-connect-workflow).

## Ads and browser release

- [ ] In [AdMob](https://admob.google.com/), create or confirm the publisher account and add separate Android and iOS apps. Link each to its supported store listing for review and full ad-serving eligibility. [AdMob setup guide](https://support.google.com/admob/answer/9989980).
- [ ] In [AdSense](https://www.google.com/adsense/start/), check the site review and separate [H5 Games Ads application](https://support.google.com/adsense/answer/1705831). Both were recorded as pending in October; verify their current status.
- [ ] For a browser release, upload the game through the chosen site host's deployment flow and serve `ads.txt` at the site root. The repository does not identify a verified live upload destination.

## Detailed release plans

- [Android release readiness](TODO-ANDROID.md)
- [iOS release readiness](TODO-IOS.md)
- [Ads and browser handoff](docs/ads-config.md)
