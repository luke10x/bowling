// Tegel translation source DSL - Japanese (jp_jp)
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
// Ezekiel -> エゼキエル; Cherubel -> ケルビム; Seraphel -> セラフィム;
// Thrones -> 座天使.
// Dausos is the Lithuanian spiritual realm; English calls it The Heavens.
// Do not carry the Lithuanian name Dausos into Japanese-facing text.
// Nitratai is Lithuanian-only: translate the resource as crystals/gems or
// energy according to the scene, not as nitrates. Existing UI uses
// クリスタル / ジェム, エネルギー, and ニトロ for the pedal.
//
// Japanese line breaks (改行・禁則処理):
// Layout constraint: long Japanese runs, especially katakana loanwords and
// compounds, can behave as a single unbreakable unit and overflow narrow UI.
// Do not rely on automatic word wrapping to split them. Keep button labels
// short; test dialogue and labels at the narrowest supported/mobile width.
// For new or explicitly revised text, prefer a shorter natural expression or
// insert an explicit \n between phrases or meaningful compound components.
// If a katakana compound itself is too wide, a break between its components is
// preferable to overflow; do not apply "keep words together" as an absolute rule.
// Keep character names intact where possible and never split a UTF-8 character.
// Keep each verse line on its own source row, ending with \n. Source rows alone
// do not create displayed line breaks; \n does. Preserve existing stanza lines
// and pauses; do not force an existing three-line translation into four lines.
// For new or explicitly revised translations, break at natural phrase or clause
// boundaries. Keep words, angel names, numbers with units, and small kana with
// the preceding kana together; avoid isolating particles from their phrase.
// Do not start a line with closing punctuation such as 、 。 ！ ？ 」 』 ）,
// or end one with an opening bracket such as 「 『 （. Keep punctuation attached
// to its phrase. Avoid adding spaces to simulate wrapping or alignment.
// Check displayed wrapping in the dialogue panel, especially on mobile; do not
// assume the renderer automatically handles Japanese line-breaking rules.
// Reformatting source must not change wording or intentional \n escapes.
//
// Poetic style: 文語自由詩, with echoes of 七五調 and 漢詩訓読の語り口.
// Use four-line stanzas, restrained archaic diction (われ, 来たれ, なり),
// parallel phrases and pauses; do not claim strict 5/7 metre or impose end rhyme.
// Prefer short clauses and 結晶 / 力 in verse to reduce long katakana runs;
// UI terms remain unchanged, and ニトロ still names the pedal.
// Use 天 / 天の門 for the spiritual realm. Keep names in their established form.
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

