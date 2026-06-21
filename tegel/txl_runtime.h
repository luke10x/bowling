#pragma once

#include <string.h>

#include "txl_font_assets.h"
#include "generated/txl_generated.h"

enum TxlLanguage
{
    TXL_LANG_EN_US = 0,
    TXL_LANG_ZH_CN = 1,
    TXL_LANG_LT_LT = 2,
    TXL_LANG_JP_JP = 3,
    TXL_LANG_COUNT = 4,
};

static constexpr const char *k_txl_language_storage_en_us = "en_us";
static constexpr const char *k_txl_language_storage_zh_cn = "zh_cn";
static constexpr const char *k_txl_language_storage_lt_lt = "lt_lt";
static constexpr const char *k_txl_language_storage_jp_jp = "jp_jp";

static constexpr const char *k_txl_shared_chars =
    " !\"#$%&'()*+,-./0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~"
    "◀▶▼▲✓";

inline const char *Txl_Get(TxlLanguage language, TxlKey key)
{
    const int idx = (int)key;
    if (idx < 0 || idx >= TXL_KEY_COUNT)
        return "";
    switch (language)
    {
        case TXL_LANG_LT_LT:
            return g_txl_lt_lt[idx];
        case TXL_LANG_JP_JP:
            return g_txl_jp_jp[idx];
        case TXL_LANG_ZH_CN:
            return g_txl_zh_cn[idx];
        case TXL_LANG_EN_US:
        default:
            return g_txl_en_us[idx];
    }
}

inline const char *Txl_CharsForLanguage(TxlLanguage language)
{
    switch (language)
    {
        case TXL_LANG_LT_LT:
            return g_txl_chars_lt_lt;
        case TXL_LANG_JP_JP:
            return g_txl_chars_jp_jp;
        case TXL_LANG_ZH_CN:
            return g_txl_chars_zh_cn;
        case TXL_LANG_EN_US:
        default:
            return g_txl_chars_en_us;
    }
}

inline const char *Txl_LanguageStorageValue(TxlLanguage language)
{
    switch (language)
    {
        case TXL_LANG_LT_LT:
            return k_txl_language_storage_lt_lt;
        case TXL_LANG_JP_JP:
            return k_txl_language_storage_jp_jp;
        case TXL_LANG_ZH_CN:
            return k_txl_language_storage_zh_cn;
        case TXL_LANG_EN_US:
        default:
            return k_txl_language_storage_en_us;
    }
}

inline TxlLanguage Txl_LanguageFromStorage(const char *value)
{
    if (value && strcmp(value, k_txl_language_storage_lt_lt) == 0)
        return TXL_LANG_LT_LT;
    if (value && strcmp(value, k_txl_language_storage_jp_jp) == 0)
        return TXL_LANG_JP_JP;
    if (value && strcmp(value, k_txl_language_storage_zh_cn) == 0)
        return TXL_LANG_ZH_CN;
    return TXL_LANG_EN_US;
}

inline TxlEmbeddedFont Txl_UiFont(TxlLanguage language)
{
    switch (language)
    {
        case TXL_LANG_LT_LT:
            return k_txl_font_ui_lt_lt;
        case TXL_LANG_JP_JP:
            return k_txl_font_ui_jp_jp;
        case TXL_LANG_ZH_CN:
            return k_txl_font_ui_zh_cn;
        case TXL_LANG_EN_US:
        default:
            return k_txl_font_ui_en_us;
    }
}

inline TxlEmbeddedFont Txl_MonoFont()
{
    return k_txl_font_mono;
}
