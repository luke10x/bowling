# Loss interstitials

A completed bowling loss requests one interstitial. Android and iOS preload
the next ad while the match runs. If no ad is ready, the normal result screen
remains available. Dismissing or closing an ad returns to that result screen.
The ad network controls when its close button appears and may decide not to
serve an ad.

The repository uses Google's sample mobile app and interstitial unit IDs for
testing. Replace both IDs before distributing an ad-supported build:

- Android: pass `-PADMOB_ANDROID_APP_ID=ca-app-pub-...~...` and
  `-PADMOB_ANDROID_LOSS_UNIT=ca-app-pub-.../...` to Gradle (or put these
  properties in a local Gradle properties file).
- iOS: override `ADMOB_IOS_APP_ID` and `ADMOB_IOS_LOSS_UNIT` from
  `ios-project/ios-project.xcconfig` in the build configuration.
- Emscripten: set `BOWLING_ADSENSE_CLIENT` in `wasm/ads_config.js` to the
  approved H5 Games AdSense publisher ID, and turn off `BOWLING_ADS_TEST`
  before publishing. The browser build skips ads while the ID is empty.

Browser interstitials use Google's H5 Games Ad Placement API, which requires
publisher approval and a tap to start an ad break. The loss overlay's Continue
button supplies that tap. It closes if the browser cannot request an ad.