TXL_MENU = "メニュー"; // Used in: main menu (clayton/menu_clay.h), gameplay/HUD (game.cpp)
TXL_RENAME = "チート"; // Used in: main menu (clayton/menu_clay.h)
TXL_GO_TO_SCHOOL = "学校へ行く"; // Used in: main menu (clayton/menu_clay.h)
TXL_LANGUAGE = "言語"; // Used in: language selector (clayton/language_clay.h), main menu (clayton/menu_clay.h)
TXL_CAMPAIGN = "キャンペーン"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_CAMPAIGN_TITLE = "キャンペーン"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_LEVELS = "キャンペーンレベル"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESTART_CAMPAIGN = "キャンペーンをやり直す"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_WINS = "勝利"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_LOST = "敗北"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_QUIT = "中断"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BEST_SCORE = "ベストスコア"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_TIME_TO_BEAT = "クリア時間"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BACK = "戻る"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_REPLAY = "もう一度"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PLAY = "プレイ"; // Used in: campaign and results windows (clayton/win_stack.h), high-score window (hiscore/hiscore_clay.h), gameplay/HUD (game.cpp)
TXL_NOT_AVAILABLE = "なし"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PRACTICE = "練習"; // Used in: main menu (clayton/menu_clay.h), gameplay/HUD (game.cpp)
TXL_DEVICE_SHARE = "デバイス共有"; // Used in: main menu (clayton/menu_clay.h)
TXL_GAME_SETTINGS = "ゲーム設定"; // Used in: main menu (clayton/menu_clay.h), game settings (settings/settings_clay.h)
TXL_TRACKER = "トラッカー"; // Used in: main menu (clayton/menu_clay.h)
TXL_CREDITS = "クレジット"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_AD_MANAGER_TITLE = "広告管理"; // 広告管理ウィンドウ
TXL_AD_SPONSOR_PROMPT = "スポンサーからのお知らせ"; // ブラウザー広告のボタン
TXL_AD_CREDIT_FMT = "広告クレジット: %.1f / %.0f"; // 広告管理ウィンドウ
TXL_AD_CREDIT_BUCKET_FMT = "勝利貯金: %.0f / %.0f (低クレジット勝利 +%.0f)"; // 広告管理ウィンドウ
TXL_AD_CREDIT_RULES_FMT = "勝利: -%d、敗北: -%d、毎日ログイン: +%d"; // 広告ルール
TXL_AD_HOLIDAY_ACTIVE = "広告休暇中"; // 広告管理ウィンドウ
TXL_AD_HOLIDAY_INACTIVE = "広告休暇はありません"; // 広告管理ウィンドウ
TXL_AD_HOLIDAY_REMAINING_FMT = "残り: %d時間 %d分"; // 広告管理ウィンドウ
TXL_AD_HOLIDAY_END_FMT = "終了: %s（現地時刻）"; // 広告管理ウィンドウ
TXL_ADS_TODAY_FMT = "今日のゲーム広告: %d / %d"; // 広告管理ウィンドウ
TXL_ADS_TO_HOLIDAY_FMT = "休暇まであと%d件"; // 広告管理ウィンドウ
TXL_AD_MONEY_TODAY_FMT = "今日のコイン広告: %d / %d"; // 広告管理ウィンドウ
TXL_AD_MONEY_RULE_FMT = "報酬広告: $%d、入荷ごとに%d回"; // 広告ルール
TXL_AD_HOLIDAY_RULE_FMT = "広告%d回後: %d時間の休暇"; // 広告ルール
TXL_GET_REWARD = "ゲット"; // ショップの報酬カード
TXL_REWARD_CARD = "報酬"; // ショップの報酬カードの見出し
TXL_REWARD_LOADING = "報酬を読み込み中"; // 報酬広告の進行中
TXL_WATCH_AD = "広告を見る"; // ショップの報酬ボタン
TXL_WATCH_PROMO = "ゲーム紹介を見る"; // H5 承認待ちの代替表示
TXL_MINIGAMES = "ミニゲーム"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_SELECT_LANGUAGE = "言語選択"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_ENGLISH = "英語"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_CHINESE = "中国語"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_LITHUANIAN = "リトアニア語"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_JAPANESE = "日本語"; // Used in: language selector (clayton/language_clay.h)
TXL_SOUND = "サウンド"; // Used in: sound window (sounds/sound_clay.h), gameplay/HUD (game.cpp)
TXL_SONG = "曲"; // Used in: sound window (sounds/sound_clay.h)
TXL_OIL = "オイル"; // Used in: oil window (oil/oil_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_BALLS = "ボール"; // Used in: shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_SHOP = "ショップ"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_HI_SCORE = "ハイスコア"; // Used in: gameplay/HUD (game.cpp)
TXL_PRACTICE_TITLE = "練習"; // Used in: gameplay/HUD (game.cpp)
TXL_FREESTYLE_TITLE = "フリースタイル"; // Used in: gameplay/HUD (game.cpp)
TXL_TRY_AGAIN = "もう一度"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_BONUS_GAME = "ボーナスゲーム"; // Used in: gameplay/HUD (game.cpp)
TXL_BALL_CHARGE_FMT = "エネルギー"; // Used in: gameplay/HUD (game.cpp)
TXL_NOS_NO_ENERGY = "エネルギーが足りません。ジェムを集めてください。"; // Used in: gameplay/HUD (game.cpp)
TXL_GREETING_RESUME = "ボウリングを続けますか？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_GREETING_WELCOME = "このボウリングゲームへようこそ。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_GREETING_SUBLINE = "「レディー」を押してゲーム画面に戻ります。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_READY = "レディー"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SELECT_ANGEL = "エンジェルを選択"; // Used in: gameplay/HUD (game.cpp)
TXL_SELECT_BOT = "対戦相手を選択"; // Used in: bot selection (bots/bots_clay.h), gameplay/HUD (game.cpp)
TXL_SELECT_HOUSE = "レーンを選択"; // Used in: gameplay/HUD (game.cpp)
TXL_SWITCH_HOUSE = "レーンを切り替え"; // Used in: house selection (houses/houses_clay.h), gameplay/HUD (game.cpp)
TXL_SELECT_BALL = "ボールを選択"; // Used in: gameplay/HUD (game.cpp)
TXL_SELL_FOR_FMT = "$ %d で売却"; // 在庫の売却ボタン
TXL_SELL_CONFIRM_TITLE = "ボールを売却"; // 売却確認
TXL_SELL_CONFIRM_ITEM_FMT = "%sを$%dで売却しますか？"; // 売却確認
TXL_SELL_CONFIRM_LOSS_FMT = "元の価格より$%dの損失です。"; // 売却確認
TXL_SELL_CONFIRM_GAIN_FMT = "元の価格より$%dの利益です。"; // 売却確認
TXL_CURRENT = "現在使用中"; // Used in: shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_INVENTORY = "所持ボール"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_BUY_NOW = "今すぐ購入"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_HOUSES = "レーン"; // Used in: house selection (houses/houses_clay.h)
TXL_SELECTED_FMT = "選択中: %s"; // Used in: house selection (houses/houses_clay.h), bot selection (bots/bots_clay.h)
TXL_SELECT_A_HOUSE = "レーンを選んでください"; // Used in: house selection (houses/houses_clay.h)
TXL_NO_HOUSES = "レーンがありません"; // Used in: house selection (houses/houses_clay.h)
TXL_BOT_AVATAR = "対戦相手のアバター"; // Used in: bot selection (bots/bots_clay.h)
TXL_SELECT_A_BOT_AVATAR = "対戦相手のアバターを選択"; // Used in: bot selection (bots/bots_clay.h)
TXL_NO_BOTS = "対戦相手がいません"; // Used in: bot selection (bots/bots_clay.h)
TXL_INVENTORY_EMPTY = "所持ボールがありません"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_EMPTY = "ショップは空です"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h)
TXL_LOCKED = "ロック中"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PASS = "合格"; // Used in: campaign and results windows (clayton/win_stack.h), school window (school/school_clay.h)
TXL_RESETS_IN_FMT = "%s にリセット"; // Used in: shop window (clayton/shop_clay.h)
TXL_SOUND_SETTINGS = "サウンド設定"; // Used in: sound window (sounds/sound_clay.h)
TXL_CHANGING_QUALITY = "品質を変更中..."; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_MODE = "オーディオモード"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_SYNTH = "シンセ"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_OFF = "オフ"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_PANIC = "緊急停止"; // Used in: sound window (sounds/sound_clay.h)
TXL_SDL_BUFFER = "SDL バッファ"; // Used in: sound window (sounds/sound_clay.h)
TXL_MUSIC_VOLUME = "音楽音量"; // Used in: sound window (sounds/sound_clay.h)
TXL_OIL_STATUS = "オイル状態"; // Used in: oil window (oil/oil_clay.h)
TXL_LANE_OIL_MAP = "レーンオイルマップ"; // Used in: oil window (oil/oil_clay.h)
TXL_TRACK_INFO = "トラック情報"; // Used in: oil window (oil/oil_clay.h)
TXL_MAX_OIL_LEVEL = "最大オイル量"; // Used in: oil window (oil/oil_clay.h)
TXL_CURRENT_OIL_LEVEL = "現在"; // Used in: oil window (oil/oil_clay.h)
TXL_CARRYDOWN = "キャリーダウン"; // Used in: oil window (oil/oil_clay.h)
TXL_OIL_DECAY = "減衰"; // Used in: oil window (oil/oil_clay.h)
TXL_SLIPPERY = "滑りやすさ"; // Used in: oil window (oil/oil_clay.h)
TXL_PUSHBACK = "押し戻し"; // Used in: oil window (oil/oil_clay.h)
TXL_REOILS_FMT = "再オイル: %d/%d"; // Used in: oil window (oil/oil_clay.h)
TXL_REOIL = "再オイル"; // Used in: oil window (oil/oil_clay.h), school window (school/school_clay.h)
TXL_CANT_AFFORD = "資金不足"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), oil window (oil/oil_clay.h)
TXL_REOIL_LOCKED = "再オイルはロックされています"; // Used in: oil window (oil/oil_clay.h)
TXL_FRESHLY_OILED = "オイル済み"; // Used in: gameplay/HUD (game.cpp)
TXL_TOP_SCORES = "トップスコア"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_YOUR_SCORE_TOP_FMT = "あなたのスコアはトップ%dに入っています"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_GOOD_RUN = "良いゲーム！"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_POINTS_FMT = "%dポイント"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_SCORE_HIGHER_THAN_FMT = "あなたのスコアは最近の全プレイの%.0f%%を上回っています"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_LEADERBOARD_LAST_HOUR = "リーダーボード（直近1時間）"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_PLAYER = "プレイヤー"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_SCORE = "スコア"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_AGE = "時間"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_NO_SCORES_YET = "まだスコアがありません - 最初に記録しよう！"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_ATTEMPTS_FMT = "試行回数: %d"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_RANGE_FMT = "範囲: %d～%d"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_GAME_SETTINGS_TITLE = "ゲーム設定"; // Used in: game settings (settings/settings_clay.h)
TXL_HIGH_DETAILS = "高画質"; // Used in: game settings (settings/settings_clay.h)
TXL_HIGH_DETAILS_HELP = "画面の装飾と追加の視覚効果"; // Used in: game settings (settings/settings_clay.h)
TXL_PATCHED_YMFM = "改良版YMFM"; // Used in: game settings (settings/settings_clay.h)
TXL_PATCHED_YMFM_HELP = "実験的な高速FM音源を使用します（音声を再起動）"; // Used in: game settings (settings/settings_clay.h)
TXL_PERFORMANCE_ANALYSIS = "性能解析"; // Used in: game settings (settings/settings_clay.h)
TXL_PERFORMANCE_ANALYSIS_HELP = "フレームグラフと計測結果を5秒ごとに表示（F3）"; // Used in: game settings (settings/settings_clay.h)
TXL_RESET_PROGRESS = "初期化"; // Used in: campaign and results windows (clayton/win_stack.h), game settings (settings/settings_clay.h)
TXL_RESET_CAMPAIGN = "キャンペーンをやり直す"; // Used in: game settings (settings/settings_clay.h)
TXL_RESET_PROGRESS_HELP = "キャンペーンをリセットすると、\nレベル記録、ボール、解除、ルーンを含む\nキャンペーン状態が消去されます。\n初期化ではアプリ設定も消去されます。"; // Used in: game settings (settings/settings_clay.h)
TXL_CHECK_FOR_UPDATE = "更新を確認"; // Used in: game settings (settings/settings_clay.h)
TXL_UPDATE_PWA = "PWA を更新"; // Used in: game settings (settings/settings_clay.h)
TXL_UPDATE_WEB = "ウェブサイトを再読み込み"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_INSTALLED_FMT = "インストール済みビルド: %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_PUBLISHED_FMT = "公開ビルド: %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_CHECKING = "更新を確認中..."; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_UP_TO_DATE_FMT = "最新です。公開ビルド: %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_AVAILABLE_FMT = "更新があります: %s -> %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_OFFLINE = "オフラインです。ネット接続後に再確認してください。"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_ERROR = "更新サーバーに接続できませんでした。"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_APPLYING = "更新を適用しています..."; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_UNSUPPORTED = "PWA 更新確認はウェブ版でのみ利用できます。"; // Used in: game settings (settings/settings_clay.h)
TXL_CREDITS_AUTHOR_FMT = "ゲームデザイン、プログラム、アート、サウンド: %s。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_AUTHOR_NAME = "Lape Kale"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_CHARACTER_BASE = "キャラクター基礎モデル:\nOpenGameArt.org の\nVery Low Poly Human。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_CHEST = "宝箱モデル: CDmir、\nOpenGameArt.org（CC0）。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_MIXAMO = "キャラクターアニメーション: Mixamo。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_TECH = "SDL、OpenGL ES、Clay、GLM、\nJolt Physics、ymfm を使用。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_TRACKER = "音楽と効果音はすべて、\nゲーム内蔵のトラッカーで制作しました。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_THANKS = "プレイ、テスト、そしてもう1フレーム\n投げ続けてくれた皆さんに感謝します。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SCHOOL_TITLE_FMT = "レッスン %d. %s"; // Used in: school window (school/school_clay.h)
TXL_SCHOOL_PROGRESS_GOOD_SHOTS = "ナイスショット"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_LIGHT = "軽い球"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_HEAVY = "重い球"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_HIT_ALL = "3本すべてに当てる"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_REOILS = "再オイル"; // Used in: school progress row (school/school_clay.h)
TXL_AIM_LESSON = "エイムレッスン"; // Used in: school window (school/school_clay.h)
TXL_BALL_MASS = "ボールの質量"; // Used in: school window (school/school_clay.h)
TXL_SPIN_BALL = "ボールに回転をかける"; // Used in: school window (school/school_clay.h)
TXL_OIL_AND_SKID = "オイルとスキッド"; // Used in: school window (school/school_clay.h)
TXL_STRIKE_LINE = "ストライクライン"; // Used in: school window (school/school_clay.h)
TXL_MASS_FMT = "質量 (%.1fkg)"; // Used in: school window (school/school_clay.h)
TXL_LIGHT_BALL_TEST = "軽いボールのテスト"; // Used in: school window (school/school_clay.h)
TXL_HEAVY_BALL_TEST = "重いボールのテスト"; // Used in: school window (school/school_clay.h)
TXL_PASSED = "合格"; // Used in: school window (school/school_clay.h)
TXL_MASS = "質量"; // Used in: shop window (clayton/shop_clay.h), school window (school/school_clay.h)
TXL_SPIN = "回転"; // Used in: shop window (clayton/shop_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_SKID = "スキッド"; // Used in: shop window (clayton/shop_clay.h)
TXL_BITE = "バイト"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_COMMON = "コモン"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_RARE = "レア"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_EPIC = "エピック"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_LEGENDARY = "レジェンダリー"; // Used in: shop window (clayton/shop_clay.h)
TXL_SPIN_TEST_FMT = "スピンテスト: %d/%d ターゲット (レベル %d/%d)"; // Used in: school window (school/school_clay.h)
TXL_AIM_LESSON_POINTS_FMT = "エイムレッスン: %d/%d ポイント"; // Used in: school window (school/school_clay.h)
TXL_PULL_BALL_BACK = "ボールを一番後ろまで引いてください"; // Used in: school window (school/school_clay.h)
TXL_LET_IT_GO_NOW = "今すぐ放してください"; // Used in: school window (school/school_clay.h)
TXL_MOVE_RIGHT = "右に移動"; // Used in: school window (school/school_clay.h)
TXL_MOVE_LEFT = "左に移動"; // Used in: school window (school/school_clay.h)
TXL_OIL_TEST_REOILS_FMT = "オイルテスト: 再オイル %d/%d"; // Used in: school window (school/school_clay.h)
TXL_REMAINING_OIL = "残りのオイル"; // Used in: school window (school/school_clay.h)
TXL_REOIL_PROGRESS = "再オイルの進行状況"; // Used in: school window (school/school_clay.h)
TXL_WEAR_IT_DOWN = "再オイルする前に使い切ってください。"; // Used in: school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_STRIKE_TEST_SCORE_STRIKE = "ストライクテスト: ストライクを取れ"; // Used in: school window (school/school_clay.h)
TXL_LEVEL1_TITLE = "レベル1 最初の目標"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL1_SUBTITLE = "ノーマルバイオーム 100点でクリア"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL2_TITLE = "レベル2 エゼキエル登場"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL2_SUBTITLE = "風力発電所 エゼキエルに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL3_TITLE = "レベル3 砂漠の警告"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL3_SUBTITLE = "砂漠 エゼキエルに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL4_TITLE = "レベル4 ガラスの氷"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL4_SUBTITLE = "氷 エゼキエルに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL5_TITLE = "レベル5 ネオン硝子"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL5_SUBTITLE = "郊外  エゼキエルに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL6_TITLE = "レベル6 ネオンの犬"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL6_SUBTITLE = "ネオン  犬に勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL7_TITLE = "レベル7 力のピラミッド"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL7_SUBTITLE = "砂漠  犬に勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL8_TITLE = "レベル8 木材ゾーン"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL8_SUBTITLE = "森林  犬に勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL9_TITLE = "レベル9 クリスタルの謁見"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL9_SUBTITLE = "クリスタル洞窟 くちばしに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL10_TITLE = "レベル10 ガス工場の告白"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL10_SUBTITLE = "ガス工場 くちばしに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL11_TITLE = "レベル11 墓地"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL11_SUBTITLE = "灰色の砂漠 くちばしに勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL12_TITLE = "レベル12 街の車輪"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL12_SUBTITLE = "廃墟の街  牛に勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL13_TITLE = "レベル13 灰のパレード"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL13_SUBTITLE = "灰の荒地  牛に勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_BALL_NAME_0 = "残り火ストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_1 = "炎ロール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_2 = "ブレイズフック"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_3 = "インフェルノ"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_4 = "フェニックス"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_5 = "フロストグライド"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_6 = "チルストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_7 = "北極ドリフト"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_8 = "氷河バイト"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_9 = "吹雪キング"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_10 = "星くず"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_11 = "星雲ロール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_12 = "宇宙フック"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_13 = "虚空ストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_14 = "ブラックホール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_15 = "UFOスピン"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_16 = "キセノンバイト"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_17 = "森ロール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_18 = "ツタストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_19 = "雷オーク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_20 = "巨人の根"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_21 = "世界樹"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_22 = "影ドリフト"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_23 = "深淵フック"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_24 = "ヌリファイア"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_25 = "回路ロール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_26 = "ネオンストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_27 = "量子フック"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_28 = "特異点"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_29 = "潮ロール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_30 = "リーフストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_31 = "クラーケンバイト"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_32 = "リヴァイアサン"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_33 = "ルーンボール"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_34 = "オラクルストライク"; // Used in: shop window (clayton/shop_clay.h)
TXL_YOU = "あなた"; // Used in: campaign and results windows (clayton/win_stack.h), new-game window (clayton/new_game_clay.h), high-score window (hiscore/hiscore_clay.h), gameplay/HUD (game.cpp)
TXL_BONUS_ROUND = "ボーナス"; // Used in: gameplay/HUD (game.cpp)
TXL_TUTORIAL_FMT = "レッスン%d"; // Used in: gameplay/HUD (game.cpp)
TXL_COMPLETION_TIME_FMT = "クリア時間 %s"; // Used in: gameplay/HUD (game.cpp)
TXL_AUDIO_BUFFER_INCREASED_FMT = "負荷対策で音声バッファを%dに変更"; // Used in: gameplay/HUD (game.cpp)
TXL_SPIN_TO_HOOK = "回して曲げる"; // Used in: gameplay/HUD (game.cpp)
TXL_DROP_HERE_TO_USE = "ここへ\nドロップ"; // Used in: gameplay/HUD (game.cpp)
TXL_CANNOT_USE_NOW = "今は\n使えません"; // Used in: gameplay/HUD (game.cpp)
TXL_NEED_AT_LEAST_2_BALLS = "ボールが\n2個必要"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_DEFAULT = "ルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_LOST = "ボール爆散"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_EVAPORATED = "ボール消滅"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_SURVIVED = "ボール生存"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_PINS_FROZEN = "ピン凍結"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_PATROL_DEPLOYED = "警備ピン配置"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_FOOTBALL = "フットボール起動"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_NITRO = "ニトロ補充完了"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_FLASH = "フラッシュ起動"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_SKULL = "スカル起動"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_ICY_BALL = "アイスボール起動"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BOOM = "ブーム起動"; // Used in: gameplay/HUD (game.cpp)
TXL_MISSED = "ミス"; // Used in: gameplay/HUD (game.cpp)
TXL_STALLED = "停止"; // Used in: gameplay/HUD (game.cpp)
TXL_SPLIT = "スプリット"; // Used in: gameplay/HUD (game.cpp)
TXL_STRIKE = "ストライク"; // Used in: new-game window (clayton/new_game_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_SPARE = "スペア"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_BLOCK_WOOD = "木"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_BRICK = "レンガ"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_CONCRETE = "コンクリ"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_GLASS = "ガラス"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_TITLE = "宝箱"; // Used in: gameplay/HUD (game.cpp)
TXL_TAP_TO_OPEN_CHEST = "タップで開く"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_RUNE_PICKED_FMT = "能力を入手: %s"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_COINS_PICKED_FMT = "%dコイン入手"; // Used in: gameplay/HUD (game.cpp)
TXL_CONTINUE = "続ける"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_RUNE_NAME_BOOM = "ブームルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_BOLT = "稲妻ルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_FREEZE = "氷結ルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_SKULL = "スカルルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_GUARD_PINS = "警備ピンルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_FOOTBALL = "フットボールルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_NITRO = "ニトロルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_DEFAULT = "ルーン"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_BOOM = "爆発ショット。強力だがボールを失う危険あり。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_BOLT = "防衛時、稲妻で敵ボールを消す。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_FREEZE = "防衛でピン凍結、攻撃で氷ボール化。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_SKULL = "スカル球。強打とブロック破壊が得意。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_GUARD_PINS = "警備ピン3本で敵を妨害。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_FOOTBALL = "球を跳ねさせ、軽く速くする。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_NITRO = "ニトロを満タンにする。残量80%未満で使用可能。"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NITRO_TOO_FULL = "ニトロ80%以上"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_DEFAULT = "ルーントレイに能力を追加。"; // Used in: gameplay/HUD (game.cpp)
TXL_MINIGAMES_HELP = "テスト入口。報酬あり、物語進行なし。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_MINIGAME_COIN_RUSH = "コインラッシュ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_MINIGAME_COUNT_MASTERS = "カウントマスター"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_MINIGAME_CROWD_CONTROL = "群衆コントロール"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_BONUS_PROMPT_FMT = "ボーナスを遊びますか？%s"; // Used in: gameplay/HUD (game.cpp)
TXL_RESET_PROGRESS_CONFIRM_TITLE = "進行リセット"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ARE_YOU_SURE = "よろしいですか？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESET_PROGRESS_FULL_DETAIL = "キャンペーン状態とアプリ設定を消去します。\nレベル記録、ボール、解除、ルーンを含みます。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESET_PROGRESS_SOFT_DETAIL = "キャンペーン状態を消去します。\nレベル記録、ボール、解除、ルーンを含みます。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_NO = "いいえ"; // Used in: campaign and results windows (clayton/win_stack.h), house selection (houses/houses_clay.h), high-score window (hiscore/hiscore_clay.h), bot selection (bots/bots_clay.h), gameplay/HUD (game.cpp)
TXL_YES = "はい"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_EXIT_BONUS_LEVEL = "ボーナス終了"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_EXIT_BONUS_LEVEL_CONFIRM = "進行中のボーナスを終了しますか？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_STRIKES = "ストライク"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_SPARES = "スペア"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_COINS = "コイン"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_WON_BY_POINTS = "点差勝利"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_TOTAL = "合計"; // Used in: campaign and results windows (clayton/win_stack.h), new-game window (clayton/new_game_clay.h)
TXL_COINS_OBTAINED = "獲得コイン"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_VICTORY = "勝利"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_YOU_LOSE = "敗北"; // Used in: gameplay/HUD (game.cpp)
TXL_NEXT = "次へ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_RETRY = "再挑戦"; // Used in: gameplay/HUD (game.cpp)
TXL_REPEAT = "リピート"; // Used in: gameplay/HUD (game.cpp)
TXL_INVENTORY_SHORT = "所持品"; // Used in: gameplay/HUD (game.cpp)
TXL_RELOADS_IN_FMT = "補充まで %s"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_RESTOCK_SOON = "まもなく補充"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_BONUS_LEVEL = "ボーナス"; // Used in: gameplay/HUD (game.cpp)
TXL_PLAY_BONUS = "ボーナス開始"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_COMPLETE = "ボーナス完了"; // Used in: gameplay/HUD (game.cpp)
TXL_COINS_PICKED_EARNED_FMT = "コイン: %d   獲得: $%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_COMPLETE_FMT = "ボーナス完了  +$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_EXITED_EARNED_FMT = "途中終了。獲得: $%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_EXITED_FMT = "ボーナス終了  +$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_VICTORY = "ボーナス勝利"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_DEFEAT = "ボーナス敗北"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_LOST = "ボーナス失敗"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_VICTORY_BASE = "勝利：守護者が敵基地に到達。"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_DEFEAT_SPAWN = "敗北: 敵が出現地点に到達。"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_SPAWN_LABEL = "出現"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_POWER_LABEL = "戦力"; // Used in: gameplay/HUD (game.cpp)
TXL_VICTORY_DETAIL = "勝利。"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_DEFEAT_DETAIL = "敗北。"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_ENEMIES = "敵"; // Used in: gameplay/HUD (game.cpp)
TXL_BOSSES = "ボス"; // Used in: gameplay/HUD (game.cpp)
TXL_SUPER_BOSS = "強ボス"; // Used in: gameplay/HUD (game.cpp)
TXL_GATES = "ゲート"; // Used in: gameplay/HUD (game.cpp)
TXL_PINS = "ピン"; // Used in: gameplay/HUD (game.cpp)
TXL_STANDERS = "残り"; // Used in: gameplay/HUD (game.cpp)
TXL_YOU_BEAT_ANGEL = "天使に勝利"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ANGEL_WINS = "天使の勝利"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BOT_SCORE_FMT = "あなた: %d    天使: %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_COMPLETE = "キャンペーン完了"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_COMPLETE_DETAIL = "全レベルクリア。レーンはあなたのもの。"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_TOTAL_TIME_FMT = "合計時間  %s"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ATTEMPTS_TOTAL_FMT = "挑戦  %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ENTER_CHEAT = "チート入力"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEAT_ACTIVATED = "チート有効"; // Used in: gameplay/HUD (game.cpp)
TXL_NO_SUCH_CHEAT = "該当チートなし"; // Used in: gameplay/HUD (game.cpp)
TXL_MAKES_YOU_SPAWN_FAST = "出現が速い"; // Used in: gameplay/HUD (game.cpp)
TXL_MAKES_YOU_STRONG = "強くなる"; // Used in: gameplay/HUD (game.cpp)
TXL_TURN_FMT = "%sの番"; // Used in: gameplay/HUD (game.cpp)
TXL_NOS = "ニトロ"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_AVAILABLE = "ボーナスあり"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_UPDATED = "ショップ更新"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_RESTOCK_PROMPT = "新ボール入荷。ショップへ？"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_VISIT_SHOP = "ショップへ"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_LATER = "あとで"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_KEYPAD_ENTER_TEXT = "テキスト入力"; // Used in: keypad HUD (clayton/keypad.h)
TXL_KEYPAD_ENTER_NUMBER = "数値入力"; // Used in: numeric keypad (clayton/numkeypad.h)
TXL_DELETE = "削除"; // Used in: numeric keypad (clayton/numkeypad.h), keypad HUD (clayton/keypad.h)
TXL_ENTER = "決定"; // Used in: numeric keypad (clayton/numkeypad.h), keypad HUD (clayton/keypad.h), gameplay/HUD (game.cpp)
TXL_STRESS = "アクセント"; // Used in: keypad HUD (clayton/keypad.h)
TXL_POST_CAMPAIGN = "キャンペーン後"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SETTINGS = "設定"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_SETTINGS = "キャンペーン後の設定"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_BEST_SCORE = "ベストスコア:"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_WON_FMT = "勝利 %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_LOST_FMT = "敗北 %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_QUIT_FMT = "中断 %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BLOCKS = "ブロック"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BLOCKS_SHORT = "ブ"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_NOS_SHORT = "ニ"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RUNES = "ルーン"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RUNES_SHORT = "ル"; // Used in: campaign and results windows (clayton/win_stack.h)

