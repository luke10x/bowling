// Tegel translation source DSL - Simplified Chinese (zh_cn)
//
// Translator guidance: ../docs/translations.md
// Campaign lore: lore.md
//
// Translation status: older entries may be substantially outdated, including
// UI text, level titles/subtitles, school copy, and unrevised dialogue. Do not
// treat existing translations as authoritative terminology or story references.
// The campaign verse was newly revised against English, Lithuanian, and lore.md;
// this does not mean the rest of this locale has been reviewed or brought current.
// When revising older entries, verify current gameplay and translator Context
// notes, then consult lore.md and ../docs/translations.md.
//
// Preferred angel names:
// Ezekiel -> 以西结; Cherubel -> 基路伯; Seraphel -> 撒拉弗;
// Thrones -> 座天使.
// Dausos is the Lithuanian spiritual realm; English calls it The Heavens.
// Do not carry the Lithuanian name Dausos into Chinese-facing text.
// Nitratai is Lithuanian-only: translate the resource as crystals/gems or
// energy according to the scene, not as nitrates. Existing UI uses
// 水晶 / 宝石, 能量, and 氮气 for the pedal.
//
// Poetic style: 古风-inspired four-line prophetic verse (文白相间).
// Use compact parallel phrases, an elevated but readable register, and selective
// rhyme; this is an adaptation, not strict 五绝/七绝 or regulated 平仄 verse.
// Names and clear gameplay meaning take priority over fixed character counts.
// The realm is 天界; the resource is 水晶 / 能量; the pedal remains 氮气.
// Reference order: lore.md governs the arc and the cruel final irony; compare
// txl_en_us.h and txl_lt_lt.h for imagery, stanza progression, and game actions.
// Borrow poetic cadence, not local deities or a replacement religious setting.
// Early speech hints at salvation; later speech reveals punishment and pride.
// School instructions, confirmations, and choice buttons stay direct and clear.
// Future poetry edits must preserve meaning, stanza structure, and Context notes.
//
// Do not use internal aliases Malach, Dog, Beak, or Cow as character names.
// The angels deceive the player into a campaign of punishment; reveal this
// gradually. Do not name God.
// Story/dialogue belongs only in the Story and dialogue section below.
// Preserve wording, punctuation, line breaks, and visible stanza structure.
// Keep the Context notes beside story entries; they identify speakers, levels,
// lessons, and triggers. Keep keys in the same order in all four locale files.
//

