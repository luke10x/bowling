# Ad configuration

## Handoff status — 19 October 2026

Ad work is parked while the AdSense site review and separate H5 Games Ads application are pending. This is the state recorded in the project and reported by the account owner; this document does not imply that Google has approved either application. The last local browser build is `build/emscripten/bowling.zip`. Its presence does not establish that it has been uploaded to the live site.

| Area | Implemented | Still to do or verify |
| --- | --- | --- |
| Shared game rules | Ad credit, local-day login bonus, outcome costs, daily interstitial limit, 24-hour holiday, rewarded money limits, and save/load are implemented in `ad_policy.h`. A win requests an interstitial at the result; a loss defers it until Retry, before the next round's intro. The cheat-only Ad Manager displays the counters and rules. | Review the test-tuned numbers in `ads_config.h` before treating them as final live policy. Test across a day boundary and a shop restock on each target platform. |
| Rewarded shop card | The reward card appears when an offer remains and the platform says rewarded content is available. A completed reward callback grants the configured $100, saves the bank and policy, and runs the card, coin, and bank payout animation. Skipping or a failed ad grants nothing. A consumed card does not reappear until the shop is reopened, provided the restock and daily limits still allow another offer. | Verify the full reward, skip, no-fill, shop-reopen, and limit flows on release-like builds. |
| Android AdMob | Interstitial and rewarded SDK callbacks are wired to the game. Debug and benchmark builds use Google's sample IDs; Release reads the supplied production app and unit IDs from `ads_config.h`. The Android return-from-ad surface/focus issue was fixed and checked during device testing. | Confirm production ads can load and complete in a signed Release build when Google makes them available. Recheck audio, focus, and bank payout after returning from an ad. |
| iOS AdMob | Interstitial and rewarded SDK callbacks are wired to the game. Xcode Debug uses sample IDs; Release uses the supplied production IDs through the generated `.xcconfig`. | Test rewarded completion, skip/dismiss, no fill, interstitial return, and payout on a Release-configured device build. Production fill has not been established here. |
| Browser H5 | The AdSense publisher ID, H5 placement calls, reward callbacks, and `ads.txt` generation are implemented. Builds for public hosts are currently configured to show a labeled in-game promo with a visible countdown and progress bar. Completing its rewarded placement grants $100; skipping it grants nothing. Localhost also has a deterministic preview and optional Google mock mode. | Confirm the live host serves the updated ZIP and its root `/ads.txt`. Wait for the site to become Ready and for separate H5 Games Ads approval. Then turn off the house promo switch, rebuild, deploy, and test real H5 completion and no-fill behavior. |

The current policy values are deliberately test-heavy: loss cost `50`, interstitial daily limit `500`, rewarded daily limit `30`, and rewarded per-restock limit `10`. The comments in `ads_config.h` suggest approximate intended values of `5`, `5`, `3`, and `1`, respectively, but those have **not** been chosen as final values. Keep the current values until the owner decides the live policy; changing the header and rebuilding will change what the game enforces. Existing saved counters are clamped by the policy parser when loaded.

### Resume checklist after approvals

1. In AdSense, confirm the site is **Ready** and H5 Games Ads has been separately approved. Confirm the deployed site's root `/ads.txt` contains the generated `wasm/ads.txt` entry. The ZIP includes `ads.txt`, but a subdirectory upload does not make it the domain-root file.
2. Decide the final credit costs and daily/per-restock limits in `ads_config.h`. Keep `ADS_MONEY_REWARD` and the player-facing $100 reward aligned with the intended offer.
3. Set `ADS_H5_HOUSE_PROMO_WHILE_PENDING` to `0` in `ads_config.h`. Run `python3 scripts/generate_ads_config.py`, then `python3 scripts/generate_ads_config.py --check`.
4. With Emscripten activated, run `make -f Makefile.emscripten main`. Upload the complete `build/emscripten/bowling.zip` contents using the site's normal deployment process, and verify the browser loads the new `ads_config.js`. Clear or update any hosting/CDN cache that keeps the previous build.
5. On localhost, use `make -f Makefile.emscripten test` for the normal preview and `?googleMockAds=1` for Google's mock path. On the approved live site, verify an interstitial after a charged win, one after Retry on a charged loss, a completed shop reward, a skipped reward, no fill, the bank animation, and the daily/restock limits. Record whether Google actually serves ads; approval alone does not guarantee fill.
6. Test signed Android and Release-configured iOS builds with their production IDs separately. The browser switch does not change either native platform.