// -----------------------------------------------------------------------------
// Story and dialogue
// -----------------------------------------------------------------------------
TXL_STORY_1 =
    "迷える魂よ、天の門に来たれり。\n"
    "符文を学べ、時尽きる前に百点を。\n"
    "ストライクとスペアに、道を尋ねよ。\n"
    "一投ずつ数えよ、運に足をすくわれぬよう。\n"; // Context: Speaker: Ezekiel. Level 1's first-attempt introduction.
TXL_STORY_2 = "まず修練を受けるか。\n"; // Context: Speaker: Ezekiel. Follow-up prompt offering training from the first Level 1 introduction.
TXL_STORY_10 =
    "百点には、今日は届かなかった。\n"
    "最初の門は、まだ固く閉ざされた。\n"
    "わが学び舎へ来るがよい。学びの中に\n"
    "別の鍵が、静かに待つやもしれぬ。\n"; // Ezekiel: Level 1 loss; school hints at another path beyond this gate.
TXL_STORY_11 = "いま修練に入るか、天の試しに再び挑むか。\n"; // Context: Speaker: Ezekiel. Choice after the Level 1 failure message: train or replay.
// Ezekiel: Level 1 win with school unfinished; direct meeting invitation, then the school-or-Level-2 choice.
TXL_STORY_20 =
    "百点を得て、天の門は開いた。\n"
    "そなたの確かな腕を、われは見届けた。\n"
    "われはエゼキエル。まっすぐ会いに来るがよい。\n"
    "道はそなたのもの、門の先で待とう。\n";