TXL_MENU = "菜单"; // Used in: main menu (clayton/menu_clay.h), gameplay/HUD (game.cpp)
TXL_RENAME = "作弊"; // Used in: main menu (clayton/menu_clay.h)
TXL_GO_TO_SCHOOL = "去学校"; // Used in: main menu (clayton/menu_clay.h)
TXL_LANGUAGE = "语言"; // Used in: language selector (clayton/language_clay.h), main menu (clayton/menu_clay.h)
TXL_CAMPAIGN = "战役"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_CAMPAIGN_TITLE = "战役"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_LEVELS = "战役关卡"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESTART_CAMPAIGN = "重开战役"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_WINS = "胜利"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_LOST = "失败"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_QUIT = "退出"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BEST_SCORE = "最高分"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_TIME_TO_BEAT = "通关时间"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BACK = "返回"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_REPLAY = "重玩"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PLAY = "开始"; // Used in: campaign and results windows (clayton/win_stack.h), high-score window (hiscore/hiscore_clay.h), gameplay/HUD (game.cpp)
TXL_NOT_AVAILABLE = "无"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PRACTICE = "练习"; // Used in: main menu (clayton/menu_clay.h), gameplay/HUD (game.cpp)
TXL_DEVICE_SHARE = "设备共享"; // Used in: main menu (clayton/menu_clay.h)
TXL_GAME_SETTINGS = "游戏设置"; // Used in: main menu (clayton/menu_clay.h), game settings (settings/settings_clay.h)
TXL_TRACKER = "音轨编辑"; // Used in: main menu (clayton/menu_clay.h)
TXL_CREDITS = "制作名单"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_AD_MANAGER_TITLE = "广告管理"; // 用于广告管理窗口
TXL_AD_CREDIT_FMT = "广告积分：%.1f / %.0f"; // 用于广告管理窗口
TXL_AD_CREDIT_RULES_FMT = "胜利：-%d，失败：-%d，每日登录：+%d"; // 广告规则
TXL_AD_HOLIDAY_ACTIVE = "广告假期进行中"; // 用于广告管理窗口
TXL_AD_HOLIDAY_INACTIVE = "当前没有广告假期"; // 用于广告管理窗口
TXL_AD_HOLIDAY_REMAINING_FMT = "剩余：%d 小时 %d 分钟"; // 用于广告管理窗口
TXL_AD_HOLIDAY_END_FMT = "结束时间：%s（本地时间）"; // 用于广告管理窗口
TXL_ADS_TODAY_FMT = "今日游戏广告：%d / %d"; // 用于广告管理窗口
TXL_ADS_TO_HOLIDAY_FMT = "距离假期还需广告：%d"; // 用于广告管理窗口
TXL_AD_MONEY_TODAY_FMT = "今日金币广告：%d / %d"; // 用于广告管理窗口
TXL_AD_MONEY_RULE_FMT = "奖励广告：$%d，每次补货 %d 次"; // 广告规则
TXL_AD_HOLIDAY_RULE_FMT = "观看 %d 次游戏广告后：休假 %d 小时"; // 广告规则
TXL_GET_REWARD = "领取"; // 用于商店奖励卡片
TXL_REWARD_CARD = "奖励"; // 商店奖励卡片标题
TXL_REWARD_LOADING = "奖励加载中"; // 激励广告进行中
TXL_WATCH_AD = "观看广告"; // 用于商店奖励按钮
TXL_WATCH_PROMO = "观看游戏宣传"; // H5 审核期间的替代展示
TXL_MINIGAMES = "小游戏"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_SELECT_LANGUAGE = "选择语言"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_ENGLISH = "English"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_CHINESE = "中文"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_LITHUANIAN = "立陶宛语"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_JAPANESE = "日语"; // Used in: language selector (clayton/language_clay.h)
TXL_SOUND = "声音"; // Used in: sound window (sounds/sound_clay.h), gameplay/HUD (game.cpp)
TXL_SONG = "歌曲"; // Used in: sound window (sounds/sound_clay.h)
TXL_OIL = "油道"; // Used in: oil window (oil/oil_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_BALLS = "球库"; // Used in: shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_SHOP = "商店"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_HI_SCORE = "最高分"; // Used in: gameplay/HUD (game.cpp)
TXL_PRACTICE_TITLE = "练习"; // Used in: gameplay/HUD (game.cpp)
TXL_FREESTYLE_TITLE = "自由赛"; // Used in: gameplay/HUD (game.cpp)
TXL_TRY_AGAIN = "再试一次"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_BONUS_GAME = "奖励关"; // Used in: gameplay/HUD (game.cpp)
TXL_BALL_CHARGE_FMT = "能量"; // Used in: gameplay/HUD (game.cpp)
TXL_NOS_NO_ENERGY = "能量不足。拾取宝石。"; // Used in: gameplay/HUD (game.cpp)
TXL_GREETING_RESUME = "你想继续打保龄球吗？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_GREETING_WELCOME = "欢迎来到这款保龄球游戏。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_GREETING_SUBLINE = "点击“准备”以聚焦游戏画面。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_READY = "准备"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SELECT_ANGEL = "选择天使"; // Used in: gameplay/HUD (game.cpp)
TXL_SELECT_BOT = "选择对手"; // Used in: bot selection (bots/bots_clay.h), gameplay/HUD (game.cpp)
TXL_SELECT_HOUSE = "选择球道"; // Used in: gameplay/HUD (game.cpp)
TXL_SWITCH_HOUSE = "切换球道"; // Used in: house selection (houses/houses_clay.h), gameplay/HUD (game.cpp)
TXL_SELECT_BALL = "选择球"; // Used in: gameplay/HUD (game.cpp)
TXL_SELL_FOR_FMT = "出售得 $ %d"; // 用于库存出售按钮
TXL_SELL_CONFIRM_TITLE = "出售球"; // 用于出售确认
TXL_SELL_CONFIRM_ITEM_FMT = "出售 %s，获得 $%d？"; // 用于出售确认
TXL_SELL_CONFIRM_LOSS_FMT = "比原价亏损 $%d。"; // 用于出售确认
TXL_SELL_CONFIRM_GAIN_FMT = "比原价收益 $%d。"; // 用于出售确认
TXL_CURRENT = "当前使用"; // Used in: shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_INVENTORY = "库存"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_BUY_NOW = "立即购买"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_HOUSES = "球道"; // Used in: house selection (houses/houses_clay.h)
TXL_SELECTED_FMT = "已选择：%s"; // Used in: house selection (houses/houses_clay.h), bot selection (bots/bots_clay.h)
TXL_SELECT_A_HOUSE = "选择一个球道"; // Used in: house selection (houses/houses_clay.h)
TXL_NO_HOUSES = "没有球道"; // Used in: house selection (houses/houses_clay.h)
TXL_BOT_AVATAR = "对手形象"; // Used in: bot selection (bots/bots_clay.h)
TXL_SELECT_A_BOT_AVATAR = "选择一个对手形象"; // Used in: bot selection (bots/bots_clay.h)
TXL_NO_BOTS = "没有对手"; // Used in: bot selection (bots/bots_clay.h)
TXL_INVENTORY_EMPTY = "库存为空"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_EMPTY = "商店已空"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h)
TXL_LOCKED = "未解锁"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PASS = "通过"; // Used in: campaign and results windows (clayton/win_stack.h), school window (school/school_clay.h)
TXL_RESETS_IN_FMT = "%s后重置"; // Used in: shop window (clayton/shop_clay.h)
TXL_SOUND_SETTINGS = "声音设置"; // Used in: sound window (sounds/sound_clay.h)
TXL_CHANGING_QUALITY = "正在切换音频模式..."; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_MODE = "音频模式"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_SYNTH = "合成器"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_OFF = "关闭"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_PANIC = "急停"; // Used in: sound window (sounds/sound_clay.h)
TXL_SDL_BUFFER = "SDL 缓冲"; // Used in: sound window (sounds/sound_clay.h)
TXL_MUSIC_VOLUME = "音乐音量"; // Used in: sound window (sounds/sound_clay.h)
TXL_OIL_STATUS = "油道状态"; // Used in: oil window (oil/oil_clay.h)
TXL_LANE_OIL_MAP = "油图"; // Used in: oil window (oil/oil_clay.h)
TXL_TRACK_INFO = "球道信息"; // Used in: oil window (oil/oil_clay.h)
TXL_MAX_OIL_LEVEL = "最大油量"; // Used in: oil window (oil/oil_clay.h)
TXL_CURRENT_OIL_LEVEL = "当前"; // Used in: oil window (oil/oil_clay.h)
TXL_CARRYDOWN = "带油"; // Used in: oil window (oil/oil_clay.h)
TXL_OIL_DECAY = "衰减"; // Used in: oil window (oil/oil_clay.h)
TXL_SLIPPERY = "滑度"; // Used in: oil window (oil/oil_clay.h)
TXL_PUSHBACK = "回推力"; // Used in: oil window (oil/oil_clay.h)
TXL_REOILS_FMT = "重新上油：%d/%d"; // Used in: oil window (oil/oil_clay.h)
TXL_REOIL = "重新上油"; // Used in: oil window (oil/oil_clay.h), school window (school/school_clay.h)
TXL_CANT_AFFORD = "余额不足"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), oil window (oil/oil_clay.h)
TXL_REOIL_LOCKED = "暂时不能上油"; // Used in: oil window (oil/oil_clay.h)
TXL_FRESHLY_OILED = "刚上过油"; // Used in: gameplay/HUD (game.cpp)
TXL_TOP_SCORES = "最高分"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_YOUR_SCORE_TOP_FMT = "你的分数进入前%d名"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_GOOD_RUN = "不错的一局！"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_POINTS_FMT = "%d分"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_SCORE_HIGHER_THAN_FMT = "你的分数超过了近期%.0f%%的对局"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_LEADERBOARD_LAST_HOUR = "排行榜（最近一小时）"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_PLAYER = "玩家"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_SCORE = "分数"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_AGE = "时间"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_NO_SCORES_YET = "还没有分数，来当第一个吧！"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_ATTEMPTS_FMT = "尝试次数：%d"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_RANGE_FMT = "范围：%d-%d"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_GAME_SETTINGS_TITLE = "游戏设置"; // Used in: game settings (settings/settings_clay.h)
TXL_HIGH_DETAILS = "高细节"; // Used in: game settings (settings/settings_clay.h)
TXL_HIGH_DETAILS_HELP = "更多画面细节和装饰特效"; // Used in: game settings (settings/settings_clay.h)
TXL_PATCHED_YMFM = "改进版 YMFM"; // Used in: game settings (settings/settings_clay.h)
TXL_PATCHED_YMFM_HELP = "使用实验性高速 FM 合成器；音频将重新启动"; // Used in: game settings (settings/settings_clay.h)
TXL_PERFORMANCE_ANALYSIS = "性能分析"; // Used in: game settings (settings/settings_clay.h)
TXL_PERFORMANCE_ANALYSIS_HELP = "帧耗时图及每5秒一次的计时报告（F3）"; // Used in: game settings (settings/settings_clay.h)
TXL_RESET_PROGRESS = "恢复出厂设置"; // Used in: campaign and results windows (clayton/win_stack.h), game settings (settings/settings_clay.h)
TXL_RESET_CAMPAIGN = "重开战役"; // Used in: game settings (settings/settings_clay.h)
TXL_RESET_PROGRESS_HELP = "重开战役会清除战役状态，包括关卡记录、球、解锁和符文。\n恢复出厂设置还会清除应用设置。"; // Used in: game settings (settings/settings_clay.h)
TXL_CHECK_FOR_UPDATE = "检查更新"; // Used in: game settings (settings/settings_clay.h)
TXL_UPDATE_PWA = "更新 PWA"; // Used in: game settings (settings/settings_clay.h)
TXL_UPDATE_WEB = "重新加载网站"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_INSTALLED_FMT = "已安装构建：%s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_PUBLISHED_FMT = "已发布构建：%s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_CHECKING = "正在检查更新……"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_UP_TO_DATE_FMT = "已是最新。已发布构建：%s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_AVAILABLE_FMT = "有可用更新：%s -> %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_OFFLINE = "当前离线。请连接网络后检查更新。"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_ERROR = "无法连接更新服务器。"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_APPLYING = "正在应用更新……"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_UNSUPPORTED = "只有网页应用支持 PWA 更新检查。"; // Used in: game settings (settings/settings_clay.h)
TXL_CREDITS_AUTHOR_FMT = "游戏设计、程序、美术和声音：%s。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_AUTHOR_NAME = "Lape Kale"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_CHARACTER_BASE = "角色基础模型：\nOpenGameArt.org 的\nVery Low Poly Human。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_CHEST = "宝箱模型：CDmir，\nOpenGameArt.org（CC0）。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_MIXAMO = "角色动画来自 Mixamo。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_TECH = "使用 SDL、OpenGL ES、Clay、GLM、\nJolt Physics 和 ymfm 构建。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_TRACKER = "所有音乐和音效均由\n游戏内置的追踪器制作。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_THANKS = "特别感谢每一位游玩、测试，\n并继续多投一局的人。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SCHOOL_TITLE_FMT = "第%d课：%s"; // Used in: school window (school/school_clay.h)
TXL_AIM_LESSON = "瞄准课"; // Used in: school window (school/school_clay.h)
TXL_BALL_MASS = "球的质量"; // Used in: school window (school/school_clay.h)
TXL_SPIN_BALL = "给球上旋"; // Used in: school window (school/school_clay.h)
TXL_OIL_AND_SKID = "油与滑行"; // Used in: school window (school/school_clay.h)
TXL_STRIKE_LINE = "全中线路"; // Used in: school window (school/school_clay.h)
TXL_MASS_FMT = "质量（%.1f公斤）"; // Used in: school window (school/school_clay.h)
TXL_LIGHT_BALL_TEST = "轻球测试"; // Used in: school window (school/school_clay.h)
TXL_HEAVY_BALL_TEST = "重球测试"; // Used in: school window (school/school_clay.h)
TXL_PASSED = "已通过"; // Used in: school window (school/school_clay.h)
TXL_MASS = "质量"; // Used in: shop window (clayton/shop_clay.h), school window (school/school_clay.h)
TXL_SPIN = "旋转"; // Used in: shop window (clayton/shop_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_SKID = "滑行"; // Used in: shop window (clayton/shop_clay.h)
TXL_BITE = "咬道"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_COMMON = "普通"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_RARE = "稀有"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_EPIC = "史诗"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_LEGENDARY = "传说"; // Used in: shop window (clayton/shop_clay.h)
TXL_SPIN_TEST_FMT = "旋转测试：%d/%d 目标（第%d/%d关）"; // Used in: school window (school/school_clay.h)
TXL_AIM_LESSON_POINTS_FMT = "瞄准课：%d/%d分"; // Used in: school window (school/school_clay.h)
TXL_PULL_BALL_BACK = "把球尽量向后拉"; // Used in: school window (school/school_clay.h)
TXL_LET_IT_GO_NOW = "现在放手"; // Used in: school window (school/school_clay.h)
TXL_MOVE_RIGHT = "向右移动"; // Used in: school window (school/school_clay.h)
TXL_MOVE_LEFT = "向左移动"; // Used in: school window (school/school_clay.h)
TXL_OIL_TEST_REOILS_FMT = "油道测试：重新上油 %d/%d"; // Used in: school window (school/school_clay.h)
TXL_REMAINING_OIL = "剩余油量"; // Used in: school window (school/school_clay.h)
TXL_REOIL_PROGRESS = "上油进度"; // Used in: school window (school/school_clay.h)
TXL_WEAR_IT_DOWN = "先把油耗掉，再重新上油。"; // Used in: school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_STRIKE_TEST_SCORE_STRIKE = "全中测试：打出全中"; // Used in: school window (school/school_clay.h)
TXL_LEVEL1_TITLE = "第1关  第一个里程碑"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL1_SUBTITLE = "普通球道  达到100分即可通过"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL2_TITLE = "第2关  以西结到来"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL2_SUBTITLE = "风电场  击败以西结"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL3_TITLE = "第3关  沙漠警告"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL3_SUBTITLE = "沙漠球道  击败以西结"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL4_TITLE = "第4关  玻璃冰面"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL4_SUBTITLE = "冰面球道  击败以西结"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL5_TITLE = "第5关  霓虹玻璃课"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL5_SUBTITLE = "郊区球道  击败以西结"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL6_TITLE = "第6关  霓虹里的狗"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL6_SUBTITLE = "霓虹球道  击败狗"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL7_TITLE = "第7关  力量金字塔"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL7_SUBTITLE = "沙漠球道  击败狗"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL8_TITLE = "第8关  木材地带"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL8_SUBTITLE = "森林球道  击败狗"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL9_TITLE = "第9关  水晶觐见"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL9_SUBTITLE = "水晶洞窟  击败喙"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL10_TITLE = "第10关  煤气工厂告白"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL10_SUBTITLE = "煤气工厂  击败喙"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL11_TITLE = "第11关  墓园"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL11_SUBTITLE = "灰色沙漠  击败喙"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL12_TITLE = "第12关  城市之轮"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL12_SUBTITLE = "废墟城市球道  击败牛"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL13_TITLE = "第13关  灰烬游行"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL13_SUBTITLE = "灰烬荒原  击败牛"; // Used in: gameplay/HUD (game.cpp)
TXL_BALL_NAME_0 = "余烬强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_1 = "烈焰滚"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_2 = "火焰钩"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_3 = "炼狱怒"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_4 = "凤凰升"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_5 = "霜滑"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_6 = "寒意强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_7 = "极地漂移"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_8 = "冰川咬道"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_9 = "暴雪王"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_10 = "星尘"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_11 = "星云滚"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_12 = "宇宙钩"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_13 = "虚空强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_14 = "黑洞"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_15 = "UFO旋"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_16 = "氙光咬道"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_17 = "森林滚"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_18 = "藤蔓强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_19 = "雷橡"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_20 = "泰坦根"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_21 = "世界树"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_22 = "暗影漂移"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_23 = "深渊钩"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_24 = "归零者"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_25 = "电路滚"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_26 = "霓虹强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_27 = "量子钩"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_28 = "奇点"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_29 = "潮汐滚"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_30 = "礁石强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_31 = "海妖咬道"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_32 = "利维坦"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_33 = "符文球"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_34 = "神谕强击"; // Used in: shop window (clayton/shop_clay.h)
TXL_YOU = "你"; // Used in: campaign and results windows (clayton/win_stack.h), new-game window (clayton/new_game_clay.h), high-score window (hiscore/hiscore_clay.h), gameplay/HUD (game.cpp)
TXL_BONUS_ROUND = "奖励局"; // Used in: gameplay/HUD (game.cpp)
TXL_TUTORIAL_FMT = "课程 %d"; // Used in: gameplay/HUD (game.cpp)
TXL_COMPLETION_TIME_FMT = "通关时间 %s"; // Used in: gameplay/HUD (game.cpp)
TXL_AUDIO_BUFFER_INCREASED_FMT = "性能偏低，音频缓冲已调至%d"; // Used in: gameplay/HUD (game.cpp)
TXL_SPIN_TO_HOOK = "旋转可弯球"; // Used in: gameplay/HUD (game.cpp)
TXL_DROP_HERE_TO_USE = "拖到这里\n使用"; // Used in: gameplay/HUD (game.cpp)
TXL_CANNOT_USE_NOW = "现在\n不能用"; // Used in: gameplay/HUD (game.cpp)
TXL_NEED_AT_LEAST_2_BALLS = "至少需要\n2个球"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_DEFAULT = "符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_LOST = "球已爆炸"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_EVAPORATED = "球已蒸发"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_SURVIVED = "球扛过闪光"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_PINS_FROZEN = "球瓶冻结"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_PATROL_DEPLOYED = "巡逻瓶已部署"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_FOOTBALL = "足球已激活"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_FLASH = "闪光已激活"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_SKULL = "骷髅已激活"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_ICY_BALL = "冰球已激活"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BOOM = "爆炸已激活"; // Used in: gameplay/HUD (game.cpp)
TXL_MISSED = "未命中"; // Used in: gameplay/HUD (game.cpp)
TXL_STALLED = "停住"; // Used in: gameplay/HUD (game.cpp)
TXL_SPLIT = "分瓶"; // Used in: gameplay/HUD (game.cpp)
TXL_STRIKE = "全中"; // Used in: new-game window (clayton/new_game_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_SPARE = "补中"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_BLOCK_WOOD = "木块"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_BRICK = "砖块"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_CONCRETE = "混凝土"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_GLASS = "玻璃"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_TITLE = "宝箱"; // Used in: gameplay/HUD (game.cpp)
TXL_TAP_TO_OPEN_CHEST = "点击打开宝箱"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_RUNE_PICKED_FMT = "获得能力：%s"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_COINS_PICKED_FMT = "获得%d枚金币"; // Used in: gameplay/HUD (game.cpp)
TXL_CONTINUE = "继续"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_RUNE_NAME_BOOM = "爆炸符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_BOLT = "闪电符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_FREEZE = "冰冻符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_SKULL = "骷髅符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_GUARD_PINS = "巡逻瓶符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_FOOTBALL = "足球符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_DEFAULT = "符文"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_BOOM = "爆炸投球，冲击强但有失球风险。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_BOLT = "防守闪电会蒸发敌方球。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_FREEZE = "防守冻瓶，进攻让球变冰。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_SKULL = "骷髅球：击瓶更狠，破障更强。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_GUARD_PINS = "部署三个巡逻瓶阻挡对手。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_FOOTBALL = "立刻弹起球，并让它更轻更快。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_DEFAULT = "向符文栏添加一个能力。"; // Used in: gameplay/HUD (game.cpp)
TXL_MINIGAMES_HELP = "测试入口。奖励有效，故事不推进。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_MINIGAME_COIN_RUSH = "金币冲刺"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_MINIGAME_COUNT_MASTERS = "数数大师"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_MINIGAME_CROWD_CONTROL = "人群控制"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_BONUS_PROMPT_FMT = "要玩奖励局吗？%s"; // Used in: gameplay/HUD (game.cpp)
TXL_RESET_PROGRESS_CONFIRM_TITLE = "重置进度"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ARE_YOU_SURE = "确定吗？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESET_PROGRESS_FULL_DETAIL = "会清除战役状态和应用设置。\n包括关卡记录、球、解锁和符文。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESET_PROGRESS_SOFT_DETAIL = "会清除战役状态。\n包括关卡记录、球、解锁和符文。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_NO = "否"; // Used in: campaign and results windows (clayton/win_stack.h), house selection (houses/houses_clay.h), high-score window (hiscore/hiscore_clay.h), bot selection (bots/bots_clay.h), gameplay/HUD (game.cpp)
TXL_YES = "是"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_EXIT_BONUS_LEVEL = "退出奖励关"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_EXIT_BONUS_LEVEL_CONFIRM = "要退出进行中的奖励关吗？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_STRIKES = "全中"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_SPARES = "补中"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_COINS = "金币"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_WON_BY_POINTS = "分数胜利"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_TOTAL = "合计"; // Used in: campaign and results windows (clayton/win_stack.h), new-game window (clayton/new_game_clay.h)
TXL_COINS_OBTAINED = "获得金币"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_VICTORY = "胜利"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_YOU_LOSE = "失败"; // Used in: gameplay/HUD (game.cpp)
TXL_NEXT = "下一步"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_RETRY = "重试"; // Used in: gameplay/HUD (game.cpp)
TXL_REPEAT = "再来"; // Used in: gameplay/HUD (game.cpp)
TXL_INVENTORY_SHORT = "库存"; // Used in: gameplay/HUD (game.cpp)
TXL_RELOADS_IN_FMT = "%s后补货"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_RESTOCK_SOON = "即将补货"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_BONUS_LEVEL = "奖励关"; // Used in: gameplay/HUD (game.cpp)
TXL_PLAY_BONUS = "开始奖励关"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_COMPLETE = "奖励局完成"; // Used in: gameplay/HUD (game.cpp)
TXL_COINS_PICKED_EARNED_FMT = "拾取金币：%d   获得：$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_COMPLETE_FMT = "奖励完成  +$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_EXITED_EARNED_FMT = "中途退出。已获得：$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_EXITED_FMT = "奖励局已退出  +$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_VICTORY = "奖励局胜利"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_DEFEAT = "奖励局失败"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_LOST = "奖励局失败"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_VICTORY_BASE = "胜利：守卫到达敌方基地。"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_DEFEAT_SPAWN = "失败：敌人到达你的出生点。"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_SPAWN_LABEL = "生成"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_POWER_LABEL = "战力"; // Used in: gameplay/HUD (game.cpp)
TXL_VICTORY_DETAIL = "胜利。"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_DEFEAT_DETAIL = "失败。"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_ENEMIES = "敌人"; // Used in: gameplay/HUD (game.cpp)
TXL_BOSSES = "首领"; // Used in: gameplay/HUD (game.cpp)
TXL_SUPER_BOSS = "超级首领"; // Used in: gameplay/HUD (game.cpp)
TXL_GATES = "门"; // Used in: gameplay/HUD (game.cpp)
TXL_PINS = "球瓶"; // Used in: gameplay/HUD (game.cpp)
TXL_STANDERS = "剩余"; // Used in: gameplay/HUD (game.cpp)
TXL_YOU_BEAT_ANGEL = "你击败了天使"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ANGEL_WINS = "天使获胜"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BOT_SCORE_FMT = "你：%d    天使：%d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_COMPLETE = "战役完成"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_COMPLETE_DETAIL = "所有关卡已清。球道归你了。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_TOTAL_TIME_FMT = "总时间  %s"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ATTEMPTS_TOTAL_FMT = "尝试  %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ENTER_CHEAT = "输入作弊码"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEAT_ACTIVATED = "作弊已启用"; // Used in: gameplay/HUD (game.cpp)
TXL_NO_SUCH_CHEAT = "没有这个作弊码"; // Used in: gameplay/HUD (game.cpp)
TXL_MAKES_YOU_SPAWN_FAST = "生成更快"; // Used in: gameplay/HUD (game.cpp)
TXL_MAKES_YOU_STRONG = "力量更强"; // Used in: gameplay/HUD (game.cpp)
TXL_TURN_FMT = "%s回合"; // Used in: gameplay/HUD (game.cpp)
TXL_NOS = "氮气"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_AVAILABLE = "奖励可用"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_UPDATED = "商店已更新"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_RESTOCK_PROMPT = "新球已上架。下局前要去商店看看吗？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_VISIT_SHOP = "去商店"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_LATER = "稍后"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_KEYPAD_ENTER_TEXT = "输入文字"; // Used in: keypad HUD (clayton/keypad.h)
TXL_KEYPAD_ENTER_NUMBER = "输入数字"; // Used in: numeric keypad (clayton/numkeypad.h)
TXL_DELETE = "删除"; // Used in: numeric keypad (clayton/numkeypad.h), keypad HUD (clayton/keypad.h)
TXL_ENTER = "确认"; // Used in: numeric keypad (clayton/numkeypad.h), keypad HUD (clayton/keypad.h), gameplay/HUD (game.cpp)
TXL_STRESS = "重音"; // Used in: keypad HUD (clayton/keypad.h)
TXL_POST_CAMPAIGN = "战役后"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SETTINGS = "设置"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_SETTINGS = "战役后设置"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_BEST_SCORE = "最高分："; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_WON_FMT = "胜利 %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_LOST_FMT = "失败 %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_QUIT_FMT = "退出 %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BLOCKS = "障碍"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BLOCKS_SHORT = "挡"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_NOS_SHORT = "氮"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RUNES = "符文"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RUNES_SHORT = "符"; // Used in: campaign and results windows (clayton/win_stack.h)