Local verification completed before parking: `node --test tests/test_browser_house_promo.cjs` passed; `tests/test_ad_policy.cpp` passed; `python3 scripts/generate_ads_config.py --check` and `git diff --check` passed; `make -f Makefile.emscripten main` produced `build/emscripten/bowling.zip`. These checks establish local behavior and buildability, not account approval, ad fill, or live deployment.

### Key files

- `ads_config.h`: all IDs, the browser pending-mode switch, and game policy values.
- `ad_policy.h`: credit, day, holiday, reward-limit, and saved-state rules.
- `game.cpp`: placement decisions, callback accounting, reward payout, and shop-card availability.
- `wasm/shell_itch_io.html`: H5 calls, browser promo/preview, and callback signals.
- `scripts/generate_ads_config.py`: generates `wasm/ads_config.js`, `wasm/ads.txt`, and `ios-project/generated_ads.xcconfig`.
- `android-project/app/build.gradle`, `android-project/app/src/main/java/org/libsdl/app/TrackerActivity.java`, `ios-project/ios-project/main.mm`: native ad IDs and SDK presentation/callbacks.
- `tests/test_ad_policy.cpp`, `tests/test_browser_house_promo.cjs`: policy and browser-promo checks.

## Configuration details

Edit [`ads_config.h`](../ads_config.h) for app IDs, ad unit IDs, the H5 publisher ID, rewards, credit costs, daily limits, and the ad holiday. The Ad Manager window displays the policy values from that header.

- Android Studio/Gradle reads the ad IDs directly from the header. Debug and benchmark builds use Google's sample IDs; Release uses the production IDs.
- iOS and H5 read generated adapters. Run `python3 scripts/generate_ads_config.py` after editing the header. `make -f Makefile.ios build`, `make -f Makefile.ios open`, and `make -f Makefile.emscripten main` regenerate their adapters when needed. Run `make -f Makefile.ios ads-config` first if building directly in an already-open Xcode project.
- Run `python3 scripts/generate_ads_config.py --check` to confirm that the checked-in adapters match the header.

The H5 publisher ID is an AdSense account ID and may differ from the AdMob IDs used by the native apps. The generator also creates `wasm/ads.txt` for that H5 ID, and the browser build includes it beside `index.html`. For live site verification, publish that file at the hosting domain root (`https://your-domain/ads.txt`); putting it inside a game ZIP on a third-party host does not make it that host's root file. AdSense must mark the site Ready and separately approve H5 Games Ads before live browser ads can serve.

### Browser behavior while pending

AdSense site verification and separate H5 Games Ads approval are currently **pending**. Until both are approved, `ADS_H5_HOUSE_PROMO_WHILE_PENDING` is `1`. Public browser builds show a clearly labeled in-game promo at the existing interstitial and rewarded shop placements. It displays a countdown and progress bar, and it does **not** load Google's ad script or count as a paid ad impression. Completing the rewarded promo grants `ADS_MONEY_REWARD`; skipping it grants nothing. The existing daily and per-restock reward limits, interstitial credit rules, and shop payout animation still apply. Android and iOS use their AdMob integrations as before.

After the site is Ready and H5 Games Ads is approved, set `ADS_H5_HOUSE_PROMO_WHILE_PENDING` to `0` in `ads_config.h`, rebuild with `make -f Makefile.emscripten main`, and redeploy the full browser build. Then verify a real rewarded H5 callback before relying on it for live rewards. This switch changes the public browser flow only; the stored game reward policy and save data remain in place.

`make -f Makefile.emscripten test` serves the browser game on localhost. At the normal policy-driven placements, localhost shows a deterministic preview: an interstitial waits before Continue becomes available, and a rewarded promo can be skipped without payment or watched to collect the configured reward. It makes no paid ad request. Add `?googleMockAds=1` to the localhost game URL to test Google's H5 mock ad integration instead; those mocks can report no available ad. The live house promo is labeled as a promotion for this game so players can distinguish it from a paid ad.

`ADS_MONEY_PER_RESTOCK_LIMIT` controls how many rewarded shop ads may be watched during one restock. The policy saves both the restock ID and its used reward count. It also enforces `ADS_MONEY_DAILY_LIMIT`, so the smaller remaining allowance applies. Older saves with only a restock ID count as one used reward for that restock.

AdMob's reward amount and item shown in its web console are provider-side metadata. They cannot be changed by a C++ build setting; the game grants `ADS_MONEY_REWARD` after the SDK's earned-reward callback regardless of that metadata.