TXL_STORY_21 = "先に学ぶか、それとも今すぐわれに会いに来るか。\n"; // Ezekiel: choice between school and meeting him in Level 2.
// Ezekiel: Level 1 win after school is complete; same invitation without another school offer.
TXL_STORY_22 =
    "百点を得て、天の門は開いた。\n"
    "そなたの確かな腕を、われは見届けた。\n"
    "われはエゼキエル。まっすぐ会いに来るがよい。\n"
    "道はそなたのもの、門の先で待とう。\n";
TXL_STORY_30 =
    "まだ学校を出られない。\n"
    "先にレッスンを終えよう。\n"; // Context: Speaker: Ezekiel. Attempt to leave mandatory training before completing it.
TXL_STORY_31 =
    "本当に？\n"
    "今のレッスン挑戦を中止し、選んだレッスンを始めます。\n"; // Context: Speaker: Ezekiel. Confirmation before replacing the current training attempt.
TXL_STORY_32 =
    "今のレッスンをやり直す？\n"
    "このレッスンの挑戦はリセットされます。\n"; // Context: Speaker: Ezekiel. Confirmation before restarting the current training lesson.
TXL_STORY_1000 =
    "レッスン2：球の質量。\n"
    "ここでは質量を学ぶ。\n"
    "球ごとに質量があり、感触と転がりが変わる。\n"
    "まず軽い球でピンに当てよう。\n"
    "卒業には重い球でも当てる必要がある。\n"; // Context: Speaker: Ezekiel. Lesson 2 introduction: ball mass.