// -----------------------------------------------------------------------------
// Story and dialogue
// -----------------------------------------------------------------------------
TXL_STORY_1 =
    "迷途之魂，今入天界之门。\n"
    "研习符文，及时取得百分。\n"
    "全中补中，自有前路可寻。\n"
    "细数每记，莫使侥幸欺心。\n"; // Context: Speaker: Ezekiel. Level 1's first-attempt introduction.
TXL_STORY_2 = "可愿先入学堂修习？\n"; // Context: Speaker: Ezekiel. Follow-up prompt offering training from the first Level 1 introduction.
TXL_STORY_10 =
    "今日未能取百分，\n"
    "想是骄心误汝行。\n"
    "且入学堂重习路，\n"
    "归来稳手定前程。\n"; // Context: Speaker: Ezekiel. Level 1 failure message after scoring below 100.
TXL_STORY_11 = "如今先入学堂，还是再闯天界第一关？\n"; // Context: Speaker: Ezekiel. Choice after the Level 1 failure message: train or replay.
// Context: Speaker: Ezekiel (not yet seen by the player). After winning Level 1 with 100+ points while school is unfinished; promises an upcoming meeting. Automatically continues to story 21 for the training-or-Level-2 choice.
TXL_STORY_20 =
    "你求取分数之时，未曾见吾身影。\n"
    "投球颇为精准，已显不凡技艺。\n"
    "那一段时光，吾始终默然注视。\n"
    "不久之后，你亦将见吾现身。\n";
