#pragma once

// Single source for ad identity and game policy. Edit values here, then run
// `python3 scripts/generate_ads_config.py` to refresh the iOS and H5 adapters.
// Android Gradle reads the string IDs directly from this header.

// Production AdMob IDs supplied for the Android and iOS apps.
#define ADS_ANDROID_APP_ID "ca-app-pub-2747257038106344~4817748726"
#define ADS_ANDROID_INTERSTITIAL_ID "ca-app-pub-2747257038106344/9125404259"
#define ADS_ANDROID_REWARDED_ID "ca-app-pub-2747257038106344/1545552273"
#define ADS_IOS_APP_ID "ca-app-pub-2747257038106344~8933832565"
#define ADS_IOS_INTERSTITIAL_ID "ca-app-pub-2747257038106344/2790351187"
#define ADS_IOS_REWARDED_ID "ca-app-pub-2747257038106344/9232470600"

// Google's sample units are used by Debug and Android benchmark builds.
#define ADS_TEST_ANDROID_APP_ID "ca-app-pub-3940256099942544~3347511713"
#define ADS_TEST_ANDROID_INTERSTITIAL_ID "ca-app-pub-3940256099942544/1033173712"
#define ADS_TEST_ANDROID_REWARDED_ID "ca-app-pub-3940256099942544/5224354917"
#define ADS_TEST_IOS_APP_ID "ca-app-pub-3940256099942544~1458002511"
#define ADS_TEST_IOS_INTERSTITIAL_ID "ca-app-pub-3940256099942544/4411468910"
#define ADS_TEST_IOS_REWARDED_ID "ca-app-pub-3940256099942544/1712485313"

// Browser H5 Games Ad Placement settings.
#define ADS_H5_PUBLISHER_ID "ca-pub-1023905489122100"
#define ADS_H5_FREQUENCY_HINT_SECONDS 30
#define ADS_H5_LOCAL_TEST_MODE 1
// Live browser fallback while AdSense H5 Games Ads approval is pending.
// Set to 0, regenerate adapters, rebuild, and redeploy after approval.
#define ADS_H5_HOUSE_PROMO_WHILE_PENDING 1

// Game rules shown in the cheat-only Ad Manager window.
#define ADS_CREDIT_INITIAL 20 
#define ADS_CREDIT_MAX 20 
#define ADS_CREDIT_DAILY_LOGIN_BONUS 10
#define ADS_CREDIT_BUCKET_SIZE 5 
#define ADS_CREDIT_BUCKET_WIN_BONUS 0 // Disabled for now
#define ADS_CREDIT_WIN_COST 1
#define ADS_CREDIT_LOSS_COST 5 // jsut to test somethingi shoould be ~ 5
#define ADS_INTERSTITIAL_DAILY_LIMIT 5 // just  to test, normal value ~ 5
#define ADS_HOLIDAY_HOURS 2 // only 2 hour ad holiday 
#define ADS_MONEY_DAILY_LIMIT 5 // normal value 3, we retest something
#define ADS_MONEY_PER_RESTOCK_LIMIT 1 // normal 1
#define ADS_MONEY_REWARD 100

namespace AdsConfig
{
inline constexpr float CreditInitial = ADS_CREDIT_INITIAL;
inline constexpr float CreditMax = ADS_CREDIT_MAX;
inline constexpr float DailyLoginBonus = ADS_CREDIT_DAILY_LOGIN_BONUS;
inline constexpr float CreditBucketSize = ADS_CREDIT_BUCKET_SIZE;
inline constexpr float CreditBucketWinBonus = ADS_CREDIT_BUCKET_WIN_BONUS;
inline constexpr float WinCost = ADS_CREDIT_WIN_COST;
inline constexpr float LossCost = ADS_CREDIT_LOSS_COST;
inline constexpr int InterstitialDailyLimit = ADS_INTERSTITIAL_DAILY_LIMIT;
inline constexpr int HolidayHours = ADS_HOLIDAY_HOURS;
inline constexpr int MoneyDailyLimit = ADS_MONEY_DAILY_LIMIT;
inline constexpr int MoneyPerRestockLimit = ADS_MONEY_PER_RESTOCK_LIMIT;
inline constexpr int MoneyReward = ADS_MONEY_REWARD;
}