TXL_STORY_1010 = "よし！ 質量テスト合格。\n"; // Context: Speaker: Ezekiel. Lesson 2 completion.
TXL_STORY_1012 =
    "合格には質量スライダーを軽い端か重い端に合わせて。\n"
    "中間で投げても進行には入らない。\n"; // Context: Speaker: Ezekiel. Lesson 2 reminder to use a light or heavy ball.
TXL_STORY_1013 = "いいね。次は重い端に切り替えてピンに当てよう。\n"; // Context: Speaker: Ezekiel. Lesson 2 prompt to use the heavy ball.
TXL_STORY_1014 = "いいね。次は軽い端に切り替えてピンに当てよう。\n"; // Context: Speaker: Ezekiel. Lesson 2 prompt to use the light ball.
TXL_STORY_1030 = "学校にはいつでも戻れる。\n"; // Context: Speaker: Ezekiel. General return-to-school message after a lesson.
TXL_STORY_1020 =
    "おめでとう。回転テスト合格。\n"
    "球ごとに回転への反応がある。\n"
    "噛みつきなどの値も反応の強さを変える。\n"; // Context: Speaker: Ezekiel. Lesson 3 completion: spin.
TXL_STORY_1022 =
    "レッスン3：球の回転。\n"
    "投げた直後、画面上の動きで球に回転をかける。\n"
    "すると球は特定の方向へ曲がり始める。\n"
    "軽い標的ピンを全部倒せば合格（2レベル）。\n"; // Context: Speaker: Ezekiel. Lesson 3 introduction: ball spin.