TXL_STORY_21 = "先行修习，还是此刻迎战以西结？\n"; // Context: Speaker: Ezekiel. Immediately after story 20, before Level 2: choose training or facing Ezekiel.
// Context: Speaker: Ezekiel (not yet seen by the player). After winning Level 1 with 100+ points when school is already complete; promises an upcoming meeting. Replaces stories 20 and 21, without offering training again.
TXL_STORY_22 =
    "你求取分数之时，未曾见吾身影。\n"
    "投球颇为精准，已显不凡技艺。\n"
    "那一段时光，吾始终默然注视。\n"
    "不久之后，你亦将见吾现身。\n";
TXL_STORY_30 =
    "你现在还不能离开学校。\n"
    "先把课程完成。\n"; // Context: Speaker: Ezekiel. Attempt to leave mandatory training before completing it.
TXL_STORY_31 =
    "确定吗？\n"
    "这会取消当前课程尝试，并开始你选择的课程。\n"; // Context: Speaker: Ezekiel. Confirmation before replacing the current training attempt.
TXL_STORY_32 =
    "重新开始当前课程？\n"
    "本课程的当前尝试会被重置。\n"; // Context: Speaker: Ezekiel. Confirmation before restarting the current training lesson.
TXL_STORY_1000 =
    "第2课：球的质量。\n"
    "这里讲的是质量。\n"
    "每个球都有自己的质量，质量会改变它的手感和滚动方式。\n"
    "你的第一个测试，是用几颗轻球击中球瓶。\n"
    "而想毕业，你也必须用重球击中球瓶。\n"; // Context: Speaker: Ezekiel. Lesson 2 introduction: ball mass.
