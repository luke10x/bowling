#pragma once

#include "generated/txl_font_noto_sans_jp_regular.h"
#include "generated/txl_font_noto_sans_regular.h"
#include "generated/txl_font_noto_sans_sc_regular.h"
#include "generated/txl_font_roboto_mono_regular.h"
#include "generated/txl_font_roboto_regular.h"

struct TxlEmbeddedFont
{
    const unsigned char *data;
    unsigned int size;
};

static const TxlEmbeddedFont k_txl_font_ui_en_us = {
    txl_font_roboto_regular_ttf,
    txl_font_roboto_regular_ttf_len,
};

static const TxlEmbeddedFont k_txl_font_ui_zh_cn = {
    txl_font_noto_sans_sc_regular_ttf,
    txl_font_noto_sans_sc_regular_ttf_len,
};

static const TxlEmbeddedFont k_txl_font_ui_lt_lt = {
    txl_font_noto_sans_regular_ttf,
    txl_font_noto_sans_regular_ttf_len,
};

static const TxlEmbeddedFont k_txl_font_ui_jp_jp = {
    txl_font_noto_sans_jp_regular_ttf,
    txl_font_noto_sans_jp_regular_ttf_len,
};

static const TxlEmbeddedFont k_txl_font_mono = {
    txl_font_roboto_mono_regular_ttf,
    txl_font_roboto_mono_regular_ttf_len,
};

static const TxlEmbeddedFont k_txl_font_symbols = {
    txl_font_noto_sans_jp_regular_ttf,
    txl_font_noto_sans_jp_regular_ttf_len,
};