TXL_STORY_1052 =
    "レッスン4：オイルと滑り。\n"
    "このレーンは塗りたて。半分から三分の二ほどが最大オイルだ。\n"
    "このレッスンではオイルがすぐ減るので、数投で感触が変わる。\n"
    "場ごとの滑りや、球のスキッド値も影響する。\n"; // Context: Speaker: Ezekiel. Lesson 4 introduction: oil and skid.
TXL_STORY_1054 =
    "レーンに新しいオイルが必要だ。\n"
    "オイル画面を開き、塗り直してから投げよう。\n"; // Context: Speaker: Ezekiel. Lesson 4 prompt when the lane needs fresh oil.
TXL_STORY_1060 = "よし！ オイルテスト合格。\n"; // Context: Speaker: Ezekiel. Lesson 4 completion.
TXL_STORY_1032 =
    "レッスン1：狙い。\n"
    "投げ方を学ぼう。\n"
    "球をしっかり後ろへ引き、中央に保って離す。\n"
    "どのピンでも当てれば1点だ。\n"; // Context: Speaker: Ezekiel. Lesson 1 introduction: aiming.
TXL_STORY_1040 = "よし！ 狙いのテスト合格。\n"; // Context: Speaker: Ezekiel. Lesson 1 completion.
TXL_STORY_1070 =
    "レッスン5：ストライク線。\n"
    "コインを追え。線は中央から外れ、ポケットへ戻る。\n"
    "目標はストライク。\n"
    "反対側を練習したい時はライン切替を押せる。\n"; // Context: Speaker: Ezekiel. Lesson 5 introduction: strike line.
TXL_STORY_1072 =
    "見事！ ストライク。\n"
    "学校を卒業した。\n"
    "いつでも戻っていい。\n"; // Context: Speaker: Ezekiel. Lesson 5 and school completion.
TXL_STORY_1080 =
    "苦戦しているね。\n"
    "この球を試してみる？\n"; // Context: Speaker: Ezekiel. Suggested ball after the player struggles in training.
TXL_STORY_1021 =
    "ジグザグのコインで回転をもっと練習できる。\n"
    "レッスン3が解放された。\n"; // Context: Speaker: Ezekiel. Lesson 3 unlock notification after the zig-zag gold lesson.
TXL_STORY_40 =
    "われはエゼキエル、つまずきも立ち直りも見たり。\n"
    "天の眼差しの下に、符文は眠る。\n"
    "百点を得よ、初めの門を開くため。\n"
    "導きを望むなら、修練の戸は開いている。\n"; // Legacy Level 1 replay introduction; no longer routed.

// Context: Speaker: Ezekiel. Before Level 2 (Wind Farm): Ezekiel introduces himself and promises that the player can save the world; the promise is deceptive (see lore.md).
TXL_STORY_3002 =
    "われはエゼキエル、誇り高き初めの天使。\n"
    "そなたの一投一投を、われは見守りたり。\n"
    "その才は、この世を救う力となろう。\n"
    "罪と偽りに、奪われしこの世を。\n";
TXL_STORY_3102 =
    "手は揺らがず、狙いは冴えたり。\n"
    "球は定めの軌道を、よく守りたり。\n"
    "この穏やかな地を離れ、われに続け。\n"
    "砂は覆う、いにしえの罪の跡を。\n"; // Context: Speaker: Ezekiel. Level 2 completion, leading to the Red Desert.
TXL_STORY_3003 =
    "罪に乾いた荒野、渇きは深し。\n"
    "砂は球道をふさぎ、球の行く手を塞ぐ。\n"
    "油と乳香をもて、この道を潤せ。\n"
    "清めの火に、おごりだけを焼かれぬよう。\n"; // Context: Speaker: Ezekiel. Level 3 (Red Desert) introduction.
TXL_STORY_30031 =
    "真ん中を打てば、ピンは左右に裂ける。\n"
    "残る隙間は、次の一投の傷となる。\n"
    "横より入り、曲がる軌道を生かせ。\n"
    "道を改めれば、スプリットを避けられよう。\n"; // Context: Speaker: Ezekiel. Level 3 side-throw harassment after repeated splits.
TXL_STORY_30032 =
    "荒野を行くのに、油はまだ使わぬか。\n"
    "乾いた板は、おごりの一投を覚えている。\n"
    "試される前に、まず道を潤せ。\n"
    "いまオイル画面を、開いてやろうか。\n"; // Context: Speaker: Ezekiel. Level 3 oil reminder when the player has not re-oiled.
TXL_STORY_3042 =
    "ガラスは降る、光る刃の雨となり。\n"
    "その痛みを、われに返すこともできる。\n"
    "球で砕けば、その速さは奪われる。\n"
    "時を見よ、われの番に投げ返せ。\n"; // Context: Speaker: Ezekiel. Level 5 glass reminder when the player has not thrown glass back.
TXL_STORY_3050 =
    "符文が手に来たり、その力を眠らすな。\n"
    "球の回転の輪へ、符文を引き入れよ。\n"
    "蓄えるか、いま力を解き放つか。\n"
    "その選びは、そなたの手にある。\n"; // Context: Speaker: Ezekiel. First rune discovery during the campaign: generic guidance on dragging a rune into the casting ring.
TXL_STORY_3051 =
    "この符文は、球を雷鳴へと変える。\n"
    "そなたの球を失い、敵に傷を与える。\n"
    "選ぶ前に、その代価を量れ。\n"
    "爆発は道を開くが、球をも奪う。\n"; // Context: Speaker: Ezekiel. Follow-up tutorial after the player first finds a Boom rune.
TXL_STORY_3052 =
    "手に宿るは、激しき稲妻。\n"
    "われの転がる球を、光に焼き尽くせる。\n"
    "されど危うさを見れば、われも逃れん。\n"
    "時を選んで放て、雷を空しくせぬよう。\n"; // Context: Speaker: Ezekiel. Follow-up tutorial after the player first finds a Bolt rune.
TXL_STORY_3053 =
    "符文が手に来たり、その力を眠らすな。\n"
    "球の回転の輪へ、符文を引き入れよ。\n"
    "蓄えるか、いま力を解き放つか。\n"
    "その選びは、そなたの手にある。\n"; // Context: Speaker: Ezekiel. First Boom rune discovery during the campaign: casting-ring guidance, automatically followed by the Boom warning in story 3051.
TXL_STORY_3054 =
    "符文が手に来たり、その力を眠らすな。\n"
    "球の回転の輪へ、符文を引き入れよ。\n"
    "蓄えるか、いま力を解き放つか。\n"
    "その選びは、そなたの手にある。\n"; // Context: Speaker: Ezekiel. First Bolt rune discovery during the campaign: casting-ring guidance, automatically followed by the Bolt warning in story 3052.
TXL_STORY_3103 =
    "熱にも乾きにも、砂にも耐えたり。\n"
    "次には寒さが、足場を奪いに来る。\n"
    "滑る道では、約束さえも滑り落ちる。\n"
    "道を守れ、氷に身を崩されぬよう。\n"; // Context: Speaker: Ezekiel. Level 3 completion, leading to the icy Level 4.