TXL_STORY_1010 = "很好！你通过了质量测试。\n"; // Context: Speaker: Ezekiel. Lesson 2 completion.
TXL_STORY_1012 =
    "要通过这个测试，你必须把质量滑块调到最轻或最重的一端。\n"
    "停在中间不会算进通过进度。\n"; // Context: Speaker: Ezekiel. Lesson 2 reminder to use a light or heavy ball.
TXL_STORY_1013 = "很好。现在把滑块切到重球一端，再去击中球瓶。\n"; // Context: Speaker: Ezekiel. Lesson 2 prompt to use the heavy ball.
TXL_STORY_1014 = "很好。现在把滑块切到轻球一端，再去击中球瓶。\n"; // Context: Speaker: Ezekiel. Lesson 2 prompt to use the light ball.
TXL_STORY_1030 = "你随时都可以回到学校。\n"; // Context: Speaker: Ezekiel. General return-to-school message after a lesson.
TXL_STORY_1020 =
    "恭喜。你通过了旋转测试。\n"
    "每颗球对旋转都有自己天生的反应。\n"
    "像咬道这样的参数，也会影响它对旋转的响应程度。\n"; // Context: Speaker: Ezekiel. Lesson 3 completion: spin.
TXL_STORY_1022 =
    "第3课：给球上旋。\n"
    "在球出手后，立刻在屏幕上做旋转动作给球加旋。\n"
    "这样球就会开始朝某个方向发力。\n"
    "击倒所有轻量目标球瓶才能通过（共2关）。\n"; // Context: Speaker: Ezekiel. Lesson 3 introduction: ball spin.
TXL_STORY_1052 =
    "第4课：油与滑行。\n"
    "这条球道刚刚上过油，大约有半条到三分之二的长度都覆盖着满油。\n"
    "这一课里油会消耗得很快，所以打几球之后手感会明显改变。\n"
    "有些球馆本身就更滑，而球也有自己的滑行参数。\n"; // Context: Speaker: Ezekiel. Lesson 4 introduction: oil and skid.
TXL_STORY_1054 =
    "现在球道需要重新上油了。\n"
    "打开油道窗口，先重新上油，再继续投球。\n"; // Context: Speaker: Ezekiel. Lesson 4 prompt when the lane needs fresh oil.
TXL_STORY_1060 = "很好！你通过了油道测试。\n"; // Context: Speaker: Ezekiel. Lesson 4 completion.
TXL_STORY_1032 =
    "第1课：瞄准课。\n"
    "现在我们来学习如何出手。\n"
    "把球尽量往后拉，保持在中间，然后放手。\n"
    "只要击中任何球瓶，你就能得1分。\n"; // Context: Speaker: Ezekiel. Lesson 1 introduction: aiming.
TXL_STORY_1040 = "很好！你通过了瞄准测试。\n"; // Context: Speaker: Ezekiel. Lesson 1 completion.
TXL_STORY_1070 =
    "第5课：全中线路。\n"
    "跟着金币走。那条线会先从中间弯开，再回到口袋位。\n"
    "你的目标，是打出一次全中。\n"
    "你可以切换线路，练习另一侧口袋。\n"; // Context: Speaker: Ezekiel. Lesson 5 introduction: strike line.
TXL_STORY_1072 =
    "漂亮！全中。\n"
    "你从学校毕业了。\n"
    "你随时都可以回来。\n"; // Context: Speaker: Ezekiel. Lesson 5 and school completion.
TXL_STORY_1080 =
    "我看得出你有点吃力。\n"
    "要不要试试这颗球？\n"; // Context: Speaker: Ezekiel. Suggested ball after the player struggles in training.
TXL_STORY_1021 =
    "你可以继续在之字形金币那一课练更多旋转。\n"
    "第3课已经解锁。\n"; // Context: Speaker: Ezekiel. Lesson 3 unlock notification after the zig-zag gold lesson.
TXL_STORY_40 =
    "吾乃以西结，曾见你起落。\n"
    "天界默然，符文藏于其间。\n"
    "取得百分，第一道门方启。\n"
    "若需引路，且入学堂磨炼。\n"; // Context: Speaker: Ezekiel. Level 1 replay introduction after a failed attempt, before training is finished.

// Context: Speaker: Ezekiel. Before Level 2 (Wind Farm): Ezekiel introduces himself and promises that the player can save the world; the promise is deceptive (see lore.md).
TXL_STORY_3002 =
    "吾乃以西结，首位天使，自有傲骨。\n"
    "你的一投一掷，吾曾尽收眼底。\n"
    "你的天资，将助此世重获拯救。\n"
    "罪与欺妄，早已将它占据。\n";
TXL_STORY_3102 =
    "手稳而目明，此局已显锋芒。\n"
    "球循既定之约，稳过漫长球道。\n"
    "离此温和之地，随吾向荒漠。\n"
    "沙下深藏的，尽是旧日罪痕。\n"; // Context: Speaker: Ezekiel. Level 2 completion, leading to the Red Desert.
TXL_STORY_3003 =
    "罪使此地焦渴，烈日灼遍荒沙。\n"
    "尘土壅塞球道，滚动亦成重负。\n"
    "以油与乳香，涂抹这条长路。\n"
    "莫让涤罪之火，空烧你一身骄气。\n"; // Context: Speaker: Ezekiel. Level 3 (Red Desert) introduction.
TXL_STORY_30031 =
    "正中直撞，球瓶便向两旁分。\n"
    "中间空隙，留作下一投的伤痕。\n"
    "何不侧入，使弧线转向瓶群？\n"
    "改此一路，或能免去分瓶之困。\n"; // Context: Speaker: Ezekiel. Level 3 side-throw harassment after repeated splits.
TXL_STORY_30032 =
    "荒漠在前，你却不曾补油。\n"
    "干燥的球道，记着每一分自负。\n"
    "先润此路，莫待傲气受苦。\n"
    "可要吾此刻，为你开启油道窗口？\n"; // Context: Speaker: Ezekiel. Level 3 oil reminder when the player has not re-oiled.
TXL_STORY_3042 =
    "碎玻璃来，如一场锋利的雨。\n"
    "这般晶亮之痛，你亦可还予吾。\n"
    "球击碎片，速度便有所失。\n"
    "认准时机，轮到吾时便投回。\n"; // Context: Speaker: Ezekiel. Level 5 glass reminder when the player has not thrown glass back.
TXL_STORY_3050 =
    "符文已入你手，莫任其力长眠。\n"
    "拖入球的旋转圆环，方可施法。\n"
    "留待后用，或此刻唤醒其力？\n"
    "取舍在你，且审眼前之局。\n"; // Context: Speaker: Ezekiel. First rune discovery during the campaign: generic guidance on dragging a rune into the casting ring.
TXL_STORY_3051 =
    "此符能令球，迸作一声雷鸣。\n"
    "以你一球之损，换敌方受创。\n"
    "取舍之前，先称这一价。\n"
    "爆裂能破局，也将毁去你的球。\n"; // Context: Speaker: Ezekiel. Follow-up tutorial after the player first finds a Boom rune.
TXL_STORY_3052 =
    "你手中之符，藏着炽烈雷光。\n"
    "能焚吾滚动之球，使长投中断。\n"
    "然吾见险，亦会设法闪避。\n"
    "择时而发，莫将雷霆空掷。\n"; // Context: Speaker: Ezekiel. Follow-up tutorial after the player first finds a Bolt rune.
TXL_STORY_3053 =
    "符文已入你手，莫任其力长眠。\n"
    "拖入球的旋转圆环，方可施法。\n"
    "留待后用，或此刻唤醒其力？\n"
    "取舍在你，且审眼前之局。\n"; // Context: Speaker: Ezekiel. First Boom rune discovery during the campaign: casting-ring guidance, automatically followed by the Boom warning in story 3051.