TXL_STORY_3004 =
    "寒さは万物の支えを、もろくする。\n"
    "魂もまた、留まるはずの場所を滑り落ちる。\n"
    "胎も道も球道も、支えを失う。\n"
    "まっすぐに行け、不用意な逸れは冷たく深い。\n"; // Context: Speaker: Ezekiel. Level 4 introduction.
TXL_STORY_3104 =
    "そなたは滑らず、道を手に保ちたり。\n"
    "次なる割れガラスは、町の崩れを告げる。\n"
    "ひとつの窓の破れから、滅びは始まる。\n"
    "その光る裂け目の先に、ゴモラの定めがある。\n"; // Context: Speaker: Ezekiel. Level 4 completion, leading to the broken-glass level.
TXL_STORY_3040 =
    "あの町へ、われが歩めるのはここまで。\n"
    "ガラスの向こうは、別の天使の領地。\n"
    "われの務めは終わり、ケルビムの域が始まる。\n"
    "進め、割れ窓が開くのは平和への道ではない。\n"; // Context: Speaker: Ezekiel. Before Level 5 (Suburbia): Ezekiel introduces the broken-glass lesson; Cherubel is encountered in Level 6.
TXL_STORY_3041 =
    "ガラスに触れたり、その教えは始まった。\n"
    "われが投げる番には、そなたも投げ返せる。\n"
    "手番のボタンを見よ、返す時を知れ。\n"
    "破片を放ち、裁きを先へ進めよ。\n"; // Context: Speaker: Ezekiel. Level 5 first-glass tutorial.
TXL_STORY_3140 =
    "割れた窓の先で、われの教えは終わる。\n"
    "ケルビムは待つ、より厳しき試しを携えて。\n"
    "ゴモラの門を守る者に、会いに行け。\n"
    "町は呼ぶ、その報いは待たぬ。\n"; // Context: Speaker: Ezekiel. Level 5 completion, before meeting Cherub.
TXL_STORY_3141 =
    "彼に会う前に、商人のもとを訪ねよ。\n"
    "球にはそれぞれ、異なる性がある。\n"
    "手に合うものを選べ、次の試しのため。\n"
    "いま店を、開いてやろうか。\n"; // Context: Speaker: Ezekiel. First shop prompt before the Cherub encounter.
TXL_STORY_3005 =
    "われはケルビム。誰ぞ、ゴモラに入る者は。\n"
    "ここでは罪が根を張り、己を正しきものと呼ぶ。\n"
    "割れガラスの収穫は、やがて結晶の光となる。\n"
    "集めよ、裁きにはなお力が要る。\n"; // Context: Speaker: Cherubel. Level 6 (Gomorrah) introduction by Cherub.
TXL_STORY_3105 =
    "罪人の隠れる町、ゴモラを歩みたり。\n"
    "次はわれと越えよ、力を秘めた荒野を。\n"
    "砂の下では結晶が目覚め、力となる。\n"
    "町が天より盗みしものを、取り戻せ。\n"; // Context: Speaker: Cherubel. Level 6 completion, leading to the Energy Pyramid.
TXL_STORY_3006 =
    "ガラスは黄金に、黄金は結晶の光に。\n"
    "結晶を力に変え、ニトロを燃やせ。\n"
    "力を吸われし世界より、いま取り分を得よ。\n"
    "盗まれた力は、目覚めを待っている。\n"; // Context: Speaker: Cherubel. Level 7 (Energy Pyramid) introduction.
TXL_STORY_30061 =
    "力は満ちたのに、まだ火を惜しむか。\n"
    "球が動き出したら、ニトロを押し続けよ。\n"
    "速さを呼び、球道を走り抜けよ。\n"
    "集めた力を、使わず終えるな。\n"; // Context: Speaker: Cherubel. Level 7 NOS reminder when it has not been used.
TXL_STORY_3106 =
    "この力を携え、木陰の先へ進め。\n"
    "鉱山への道は、木々にふさがれている。\n"
    "裁きの前に、森もまた立ちはだかる。\n"
    "枝を切り開け、予言を成すために。\n"; // Context: Speaker: Cherubel. Level 7 completion, leading to the forest.
TXL_STORY_3007 =
    "木を伐れ、この世の緑を尽くせ。\n"
    "命のあった地を、荒野に変えよ。\n"
    "されど木は残せ、転がる球を阻むため。\n"
    "セラフィムに挑むとき、その木が役立とう。\n"; // Context: Speaker: Cherubel. Level 8 (Forest) introduction.
TXL_STORY_3107 =
    "われの末の試しを越え、道は分かれる。\n"
    "結晶の力を秘める鉱山は、セラフィムが守る。\n"
    "われは入らぬ、その深みは彼女の領分。\n"
    "進め、地の底の闇に迎えられよ。\n"; // Context: Speaker: Cherubel. Level 8 completion, leading to Seraphel's mines.
TXL_STORY_3008 =
    "ゴモラに疫病あり、このことは胸に秘めよ。\n"
    "人は減りゆく、われはそれを清めと呼ぶ。\n"
    "これがわれの最後の務め、やがて別れん。\n"
    "町の定めは、その胸に刻まれている。\n"; // Context: Speaker: Seraphel. Before Level 11 (Grey Desert / Cemetery): Seraphel describes the plague at Gomorrah.
TXL_STORY_3108 =
    "われの務めは終わり、そなたの務めは始まる。\n"
    "暗き日の下で、ゴモラは待つ。\n"
    "町に立つ座天使が、そなたを迎えよう。\n"
    "進め、その手で町の終わりをなせ。\n"; // Context: Speaker: Seraphel. After winning Level 11, before Level 12: Seraphel departs and sends the player to Thrones in Gomorrah.
TXL_STORY_3009 =
    "われはセラフィム。ここはソロモンの鉱山。\n"
    "地の底に、結晶の力は輝く。\n"
    "慈悲にも祈りにも、その力は応じぬ。\n"
    "深く掘れ、眠れる力を取り出せ。\n"; // Context: Speaker: Seraphel. Level 9 (Mines) introduction by Seraphel.
TXL_STORY_3109 =
    "大地の秘めしものを、鉱山は明け渡した。\n"
    "次は工場へ、なお力の閉じ込められた場所へ。\n"
    "結晶から引き出せ、燃えさかる力を。\n"
    "来たれ、裁きの時は熟しつつある。\n"; // Context: Speaker: Seraphel. Level 9 completion, leading to the factory.
TXL_STORY_3010 =
    "結晶は機関を養い、炉から炉へと燃える。\n"
    "町の者は力を盗み、己のものと称する。\n"
    "その力を引き出せ、何ひとつ蓄えさせるな。\n"
    "彼らの罪は、天使の秤に載っている。\n"; // Context: Speaker: Seraphel. Before Level 10 (Gas Factory): Seraphel demands the return of stolen energy.
TXL_STORY_3110 =
    "機関の返すべき力は、地の底へ戻りたり。\n"
    "疫病の影は迫り、生ある者は減りゆこう。\n"
    "町の壁を越え、墓地へと来たれ。\n"
    "裁きが人々に何をなすか、そこで見よ。\n"; // Context: Speaker: Seraphel. After winning Level 10, before Level 11 (Grey Desert / Cemetery).
TXL_STORY_3011 =
    "われは座天使、ゴモラの罪は満ちたり。\n"
    "法なき行いが、その明日を奪った。\n"
    "われは己の重さを知る、降りるときの災いも。\n"
    "翼の下で、町を灰にする手を貸せ。\n"; // Context: Speaker: Thrones. Level 12 introduction: Thrones judges Gomorrah.
TXL_STORY_3111 =
    "ゴモラは燃え、煙は空へ昇る。\n"
    "塔は崩れ、おごれる礎は滅びる。\n"
    "されどこの火の先に、最後の務めがある。\n"
    "大地そのものが、火に答えねばならぬ。\n"; // Context: Speaker: Thrones. Level 12 completion, leading to Judgment Day.
TXL_STORY_3012 =
    "裁きの日は来たり、最後の秤は据えられた。\n"
    "大地は古き負債とともに、量られる。\n"
    "天は見守る、古き世のすべてが燃えるのを。\n"
    "かつて在りしものは、ことごとく火に帰る。\n"; // Context: Speaker: Thrones. Before Level 13 (Ashland / Judgment Day): Thrones announces the final trial.
TXL_STORY_3112 =
    "ついに成れり、大地は灰となった。\n"
    "最後の閃きとともに、古き世は消えた。\n"
    "悪は尽き、試しは終わり、罪もまた尽きた。\n"
    "人は誰も残らぬゆえに、ただそなたを除いて。\n"; // Context: Speaker: Thrones. Level 13 completion and campaign ending.
TXL_STORY_32000 =
    "われはエゼキエル、いまこそ明かそう。\n"
    "生者と死者のあいだで、そなたは試しを越えた。\n"
    "悪を滅ぼす手を貸し、世界は静まり返った。\n"
    "すべてはそなたの望みどおり、次は何をなす。\n"; // Context: Speaker: Ezekiel. Post-campaign result dialog before random-game free play.
TXL_STORY_OPTION_ENTER_TRAINING = "学校へ行く"; // Context: Choice button in Level 1 introductions/outros and the Level 2 recovery introduction: enter school.
TXL_STORY_OPTION_YES = "はい"; // Context: Confirmation button for switching school lessons; tutorial confirmation uses the separate YES_TUTORIAL key.
TXL_STORY_OPTION_NO = "いいえ"; // Context: Cancel button for switching school lessons; tutorial refusal uses the separate NO_TUTORIAL key.
TXL_STORY_OPTION_NOT_NOW = "今はやめる"; // Context: Choice after failing Level 1: decline training and try Level 1 again.
TXL_STORY_OPTION_COMPETE_VS_EZEKIEL = "エゼキエルと対戦"; // Context: Choice after passing Level 1 or in the Level 2 recovery introduction: proceed to face Ezekiel in Level 2.
TXL_STORY_OPTION_OK = "確認"; // Context: Acknowledgement button for campaign dialogue and school instructions.
TXL_STORY_OPTION_NEXT = "次へ"; // Context: School lesson completion button: advance to the next lesson; after Lesson 5, leave school.
TXL_STORY_OPTION_OK_TITLE = "確認"; // Context: Accept the suggested ball after struggling in school Lesson 5; English uses title-case Ok.
TXL_STORY_OPTION_DECLINE = "断る"; // Context: Decline the suggested ball after struggling in school Lesson 5.
TXL_STORY_OPTION_OPEN_OIL = "オイルを開く"; // Context: Button on the Level 3 oil reminder (story 30032): open the oil window.
TXL_STORY_OPTION_NO_THANKS = "やめておく"; // Context: Decline opening the oil window (story 30032) or shop (story 3141).
TXL_STORY_OPTION_OPEN_SHOP = "ショップを開く"; // Context: Button after Level 5 and before Cherubel in Level 6 (story 3141): open the shop.
TXL_STORY_OPTION_KEEP_PLAYING = "プレイを続ける"; // Context: Post-campaign button (story 32000): continue into random-game free play.
TXL_STORY_OPTION_COMPETE_VS_ANGEL = "天使と対戦"; // Context: Legacy choice label for facing an angel; not currently used by a story option.
TXL_STORY_OPTION_RESET_CAMPAIGN = "キャンペーンをやり直す"; // Context: Legacy campaign reset choice label; not currently used by a story option.
TXL_LOYALTY_TITLE = "毎日のログインボーナス";
TXL_LOYALTY_STREAK_FMT = "%d日連続ログイン中！";
TXL_LOYALTY_AWARD_FMT = "継続ボーナス：$%d";
TXL_LOYALTY_ACCEPT = "受け取る";
TXL_STORY_OPTION_YES_TAKE_ME_TO_THE_NEXT_LESSON = "はい、次のレッスンへ"; // Context: Legacy school choice to advance to the next lesson; not currently used by a story option.
TXL_STORY_OPTION_NO_I_WANT_TO_LEAVE_SCHOOL = "いいえ、学校を出たい"; // Context: Legacy school choice to leave school; not currently used by a story option.
TXL_STORY_OPTION_PRACTICE_MORE = "もっと練習する"; // Context: Legacy school choice to repeat practice; not currently used by a story option.
TXL_STORY_OPTION_BACK_TO_GAME = "ゲームに戻る"; // Context: Legacy school choice to return to gameplay; not currently used by a story option.
TXL_STORY_OPTION_YES_TUTORIAL = "はい"; // Context: Button following the first Level 1 introduction (story 2): accept the tutorial.
TXL_STORY_OPTION_NO_TUTORIAL = "いいえ"; // Context: Button following the first Level 1 introduction (story 2): decline the tutorial.
TXL_STORY_SPEAKER_ANGEL = "天使"; // Context: Generic angel speaker label in the dialogue panel. Named campaign angels use their character names instead.
TXL_STORY_SPEAKER_DEVIL = "悪魔"; // Context: Devil speaker label in the dialogue panel.
TXL_STORY_SPEAKER_MYSELF = "私"; // Context: Player/self speaker label in the dialogue panel.
TXL_STORY_SPEAKER_UNKNOWN = "???"; // Context: Fallback speaker label when no known speaker matches.
TXL_STORY_NAME_CHERUBEL = "ケルビム"; // Context: Character name shown above campaign dialogue: Levels 6-8.
TXL_STORY_NAME_SERAPHEL = "セラフィム"; // Context: Character name shown above campaign dialogue: Levels 9-11.
TXL_STORY_NAME_THRONES = "座天使"; // Context: Character name shown above campaign dialogue: Levels 12-13.
TXL_STORY_NAME_EZEKIEL = "エゼキエル"; // Context: Character name shown above campaign dialogue: Levels 1-5, school instructions, and the post-campaign result dialogue.

TXL_COMPLETED_TAB = "クリア済み"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_TAB = "続編"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_LATEST_FIVE = "最近のゲーム"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_NO_GAMES = "続編の記録はまだありません。"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_TOP_FIVE = "上位スコア"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_FILTER_ON = "現在の設定のみ"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_FILTER_OFF = "現在の設定のみ"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_PLACE = "#"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_WHEN = "いつ"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_SETTINGS = "設定"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_SCORE = "スコア"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_NO_SCORES = "この設定の記録はありません。"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_NEXT_SETTINGS = "次のゲームの設定"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_JUST_NOW = "たった今"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MINUTE_AGO = "%d分前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MINUTES_AGO = "%d分前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_HOUR_AGO = "%d時間前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_HOURS_AGO = "%d時間前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_DAY_AGO = "%d日前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_DAYS_AGO = "%d日前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_WEEK_AGO = "%d週間前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_WEEKS_AGO = "%d週間前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MONTH_AGO = "%dか月前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MONTHS_AGO = "%dか月前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_YEAR_AGO = "%d年前"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_YEARS_AGO = "%d年前"; // Used in: sequel screen (clayton/win_stack.h)