TXL_STORY_3054 =
    "符文已入你手，莫任其力长眠。\n"
    "拖入球的旋转圆环，方可施法。\n"
    "留待后用，或此刻唤醒其力？\n"
    "取舍在你，且审眼前之局。\n"; // Context: Speaker: Ezekiel. First Bolt rune discovery during the campaign: casting-ring guidance, automatically followed by the Bolt warning in story 3052.
TXL_STORY_3103 =
    "热浪与飞沙，未能阻你前行。\n"
    "如今寒意将至，脚下再无安稳。\n"
    "冰路之上，许诺也会滑落。\n"
    "守住方向，莫让寒冰夺去立足之地。\n"; // Context: Speaker: Ezekiel. Level 3 completion, leading to the icy Level 4.
TXL_STORY_3004 =
    "寒意所至，万物失其依凭。\n"
    "灵魂亦会滑离，本可安住之境。\n"
    "孕育之处、道路与球道，皆难持守。\n"
    "谨守直路，莫随疏忽滑入寒深。\n"; // Context: Speaker: Ezekiel. Level 4 introduction.
TXL_STORY_3104 =
    "你未曾滑落，此路仍在手中。\n"
    "前方破碎的玻璃，预告一座城的倾覆。\n"
    "一扇窗先破，满城便有了裂隙。\n"
    "越过那道光，蛾摩拉的命数已定。\n"; // Context: Speaker: Ezekiel. Level 4 completion, leading to the broken-glass level.
TXL_STORY_3040 =
    "往那座城，吾只能行至此处。\n"
    "玻璃彼岸，另有天使掌其疆土。\n"
    "吾之职责将尽，基路伯之境将始。\n"
    "且去，破窗所开，绝非太平之路。\n"; // Context: Speaker: Ezekiel. Before Level 5 (Suburbia): Ezekiel introduces the broken-glass lesson; Cherubel is encountered in Level 6.
TXL_STORY_3041 =
    "你已碰到玻璃，此课从此开篇。\n"
    "待轮到吾投球，你便可将碎片投还。\n"
    "留意回合之钮，认准还击时刻。\n"
    "投出碎片，让裁断随之而落。\n"; // Context: Speaker: Ezekiel. Level 5 first-glass tutorial.
TXL_STORY_3140 =
    "破窗之后，吾之教导至此而终。\n"
    "基路伯正等你，教你更严酷的局。\n"
    "去见蛾摩拉门前，守城的天使。\n"
    "此城已在召唤，清算不容再迟。\n"; // Context: Speaker: Ezekiel. Level 5 completion, before meeting Cherub.
TXL_STORY_3141 =
    "见他之前，且往商人堂中。\n"
    "每一颗球，各有不同秉性。\n"
    "按你的手法选球，再赴更艰难之局。\n"
    "可要吾为你，此刻打开商店？\n"; // Context: Speaker: Ezekiel. First shop prompt before the Cherub encounter.
TXL_STORY_3005 =
    "吾乃基路伯。是谁步入蛾摩拉？\n"
    "此城罪孽生根，竟自称理所应当。\n"
    "破窗所获，终化作水晶之光。\n"
    "收取它们，清算尚需更多力量。\n"; // Context: Speaker: Cherubel. Level 6 (Gomorrah) introduction by Cherub.
TXL_STORY_3105 =
    "你已踏入蛾摩拉，罪人藏身之城。\n"
    "随吾越过，力量沉积的荒漠。\n"
    "水晶在沙下苏醒，化作可用之力。\n"
    "取回这座城，从天界窃去的所有。\n"; // Context: Speaker: Cherubel. Level 6 completion, leading to the Energy Pyramid.
TXL_STORY_3006 =
    "碎玻璃换来黄金，黄金换来水晶。\n"
    "将水晶化作能量，点燃氮气之力。\n"
    "此世已被抽空，今取回应得之份。\n"
    "被窃的力量，正等你唤醒。\n"; // Context: Speaker: Cherubel. Level 7 (Energy Pyramid) introduction.
TXL_STORY_30061 =
    "能量已满，你却迟迟未发。\n"
    "待球滚动，按住氮气踏板。\n"
    "让速度骤起，穿越这条球道。\n"
    "莫使辛苦收来的力量，空留而不用。\n"; // Context: Speaker: Cherubel. Level 7 NOS reminder when it has not been used.
TXL_STORY_3106 =
    "携此力量，继续走向林阴。\n"
    "通往矿井之路，已被树木拥塞。\n"
    "裁断将至，森林亦挡在前方。\n"
    "斩开枝叶，使预言逐一成真。\n"; // Context: Speaker: Cherubel. Level 7 completion, leading to the forest.
TXL_STORY_3007 =
    "伐尽林木，使此世再无青绿。\n"
    "生命曾居之地，终将化作荒漠。\n"
    "但留下木块，拦住滚动的球。\n"
    "面对撒拉弗，它们亦能助你。\n"; // Context: Speaker: Cherubel. Level 8 (Forest) introduction.
TXL_STORY_3107 =
    "吾之末试已过，前路就此两分。\n"
    "撒拉弗守着矿井，水晶之力深藏。\n"
    "吾不入那深处，自有守护者索其应得。\n"
    "去吧，让地底的黑暗迎接你。\n"; // Context: Speaker: Cherubel. Level 8 completion, leading to Seraphel's mines.
TXL_STORY_3008 =
    "蛾摩拉已染瘟疫，此事须藏心底。\n"
    "人口日渐凋零，吾称之为净化。\n"
    "此乃吾最后之业，事毕便将离去。\n"
    "那座城的命数，早已刻入其心。\n"; // Context: Speaker: Seraphel. Before Level 11 (Grey Desert / Cemetery): Seraphel describes the plague at Gomorrah.
TXL_STORY_3108 =
    "吾之事已毕，你之事方始。\n"
    "蛾摩拉仍在，暗日之下等待。\n"
    "座天使将于城中，接过你的前路。\n"
    "去吧，以你的手，写下此城的终局。\n"; // Context: Speaker: Seraphel. After winning Level 11, before Level 12: Seraphel departs and sends the player to Thrones in Gomorrah.
TXL_STORY_3009 =
    "吾乃撒拉弗。此地是所罗门的矿井。\n"
    "水晶之力，在大地深处闪耀。\n"
    "它不会因怜悯而起，亦不应祈求而来。\n"
    "深入地底，取出长眠于此的力量。\n"; // Context: Speaker: Seraphel. Level 9 (Mines) introduction by Seraphel.
TXL_STORY_3109 =
    "大地深藏之物，矿井已尽数交出。\n"
    "如今往工厂去，那里尚封着力量。\n"
    "将水晶之力引出，使它化作烈焰。\n"
    "来吧，裁断之时，已渐渐成熟。\n"; // Context: Speaker: Seraphel. Level 9 completion, leading to the factory.
TXL_STORY_3010 =
    "水晶养活机器，一炉又一炉燃烧。\n"
    "此城之人，盗走力量却称为己有。\n"
    "取尽其力，莫容他们再作私藏。\n"
    "罪之轻重，已在天使的秤上。\n"; // Context: Speaker: Seraphel. Before Level 10 (Gas Factory): Seraphel demands the return of stolen energy.
TXL_STORY_3110 =
    "机器应还之力，已归大地深处。\n"
    "瘟疫之影笼罩，生者将日渐稀少。\n"
    "随吾往城外去，墓地正在前方。\n"
    "你将见裁断，如何落到众人身上。\n"; // Context: Speaker: Seraphel. After winning Level 10, before Level 11 (Grey Desert / Cemetery).
TXL_STORY_3011 =
    "吾乃座天使，蛾摩拉的罪已满盈。\n"
    "不法之行，已断送它的来日。\n"
    "吾知自身之重，亦知降下时的后果。\n"
    "助吾在翼影之下，将此城烧作灰烬。\n"; // Context: Speaker: Thrones. Level 12 introduction: Thrones judges Gomorrah.
TXL_STORY_3111 =
    "蛾摩拉已燃，浓烟直上长空。\n"
    "高塔倾颓，骄矜的根基尽毁。\n"
    "此片火场之外，尚有最后一事。\n"
    "大地本身，亦须向烈火作答。\n"; // Context: Speaker: Thrones. Level 12 completion, leading to Judgment Day.
TXL_STORY_3012 =
    "审判之日已至，最后的秤已立。\n"
    "大地与旧日之债，今置于两端。\n"
    "天界默然注视，旧世逐一燃尽。\n"
    "凡曾存在之物，终须归于火中。\n"; // Context: Speaker: Thrones. Before Level 13 (Ashland / Judgment Day): Thrones announces the final trial.
TXL_STORY_3112 =
    "所言终已成真，大地化作灰尘。\n"
    "旧世最后一闪，从此不复存在。\n"
    "恶已尽，诸试已终，罪亦再无踪。\n"
    "只因人间无人，唯你独留此中。\n"; // Context: Speaker: Thrones. Level 13 completion and campaign ending.
TXL_STORY_32000 =
    "吾乃以西结，如今可明言一切。\n"
    "生者与亡者之间，你已历尽诸试。\n"
    "你助吾等除尽恶，世界从此寂静。\n"
    "此刻皆遂你愿，往后你将何行？\n"; // Context: Speaker: Ezekiel. Post-campaign result dialog before random-game free play.
TXL_STORY_OPTION_ENTER_TRAINING = "去学校"; // Context: Choice button in Level 1 introductions/outros and the Level 2 recovery introduction: enter school.
TXL_STORY_OPTION_YES = "是"; // Context: Confirmation button for switching school lessons; tutorial confirmation uses the separate YES_TUTORIAL key.
TXL_STORY_OPTION_NO = "否"; // Context: Cancel button for switching school lessons; tutorial refusal uses the separate NO_TUTORIAL key.
TXL_STORY_OPTION_NOT_NOW = "现在先不去"; // Context: Choice after failing Level 1 or during its replay introduction: decline training and try Level 1 again.
TXL_STORY_OPTION_COMPETE_VS_EZEKIEL = "和以西结对战"; // Context: Choice after passing Level 1 or in the Level 2 recovery introduction: proceed to face Ezekiel in Level 2.
TXL_STORY_OPTION_OK = "确定"; // Context: Acknowledgement button for campaign dialogue and school instructions.
TXL_STORY_OPTION_NEXT = "下一步"; // Context: School lesson completion button: advance to the next lesson; after Lesson 5, leave school.
TXL_STORY_OPTION_OK_TITLE = "确定"; // Context: Accept the suggested ball after struggling in school Lesson 5; English uses title-case Ok.
TXL_STORY_OPTION_DECLINE = "拒绝"; // Context: Decline the suggested ball after struggling in school Lesson 5.
TXL_STORY_OPTION_OPEN_OIL = "打开油道"; // Context: Button on the Level 3 oil reminder (story 30032): open the oil window.
TXL_STORY_OPTION_NO_THANKS = "不用了"; // Context: Decline opening the oil window (story 30032) or shop (story 3141).
TXL_STORY_OPTION_OPEN_SHOP = "打开商店"; // Context: Button after Level 5 and before Cherubel in Level 6 (story 3141): open the shop.
TXL_STORY_OPTION_KEEP_PLAYING = "继续玩"; // Context: Post-campaign button (story 32000): continue into random-game free play.
TXL_STORY_OPTION_COMPETE_VS_ANGEL = "和天使对战"; // Context: Legacy choice label for facing an angel; not currently used by a story option.
TXL_STORY_OPTION_RESET_CAMPAIGN = "重开战役"; // Context: Legacy campaign reset choice label; not currently used by a story option.
TXL_STORY_OPTION_YES_TAKE_ME_TO_THE_NEXT_LESSON = "好，带我去下一课"; // Context: Legacy school choice to advance to the next lesson; not currently used by a story option.
TXL_STORY_OPTION_NO_I_WANT_TO_LEAVE_SCHOOL = "不，我想离开学校"; // Context: Legacy school choice to leave school; not currently used by a story option.
TXL_STORY_OPTION_PRACTICE_MORE = "继续练习"; // Context: Legacy school choice to repeat practice; not currently used by a story option.
TXL_STORY_OPTION_BACK_TO_GAME = "回到游戏"; // Context: Legacy school choice to return to gameplay; not currently used by a story option.
TXL_STORY_OPTION_YES_TUTORIAL = "是"; // Context: Button following the first Level 1 introduction (story 2): accept the tutorial.
TXL_STORY_OPTION_NO_TUTORIAL = "否"; // Context: Button following the first Level 1 introduction (story 2): decline the tutorial.
TXL_STORY_SPEAKER_ANGEL = "天使"; // Context: Generic angel speaker label in the dialogue panel. Named campaign angels use their character names instead.
TXL_STORY_SPEAKER_DEVIL = "恶魔"; // Context: Devil speaker label in the dialogue panel.
TXL_STORY_SPEAKER_MYSELF = "我"; // Context: Player/self speaker label in the dialogue panel.
TXL_STORY_SPEAKER_UNKNOWN = "???"; // Context: Fallback speaker label when no known speaker matches.
TXL_STORY_NAME_CHERUBEL = "基路伯"; // Context: Character name shown above campaign dialogue: Levels 6-8.
TXL_STORY_NAME_SERAPHEL = "撒拉弗"; // Context: Character name shown above campaign dialogue: Levels 9-11.
TXL_STORY_NAME_THRONES = "座天使"; // Context: Character name shown above campaign dialogue: Levels 12-13.
TXL_STORY_NAME_EZEKIEL = "以西结"; // Context: Character name shown above campaign dialogue: Levels 1-5, school instructions, and the post-campaign result dialogue.
