// Tegel translation source DSL - Lithuanian (lt_lt)
//
// Translator guidance: ../docs/translations.md
// Campaign lore: lore.md
//
// Preferred angel names:
// Ezekiel -> Ezekielis; Cherubel -> Kerubas; Seraphel -> Serafelė;
// Thrones -> Ofanija. Serafelė and Ofanija are feminine; inflect names naturally
// (e.g. Ezekielį, Kerubą, Serafelę, Ofaniją).
// Use Dausos for the spiritual realm, nitratai for the campaign resource,
// and NITRO for the pedal. Dausos and nitratai are Lithuanian-only concepts.
// Story verse uses four-line stanzas with a prophetic, archaic, rhyming voice;
// preserve the poet's word order and rhyme.
//
// Do not use internal aliases Malach, Dog, Beak, or Cow as character names.
// The angels deceive the player into a campaign of punishment; reveal this
// gradually. Do not name God.
// Story/dialogue belongs only in the Story and dialogue section below.
// Preserve wording, punctuation, line breaks, and visible stanza structure.
// Keep the Context notes beside story entries; they identify speakers, levels,
// lessons, and triggers. Keep keys in the same order in all four locale files.
//

TXL_MENU = "MENIU"; // Used in: main menu (clayton/menu_clay.h), gameplay/HUD (game.cpp)
TXL_RENAME = "Čytai"; // Used in: main menu (clayton/menu_clay.h)
TXL_GO_TO_SCHOOL = "Eiti į praktiką"; // Used in: main menu (clayton/menu_clay.h)
TXL_LANGUAGE = "Kalba"; // Used in: language selector (clayton/language_clay.h), main menu (clayton/menu_clay.h)
TXL_CAMPAIGN = "Kampanija"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_CAMPAIGN_TITLE = "KAMPANIJA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_LEVELS = "KAMPANIJOS LYGIAI"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESTART_CAMPAIGN = "PRADĖTI KAMPANIJĄ IŠ NAUJO"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_WINS = "PERGALĖS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_LOST = "PRALAIMĖTA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_QUIT = "NUTRAUKTA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BEST_SCORE = "REKORDAS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_TIME_TO_BEAT = "ĮVEIKIMO LAIKAS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BACK = "ATGAL"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_REPLAY = "KARTOTI"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PLAY = "ŽAISTI"; // Used in: campaign and results windows (clayton/win_stack.h), high-score window (hiscore/hiscore_clay.h), gameplay/HUD (game.cpp)
TXL_NOT_AVAILABLE = "NĖRA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PRACTICE = "Praktika"; // Used in: main menu (clayton/menu_clay.h), gameplay/HUD (game.cpp)
TXL_DEVICE_SHARE = "Įrenginio bendrinimas"; // DELETE IT Used in: main menu (clayton/menu_clay.h)
TXL_GAME_SETTINGS = "Nustatymai"; // Used in: main menu (clayton/menu_clay.h), game settings (settings/settings_clay.h)
TXL_TRACKER = "Trekeris"; // Used in: main menu (clayton/menu_clay.h)
TXL_CREDITS = "Kreditai"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_AD_MANAGER_TITLE = "REKLAMŲ VALDYMAS"; // Naudojama: reklamų valdymo lange
TXL_AD_SPONSOR_PROMPT = "Žodis mūsų sponsoriams"; // Naršyklės reklamos mygtukas
TXL_AD_CONTINUE = "Pereiti prie reklamos"; // Naršyklės reklamos veiksmo užrašas
TXL_AD_CREDIT_FMT = "Reklamų kreditas: %.1f / %.0f"; // Naudojama: reklamų valdymo lange
TXL_AD_CREDIT_BUCKET_FMT = "Laimėjimų rezervas: %.0f / %.0f (+%.0f, kai kredito mažai)"; // Naudojama: reklamų valdymo lange
TXL_AD_CREDIT_RULES_FMT = "Laimėjimas: -%d, pralaimėjimas: -%d, prisijungimas: +%d"; // Reklamų taisyklės
TXL_AD_HOLIDAY_ACTIVE = "Pertrauka nuo reklamų"; // Naudojama: reklamų valdymo lange
TXL_AD_HOLIDAY_INACTIVE = "Pertraukos nuo reklamų nėra"; // Naudojama: reklamų valdymo lange
TXL_AD_HOLIDAY_REMAINING_FMT = "Liko: %d val. %d min."; // Naudojama: reklamų valdymo lange
TXL_AD_HOLIDAY_END_FMT = "Baigsis: %s (vietos laiku)"; // Naudojama: reklamų valdymo lange
TXL_ADS_TODAY_FMT = "Šiandien parodyta reklamų: %d / %d"; // Naudojama: reklamų valdymo lange
TXL_ADS_TO_HOLIDAY_FMT = "Iki pertraukos liko reklamų: %d"; // Naudojama: reklamų valdymo lange
TXL_AD_MONEY_TODAY_FMT = "Šiandien žiūrėta už pinigus: %d / %d"; // Naudojama: reklamų valdymo lange
TXL_AD_MONEY_RULE_FMT = "Reklama už pinigus: $%d, %d per papildymą"; // Reklamų taisyklės
TXL_AD_HOLIDAY_RULE_FMT = "Po %d reklamų: %d val. pertrauka"; // Reklamų taisyklės
TXL_GET_REWARD = "GAUK"; // Naudojama: parduotuvės atlygio kortelėje
TXL_REWARD_CARD = "Atlygis"; // Parduotuvės atlygio kortelės pavadinimas
TXL_REWARD_LOADING = "ATLYGIS KRAUNAMAS"; // Rodoma, kol įkeliama atlygio reklama
TXL_WATCH_AD = "ŽIŪRĖTI REKLAMĄ"; // Naudojama: parduotuvės atlygio mygtuke
TXL_WATCH_PROMO = "ŽIŪRĖTI PRISTATYMĄ"; // H5 patvirtinimo laukimo metu
TXL_MINIGAMES = "Minižaidimai"; // Used in: campaign and results windows (clayton/win_stack.h), main menu (clayton/menu_clay.h)
TXL_SELECT_LANGUAGE = "Pasirinkti kalbą"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_ENGLISH = "Anglų"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_CHINESE = "Kinų"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_LITHUANIAN = "Lietuvių"; // Used in: language selector (clayton/language_clay.h)
TXL_LANGUAGE_JAPANESE = "Japonų"; // Used in: language selector (clayton/language_clay.h)
TXL_SOUND = "GARSAS"; // Used in: sound window (sounds/sound_clay.h), gameplay/HUD (game.cpp)
TXL_SONG = "Daina"; // Used in: sound window (sounds/sound_clay.h)
TXL_OIL = "ALYVA"; // Used in: oil window (oil/oil_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_BALLS = "KAMUOLIAI"; // Used in: shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_SHOP = "PARDUOTUVĖ"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_HI_SCORE = "Rekordas"; // Used in: gameplay/HUD (game.cpp)
TXL_PRACTICE_TITLE = "PRAKTIKA"; // Used in: gameplay/HUD (game.cpp)
TXL_FREESTYLE_TITLE = "LAISVAS STILIUS"; // IS IT REALLY USED Used in: gameplay/HUD (game.cpp)
TXL_TRY_AGAIN = "BANDYTI DAR KARTĄ"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_BONUS_GAME = "BONUSAS"; // Used in: gameplay/HUD (game.cpp)
TXL_BALL_CHARGE_FMT = "NITRATAI"; // Used in: gameplay/HUD (game.cpp)
TXL_NOS_NO_ENERGY = "Stinga nitratų!"; // Used in: gameplay/HUD (game.cpp)
TXL_GREETING_RESUME = "Ar norite tęsti?"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_GREETING_WELCOME = "Sveiki atvykę į šį boulingo žaidimą."; // h doubt if it is usee d Used in: campaign and results windows (clayton/win_stack.h)
TXL_GREETING_SUBLINE = "Spustelėkite TĘSTI, kad sugrįžtumėte į žaidimą."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_READY = "TĘSTI"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SELECT_ANGEL = "PASIRINKTI ANGELĄ"; //  Not used i bleieve Used in: gameplay/HUD (game.cpp)
TXL_SELECT_BOT = "PASIRINKTI BOTĄ"; // Same Used in NOto used: bot selection (bots/bots_clay.h), gameplay/HUD (game.cpp)
TXL_SELECT_HOUSE = "PASIRINKTI TAKĄ"; // Used in NOT Used: gameplay/HUD (game.cpp)
TXL_SWITCH_HOUSE = "KEISTI TAKĄ"; // Used in: house selection (houses/houses_clay.h), gameplay/HUD (game.cpp)
TXL_SELECT_BALL = "PASIRINKTI KAMUOLĮ"; // Used in: gameplay/HUD (game.cpp)
TXL_SELL_FOR_FMT = "PARDUOTI UŽ $ %d"; // Naudojama: inventoriaus pardavimo mygtuke
TXL_SELL_CONFIRM_TITLE = "Parduoti kamuolį"; // Naudojama: pardavimo patvirtinime
TXL_SELL_CONFIRM_ITEM_FMT = "Parduoti %s už $%d?"; // Naudojama: pardavimo patvirtinime
TXL_SELL_CONFIRM_LOSS_FMT = "Tai $%d nuostolis, palyginti su pradine kaina."; // Naudojama: pardavimo patvirtinime
TXL_SELL_CONFIRM_GAIN_FMT = "Tai $%d pelnas, palyginti su pradine kaina."; // Naudojama: pardavimo patvirtinime
TXL_CURRENT = "DABARTINIS"; // Used in: shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_INVENTORY = "INVENTORIUS"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_BUY_NOW = "PIRKTI"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), gameplay/HUD (game.cpp)
TXL_HOUSES = "Takai"; // Used in: house selection (houses/houses_clay.h)
TXL_SELECTED_FMT = "Pasirinkta: %s"; // Used in: house selection (houses/houses_clay.h), bot selection (bots/bots_clay.h)
TXL_SELECT_A_HOUSE = "Pasirinkite taką"; // Used in: house selection (houses/houses_clay.h)
TXL_NO_HOUSES = "Nėra takų"; // Used in: house selection (houses/houses_clay.h)
TXL_BOT_AVATAR = "Boto avataras"; //  NOT USEDUsed in: bot selection (bots/bots_clay.h)
TXL_SELECT_A_BOT_AVATAR = "Pasirinkite boto avatarą"; // NOT USED Used in: bot selection (bots/bots_clay.h)
TXL_NO_BOTS = "Nėra botų"; // Used in: not used bot selection (bots/bots_clay.h)
TXL_INVENTORY_EMPTY = "INVENTORIUS TUŠČIAS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_EMPTY = "PARDUOTUVĖ TUŠČIA"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h)
TXL_LOCKED = "UŽRAKINTA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_PASS = "ĮSK."; // Used in: campaign and results windows (clayton/win_stack.h), school window (school/school_clay.h)
TXL_RESETS_IN_FMT = "Naujos prekės už %s"; // Used in: shop window (clayton/shop_clay.h)
TXL_SOUND_SETTINGS = "Garso nustatymai"; // Used in: sound window (sounds/sound_clay.h)
TXL_CHANGING_QUALITY = "Keičiama kokybė..."; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_MODE = "Garso režimas"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_SYNTH = "SINTEZĖ"; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_OFF = "IŠJ."; // Used in: sound window (sounds/sound_clay.h)
TXL_AUDIO_PANIC = "PANIKA"; // Used in: sound window (sounds/sound_clay.h)
TXL_SDL_BUFFER = "SDL buferis"; // Used in: sound window (sounds/sound_clay.h)
TXL_MUSIC_VOLUME = "Muzikos garsumas"; // Used in: sound window (sounds/sound_clay.h)
TXL_OIL_STATUS = "Alyva"; // Used in: oil window (oil/oil_clay.h)
TXL_LANE_OIL_MAP = "Tako suteptumas"; // Used in: oil window (oil/oil_clay.h)
TXL_TRACK_INFO = "Tako informacija"; // Used in: oil window (oil/oil_clay.h)
TXL_MAX_OIL_LEVEL = "Maks. alyvos lygis"; // Used in: oil window (oil/oil_clay.h)
TXL_CURRENT_OIL_LEVEL = "Dabartinis"; // Used in: oil window (oil/oil_clay.h)
TXL_CARRYDOWN = "Pernešimas"; // Used in: oil window (oil/oil_clay.h)
TXL_OIL_DECAY = "Degradacija"; // Used in: oil window (oil/oil_clay.h)
TXL_SLIPPERY = "Slidumas"; // Used in: oil window (oil/oil_clay.h)
TXL_PUSHBACK = "Įcentr. stūma"; // Used in: oil window (oil/oil_clay.h)
TXL_REOILS_FMT = "Pertepimai: %d/%d"; // Used in: oil window (oil/oil_clay.h)
TXL_REOIL = "PERTEPTI"; // Used in: oil window (oil/oil_clay.h), school window (school/school_clay.h)
TXL_CANT_AFFORD = "Nepakanka lėšų"; // Used in: campaign and results windows (clayton/win_stack.h), shop window (clayton/shop_clay.h), oil window (oil/oil_clay.h)
TXL_REOIL_LOCKED = "PERTEPIMAS UŽRAKINTAS"; // Used in: oil window (oil/oil_clay.h)
TXL_FRESHLY_OILED = "ŠVIEŽIAI PERTEPTA"; // Used in: gameplay/HUD (game.cpp)
TXL_TOP_SCORES = "Rekordai"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_YOUR_SCORE_TOP_FMT = "Jūsų rezultatas tarp %d geriausių"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_GOOD_RUN = "Puikus žaidimas!"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_POINTS_FMT = "%d taškai"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_SCORE_HIGHER_THAN_FMT = "Jūsų rezultatas didesnis nei %.0f%% visų pastarųjų žaidimų"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_LEADERBOARD_LAST_HOUR = "Lyderių lentelė (paskutinė valanda)"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_PLAYER = "Žaidėjas"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_SCORE = "Rezultatas"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_AGE = "Laikas"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_NO_SCORES_YET = "Dar nėra rezultatų – būk pirmas!"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_ATTEMPTS_FMT = "Bandymai: %d"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_RANGE_FMT = "Diapazonas: %d–%d"; // Used in: high-score window (hiscore/hiscore_clay.h)
TXL_GAME_SETTINGS_TITLE = "Žaidimo nustatymai"; // Used in: game settings (settings/settings_clay.h)
TXL_HIGH_DETAILS = "Daugiau detalių"; // Used in: game settings (settings/settings_clay.h)
TXL_HIGH_DETAILS_HELP = "Papildomos vaizdo detalės ir dekoratyvūs efektai"; // Used in: game settings (settings/settings_clay.h)
TXL_PATCHED_YMFM = "Pataisytas YMFM"; // Used in: game settings (settings/settings_clay.h)
TXL_PATCHED_YMFM_HELP = "Naudoti eksperimentinį spartesnį FM sintezatorių; garsas paleidžiamas iš naujo"; // Used in: game settings (settings/settings_clay.h)
TXL_PERFORMANCE_ANALYSIS = "Našumo analizė"; // Used in: game settings (settings/settings_clay.h)
TXL_PERFORMANCE_ANALYSIS_HELP = "Kadrų grafikas ir laiko ataskaita kas 5 sek. (F3)"; // Used in: game settings (settings/settings_clay.h)
TXL_RESET_PROGRESS = "GAMYKLINIS RESETAS"; // Used in: campaign and results windows (clayton/win_stack.h), game settings (settings/settings_clay.h)
TXL_RESET_CAMPAIGN = "NAUJA KAMPANIJA"; // Used in: game settings (settings/settings_clay.h)
TXL_RESET_PROGRESS_HELP = "Nauja kampanija ištrina progresą, įskaitant lygių įrašus, auksą, kamuolius, atrakinimus ir runas. Gamyklinis atstatymas išvalo VISKĄ."; // Used in: game settings (settings/settings_clay.h)
TXL_CHECK_FOR_UPDATE = "TIKRINTI ATNAUJINIMĄ"; // Used in: game settings (settings/settings_clay.h)
TXL_UPDATE_PWA = "ATNAUJINTI PROGRAMĄ"; // Used in: game settings (settings/settings_clay.h)
TXL_UPDATE_WEB = "IŠ NAUJO ĮKELTI PROGRAMĄ"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_INSTALLED_FMT = "Įdiegta versija: %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_PUBLISHED_FMT = "Išleista versija: %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_CHECKING = "Tikrinamas atnaujinimas..."; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_UP_TO_DATE_FMT = "Naujausia versija jau įdiegta. Išleista versija: %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_AVAILABLE_FMT = "Yra atnaujinimas: %s -> %s"; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_OFFLINE = "Esate neprisijungę. Prisijunkite prie interneto ir bandykite dar kartą."; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_ERROR = "Nepavyko pasiekti atnaujinimų serverio."; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_APPLYING = "Taikomas atnaujinimas..."; // Used in: game settings (settings/settings_clay.h)
TXL_PWA_UPDATE_UNSUPPORTED = "Programos atnaujinimų tikrinimas galimas tik žiniatinklio programoje."; // Used in: game settings (settings/settings_clay.h)
TXL_CREDITS_AUTHOR_FMT = "Žaidimo dizainas, programavimas, menas ir garsas: %s."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_AUTHOR_NAME = "Lapė Kalė"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_CHARACTER_BASE = "Personažo pagrindas: Very Low Poly Human iš OpenGameArt.org."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_CHEST = "Skrynios modelis: CDmir, OpenGameArt.org (CC0)."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_MIXAMO = "Personažų animacijos iš Mixamo."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_TECH = "Sukurta naudojant SDL, OpenGL ES, Clay, GLM, Jolt Physics ir ymfm."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_TRACKER = "Visa muzika ir garso efektai sukurti paties žaidimo integruotu trakeriu."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CREDITS_THANKS = "Ypatinga padėka visiems, kurie žaidė, testavo ir metė dar vieną kartą."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SCHOOL_TITLE_FMT = "Pratybos %d. %s"; // Used in: school window (school/school_clay.h)
TXL_SCHOOL_PROGRESS_GOOD_SHOTS = "GERI METIMAI"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_LIGHT = "LENGVAS"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_HEAVY = "SUNKUS"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_HIT_ALL = "PATAIKYK Į VISUS 3"; // Used in: school progress row (school/school_clay.h)
TXL_SCHOOL_PROGRESS_REOILS = "PAKARTOTINIAI TEPIMAI"; // Used in: school progress row (school/school_clay.h)
TXL_AIM_LESSON = "Taikymasis"; // Used in: school window (school/school_clay.h)
TXL_BALL_MASS = "Kamuolio masė"; // Used in: school window (school/school_clay.h)
TXL_SPIN_BALL = "Suktukai"; // Used in: school window (school/school_clay.h)
TXL_OIL_AND_SKID = "Alyva ir slydimas"; // Used in: school window (school/school_clay.h)
TXL_STRIKE_LINE = "Straiko linija"; // Used in: school window (school/school_clay.h)
TXL_MASS_FMT = "MASĖ (%.1fKG)"; // Used in: school window (school/school_clay.h)
TXL_LIGHT_BALL_TEST = "Lengvi kamuoliai"; // Used in: school window (school/school_clay.h)
TXL_HEAVY_BALL_TEST = "Sunkaus kamuolio testas"; // Used in: school window (school/school_clay.h)
TXL_PASSED = "Įsk."; // Used in: school window (school/school_clay.h)
TXL_MASS = "MASĖ"; // Used in: shop window (clayton/shop_clay.h), school window (school/school_clay.h)
TXL_SPIN = "SUKIMASIS"; // Used in: shop window (clayton/shop_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_SKID = "SLYDIMAS"; // Used in: shop window (clayton/shop_clay.h)
TXL_BITE = "SUKIBIMAS"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_COMMON = "PAPRASTAS"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_RARE = "RETAS"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_EPIC = "EPINIS"; // Used in: shop window (clayton/shop_clay.h)
TXL_RARITY_LEGENDARY = "LEGENDINIS"; // Used in: shop window (clayton/shop_clay.h)
TXL_SPIN_TEST_FMT = "Suktukai: %d/%d taikinių (Lygis %d/%d)"; // Used in: school window (school/school_clay.h)
TXL_AIM_LESSON_POINTS_FMT = "Taikymo pamoka: %d/%d taškų"; // Used in: school window (school/school_clay.h)
TXL_PULL_BALL_BACK = "Užtempkite kamuolį"; // Used in: school window (school/school_clay.h)
TXL_LET_IT_GO_NOW = "Dabar paleiskite"; // Used in: school window (school/school_clay.h)
TXL_MOVE_RIGHT = "Dešinėn"; // Used in: school window (school/school_clay.h)
TXL_MOVE_LEFT = "Kairėn"; // Used in: school window (school/school_clay.h)
TXL_OIL_TEST_REOILS_FMT = "Alyvos pertepimai %d/%d"; // Used in: school window (school/school_clay.h)
TXL_REMAINING_OIL = "Likusi alyva"; // Used in: school window (school/school_clay.h)
TXL_REOIL_PROGRESS = "Pertepimo progresas"; // Used in: school window (school/school_clay.h)
TXL_WEAR_IT_DOWN = "Sudėvėkite alyvą prieš pertepdami."; // Used in: school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_STRIKE_TEST_SCORE_STRIKE = "Išmuškite straiką"; // Used in: school window (school/school_clay.h)
TXL_LEVEL1_TITLE = "1. DAUSOS"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL1_SUBTITLE = "Dvasių pasaulis  Surink 100, idant pereitum"; //  i think none of the subtitles are used Used in: gameplay/HUD (game.cpp)
TXL_LEVEL2_TITLE = "2. ANGELAS EZEKIELIS"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL2_SUBTITLE = "Vėjo malūnai  Stok prieš Ezekielį"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL3_TITLE = "3. NUODĖMIŲ DYKUMA"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL3_SUBTITLE = "Dykuma  Stok prieš Ezekielį"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL4_TITLE = "4. KUR ŠALTA IR SLIDU"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL4_SUBTITLE = "Ledas  Stok prieš Ezekielį"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL5_TITLE = "5. TEN DŪŽTA LANGAI"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL5_SUBTITLE = "Gomoros paribys  Stok prieš Ezekielį"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL6_TITLE = "6. ATVYKIMAS Į GOMORĄ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL6_SUBTITLE = "Gomora  Stok prieš Kerubą"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL7_TITLE = "7. ENERGETINĖS PIRAMIDĖS"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL7_SUBTITLE = "Dykuma  Stok prieš Kerubą"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL8_TITLE = "8. ALINANTI MEDKIRTYSTĖ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL8_SUBTITLE = "Miškas  Stok prieš Kerubą"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL9_TITLE = "9. SALIAMONO NITRATŲ KASYKLOS"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL9_SUBTITLE = "Nitratų kasyklos  Stok prieš Serafelę"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL10_TITLE = "10. RADIACINĖ ZONA"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL10_SUBTITLE = "Jėgainė  Stok prieš Serafelę"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL11_TITLE = "11. MARO PADARINAI"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL11_SUBTITLE = "Pilkoji dykuma  Stok prieš Serafelę"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL12_TITLE = "12. SUGRĮŽIMAS Į GOMORĄ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL12_SUBTITLE = "Sugriautas miestas  Stok prieš Ofaniją"; // Used in: gameplay/HUD (game.cpp)
TXL_LEVEL13_TITLE = "13. PASKUTINIO TEISMO DIENA"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_LEVEL13_SUBTITLE = "Pelenynas  Stok prieš Ofaniją"; // Used in: gameplay/HUD (game.cpp)
TXL_BALL_NAME_0 = "Žarija"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_1 = "Liepsna"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_2 = "Ugnkablys"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_3 = "Pragaras"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_4 = "Feniksas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_5 = "Šerkšnas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_6 = "Vėsa"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_7 = "Arktis"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_8 = "Ledynas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_9 = "Pūga"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_10 = "Žvaigždė"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_11 = "Ūkas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_12 = "Kosmosas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_13 = "Tuštuma"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_14 = "Juodoji"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_15 = "NSO"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_16 = "Ksenonas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_17 = "Miškas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_18 = "Vijoklis"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_19 = "Perkūnas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_20 = "Titanas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_21 = "Pasaulmedis"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_22 = "Šešėlis"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_23 = "Bedugnė"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_24 = "Anuliatorius"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_25 = "Grandinė"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_26 = "Neonas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_27 = "Kvantas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_28 = "Singuliarumas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_29 = "Banga"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_30 = "Rifas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_31 = "Krakenas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_32 = "Leviatanas"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_33 = "Runa"; // Used in: shop window (clayton/shop_clay.h)
TXL_BALL_NAME_34 = "Orakulas"; // Used in: shop window (clayton/shop_clay.h)
TXL_YOU = "tu"; // Used in: campaign and results windows (clayton/win_stack.h), new-game window (clayton/new_game_clay.h), high-score window (hiscore/hiscore_clay.h), gameplay/HUD (game.cpp)
TXL_BONUS_ROUND = "BONUSAS"; // Used in: gameplay/HUD (game.cpp)
TXL_TUTORIAL_FMT = "PRAKTIKA %d"; // Used in: gameplay/HUD (game.cpp)
TXL_COMPLETION_TIME_FMT = "BAIGIMO LAIKAS %s"; // Used in: gameplay/HUD (game.cpp)
TXL_AUDIO_BUFFER_INCREASED_FMT = "Dėl našumo garso buferis padidintas iki %d"; // Used in: gameplay/HUD (game.cpp)
TXL_SPIN_TO_HOOK = "Padaryk suktuką"; // Used in: gameplay/HUD (game.cpp)
TXL_DROP_HERE_TO_USE = "NUMESK ČIA\nNAUDOTI"; // Used in: gameplay/HUD (game.cpp)
TXL_CANNOT_USE_NOW = "DABAR\nNEGALIMA"; // Used in: gameplay/HUD (game.cpp)
TXL_NEED_AT_LEAST_2_BALLS = "REIKIA BENT\n2 KAMUOLIŲ"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_DEFAULT = "RUNA"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_LOST = "KAMUOLYS SPROGO"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_EVAPORATED = "KAMUOLYS IŠGARAVO"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BALL_SURVIVED = "KAMUOLYS ATSILAIKĖ"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_PINS_FROZEN = "KĖGLIAI UŽŠALO"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_PATROL_DEPLOYED = "IŠSTATYTI SARGAI"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_FOOTBALL = "FUTBOLO REŽIMAS"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_NITRO = "NITRO PRIPILDYTA"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_FLASH = "ŽAIBAS"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_SKULL = "KAUKOLĖ"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_ICY_BALL = "LEDOKŠLIS"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_OUTCOME_BOOM = "SPROGIMAS"; // Used in: gameplay/HUD (game.cpp)
TXL_MISSED = "PRO ŠALĮ"; // Used in: gameplay/HUD (game.cpp)
TXL_STALLED = "LAIKAS BAIGĖSI"; // Used in: gameplay/HUD (game.cpp)
TXL_SPLIT = "ŽERGLAS"; // Used in: gameplay/HUD (game.cpp)
TXL_STRIKE = "STRAIKAS"; // Used in: new-game window (clayton/new_game_clay.h), school window (school/school_clay.h), gameplay/HUD (game.cpp)
TXL_SPARE = "SPĖRAS"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_STRIKE_BONUS_NEXT_ROLLS = "Kiti du metimai prideda papildomų taškų"; // Gameplay toast after STRIKE banner
TXL_SPARE_BONUS_NEXT_ROLL = "Kitas metimas prideda papildomų taškų"; // Gameplay toast after SPARE banner
TXL_SCHOOL_HINT_HIT_PIN = "Pataikykite bent į vieną kėglį";
TXL_SCHOOL_HINT_LIGHT_BALL = "Pajuskite, koks lengvas kamuolys";
TXL_SCHOOL_HINT_HEAVY_BALL = "Pajuskite, koks sunkus kamuolys";
TXL_SCHOOL_HINT_SET_MASS = "Nustatykite kitokį kamuolio svorį";
TXL_SCHOOL_HINT_SPIN_BALL = "Įsukite kamuolį";
TXL_SCHOOL_HINT_FRESH_OIL = "Pažiūrėkite, kaip lengvai kamuolys dabar slysta";
TXL_SCHOOL_HINT_OIL_WEAR = "Alyvai dylant kamuoliui slysti bus sunkiau";
TXL_SCHOOL_HINT_REOIL_NOW = "Laikas iš naujo sutepti takelį";
TXL_SCHOOL_HINT_STRIKE_REQUIRED = "Norint išlaikyti reikia straiko";
TXL_SCHOOL_HINT_STRIKE_BONUS = "Straikas prideda kitus du metimus prie savo taškų";
TXL_SCHOOL_HINT_MORE_ENERGY = "Rinkite daugiau energijos stipresniam smūgiui";
TXL_SCHOOL_HINT_GEMS_ENERGY = "Nitratai suteikia energijos";
TXL_BLOCK_WOOD = "MEDIS"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_BRICK = "PLYTA"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_CONCRETE = "BETONAS"; // Used in: gameplay/HUD (game.cpp)
TXL_BLOCK_GLASS = "STIKLAS"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_TITLE = "LOBIS"; // Used in: gameplay/HUD (game.cpp)
TXL_TAP_TO_OPEN_CHEST = "BAKSTELK ATIDARYTI"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_RUNE_PICKED_FMT = "Gavai gebėjimą: %s"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEST_COINS_PICKED_FMT = "Gavai %d monetų"; // Used in: gameplay/HUD (game.cpp)
TXL_CONTINUE = "Tęsti"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_RUNE_NAME_BOOM = "Sprogimo runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_BOLT = "Žaibo runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_FREEZE = "Šalčio runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_SKULL = "Kaukolės runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_GUARD_PINS = "Sargų runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_FOOTBALL = "Futbolo runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_NITRO = "Nitro runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NAME_DEFAULT = "Runa"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_BOOM = "Sprogstamas metimas. Paaukoja kamuolį kad sukelti daug žalos."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_BOLT = "Žaibas gali sudeginti priešo kamuolį kai šis rieda."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_FREEZE = "Padengia ledu tavo kėglius."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_SKULL = "Kaukolė muša stipriau ir laužo blokus, arba suklaidina priešą."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_GUARD_PINS = "Išleidžia tris kėglius-sargus."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_FOOTBALL = "Spiria kamuolį aukštyn."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_NITRO = "Pripildo nitro atsargas. Naudokite, kai jų mažiau nei 80 %."; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_NITRO_TOO_FULL = "NITRO 80%+"; // Used in: gameplay/HUD (game.cpp)
TXL_RUNE_DESC_DEFAULT = "Prideda gebėjimą į runų dėklą."; // Used in: gameplay/HUD (game.cpp)
TXL_MINIGAMES_HELP = "Testinė prieiga. Atlygiai lieka, istorija ne."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_MINIGAME_COIN_RUSH = "AUKSO KARŠTINĖ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_MINIGAME_COUNT_MASTERS = "SKAIČIUOTOJAI"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_MINIGAME_CROWD_CONTROL = "MINIOS KONTROLĖ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_BONUS_PROMPT_FMT = "Žaisti bonusą?%s"; // Used in: gameplay/HUD (game.cpp)
TXL_RESET_PROGRESS_CONFIRM_TITLE = "Atstatyti progresą"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ARE_YOU_SURE = "Ar tikrai?"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESET_PROGRESS_FULL_DETAIL = "Išvalo kampanijos būseną, įskaitant lygių įrašus, kamuolius, atrakinimus ir runas, taip pat programos nustatymus."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RESET_PROGRESS_SOFT_DETAIL = "Išvalo kampanijos būseną, įskaitant lygių įrašus, kamuolius, atrakinimus ir runas."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_NO = "Ne"; // Used in: campaign and results windows (clayton/win_stack.h), house selection (houses/houses_clay.h), high-score window (hiscore/hiscore_clay.h), bot selection (bots/bots_clay.h), gameplay/HUD (game.cpp)
TXL_YES = "Taip"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_EXIT_BONUS_LEVEL = "Išeiti iš bonuso"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_EXIT_BONUS_LEVEL_CONFIRM = "Nutraukti vykstantį bonuso lygį?"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_STRIKES = "Straikai"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_SPARES = "Spėrai"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_COINS = "Auksas"; // Used in: new-game window (clayton/new_game_clay.h), gameplay/HUD (game.cpp)
TXL_WON_BY_POINTS = "Laimėta taškais"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_TOTAL = "Iš viso"; // Used in: campaign and results windows (clayton/win_stack.h), new-game window (clayton/new_game_clay.h)
TXL_COINS_OBTAINED = "GAUTOS MONETOS"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_VICTORY = "PERGALĖ"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_YOU_LOSE = "PRALAIMĖJAI"; // Used in: gameplay/HUD (game.cpp)
TXL_NEXT = "TOLIAU"; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_RETRY = "DAR KARTĄ"; // Used in: gameplay/HUD (game.cpp)
TXL_REPEAT = "KARTOTI"; // Used in: gameplay/HUD (game.cpp)
TXL_INVENTORY_SHORT = "INVENTORIUS"; // Used in: gameplay/HUD (game.cpp)
TXL_RELOADS_IN_FMT = "Prekės atsinaujina už %s"; // IS THIS DUBLICATE Used in: new-game window (clayton/new_game_clay.h)
TXL_RESTOCK_SOON = "NETRUKUS"; // Used in: new-game window (clayton/new_game_clay.h)
TXL_BONUS_LEVEL = "BONUSAS"; // Used in: gameplay/HUD (game.cpp)
TXL_PLAY_BONUS = "ŽAISTI BONUSĄ"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_COMPLETE = "BONUSAS BAIGTAS"; // Used in: gameplay/HUD (game.cpp)
TXL_COINS_PICKED_EARNED_FMT = "AUKSAS: %d   UŽDIRBTA: $%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_COMPLETE_FMT = "BONUSAS BAIGTAS  +$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_EXITED_EARNED_FMT = "NEBAIGTAS.   UŽDIRBTA: $%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_EXITED_FMT = "BONUSAS NUTRAUKTAS  +$%d"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_VICTORY = "BONUSO PERGALĖ"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_DEFEAT = "BONUSO PRALAIMĖJIMAS"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_ROUND_LOST = "BONUSAS PRALAIMĖTAS"; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_VICTORY_BASE = "Pergalė: sargai pasiekė priešo bazę."; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_DEFEAT_SPAWN = "Pralaimėjimas: priešai pasiekė tavo pradžią."; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_SPAWN_LABEL = "REGEN."; // Used in: gameplay/HUD (game.cpp)
TXL_CROWD_POWER_LABEL = "GALIA"; // Used in: gameplay/HUD (game.cpp)
TXL_VICTORY_DETAIL = "Pergalė."; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_DEFEAT_DETAIL = "Pralaimėjimas."; // Used in: campaign and results windows (clayton/win_stack.h), gameplay/HUD (game.cpp)
TXL_ENEMIES = "Priešai"; // Used in: gameplay/HUD (game.cpp)
TXL_BOSSES = "Bosai"; // Used in: gameplay/HUD (game.cpp)
TXL_SUPER_BOSS = "Super bosas"; // Used in: gameplay/HUD (game.cpp)
TXL_GATES = "Vartai"; // Used in: gameplay/HUD (game.cpp)
TXL_PINS = "Kėgliai"; // Used in: gameplay/HUD (game.cpp)
TXL_STANDERS = "Likę"; // Used in: gameplay/HUD (game.cpp)
TXL_YOU_BEAT_ANGEL = "ĮVEIKEI ANGELĄ"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ANGEL_WINS = "ANGELAS LAIMI"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BOT_SCORE_FMT = "Tu: %d    Angelas: %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_COMPLETE = "KAMPANIJA BAIGTA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_CAMPAIGN_COMPLETE_DETAIL = "Visi lygiai įveikti. Takas tavo."; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_TOTAL_TIME_FMT = "Laikas  %s"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ATTEMPTS_TOTAL_FMT = "Bandymai  %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_ENTER_CHEAT = "Įvesti kodą"; // Used in: gameplay/HUD (game.cpp)
TXL_CHEAT_ACTIVATED = "kodas įjungtas"; // Used in: gameplay/HUD (game.cpp)
TXL_NO_SUCH_CHEAT = "tokio kodo nėra"; // Used in: gameplay/HUD (game.cpp)
TXL_MAKES_YOU_SPAWN_FAST = "GREITESNĖ REGENERACIJA"; // Used in: gameplay/HUD (game.cpp)
TXL_MAKES_YOU_STRONG = "DAUGIAU JĖGOS"; // Used in: gameplay/HUD (game.cpp)
TXL_TURN_FMT = "%s EILĖ"; // Used in: gameplay/HUD (game.cpp)
TXL_NOS = "NITRO"; // Used in: gameplay/HUD (game.cpp)
TXL_BONUS_AVAILABLE = "YRA BONUSAS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_UPDATED = "PARDUOTUVĖ PAPILDYTA"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SHOP_RESTOCK_PROMPT = "Yra naujų kamuolių. Apsilankyti parduotuvėje prieš kitą žaidimą?"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_VISIT_SHOP = "Į PARDUOTUVĘ"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_LATER = "VĖLIAU"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_KEYPAD_ENTER_TEXT = "Įvesti tekstą"; // Used in: keypad HUD (clayton/keypad.h)
TXL_KEYPAD_ENTER_NUMBER = "Įvesti skaičių"; // Used in: numeric keypad (clayton/numkeypad.h)
TXL_DELETE = "Trinti"; // Used in: numeric keypad (clayton/numkeypad.h), keypad HUD (clayton/keypad.h)
TXL_ENTER = "Įvesti"; // Used in: numeric keypad (clayton/numkeypad.h), keypad HUD (clayton/keypad.h), gameplay/HUD (game.cpp)
TXL_STRESS = "Kirtis"; // Used in: keypad HUD (clayton/keypad.h)
TXL_POST_CAMPAIGN = "PO KAMPANIJOS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_SETTINGS = "NUSTATYMAI"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_SETTINGS = "NUSTATYMAI PO KAMPANIJOS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_BEST_SCORE = "Rekordas:"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_WON_FMT = "PERGALĖS %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_LOST_FMT = "PRALAIMĖTA %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_POST_CAMPAIGN_QUIT_FMT = "NUTRAUKTA %d"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BLOCKS = "BLOKAI"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_BLOCKS_SHORT = "B"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_NOS_SHORT = "N"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RUNES = "RUNOS"; // Used in: campaign and results windows (clayton/win_stack.h)
TXL_RUNES_SHORT = "R"; // Used in: campaign and results windows (clayton/win_stack.h)

// -----------------------------------------------------------------------------
// Story and dialogue
// -----------------------------------------------------------------------------
TXL_STORY_1 =
    "Čia Dausose, angelai stebi tavo metimą.\n"
    "Šimtas yra skaičius atrakinsiantis spyną.\n"
    "Nedaryti klaidų, straikus ir spėrus atlikti.\n"
    "Mokinkis skaičiuot tai tau padės nepaslysti\n"; // Context: Speaker: Ezekiel. Level 1's first-attempt introduction.
TXL_STORY_2 = "Ar norėtum atlikti praktiką?\n"; // Context: Speaker: Ezekiel. Follow-up prompt offering training from the first Level 1 introduction.
TXL_STORY_10 =
    "Šiandien šimto taškų tu nesurinkai,\n"
    "Pirmuosius vartus dar užrakintus radai.\n"
    "Patarčiau tau į mokyklą užsukti\n"
    "Gal žinios padės į kitą lygį prasmukti...\n"; // Ezekiel: Level 1 loss; school hints at another path beyond this gate.
TXL_STORY_11 = "Atlik praktiką arba bandyk dar kartą — ką renkiesi?\n"; // Context: Speaker: Ezekiel. Choice after the Level 1 failure message: train or replay.
// Ezekiel: Level 1 win with school unfinished; direct meeting invitation, then the school-or-Level-2 choice.
TXL_STORY_20 =
    "Šimtas taškų tau atvėrė vartus,\n"
    "Kiekvienas tavo metimas buvo taiklus.\n"
    "Aš Ezekielis; eik tiesiai pas mane,\n"
    "Šis lygis pereitas – lauksiu tavęs kitame.\n";
TXL_STORY_21 = "Ar vistik mokysiesi, ar eisi tiesiai pas mane?\n"; // Ezekiel: choice between school and meeting him in Level 2.
// Ezekiel: Level 1 win after school is complete; same invitation without another school offer.
TXL_STORY_22 =
    "Šimtas taškų tau atvėrė vartus,\n"
    "Kiekvienas tavo metimas buvo taiklus.\n"
    "Aš Ezekielis; eik tiesiai pas mane,\n"
    "Šis lygis pereitas – lauksiu tavęs kitame.\n";
TXL_STORY_30 =
    "Dar negali palikti praktikos.\n"
    "Pirma baik pamoką.\n"; // Context: Speaker: Ezekiel. Attempt to leave mandatory training before completing it.
TXL_STORY_31 = "Ar tikrai atšaukti šią pamoką ir eiti į kitą?\n"; // Context: Speaker: Ezekiel. Confirmation before replacing the current training attempt.
TXL_STORY_32 = "Ar tikrai pradėti pamoką iš pradžių\n"; // Context: Speaker: Ezekiel. Confirmation before restarting the current training lesson.
TXL_STORY_1000 =
    "Kiekvienas kamuolys turi savo masę, todel kitaip rieda ir visai kitas jausmas.\n"
    "Pirmas testas - lengvais kamuoliais pataikyti į kėglius.\n"
    "(Vėliau reikės mesti ir sunkius kamuolius).\n"; // Context: Speaker: Ezekiel. Lesson 2 introduction: ball mass.
TXL_STORY_1010 = "Puiku! Įsisisavinta pamoka apie masę.\n"; // Context: Speaker: Ezekiel. Lesson 2 completion.
TXL_STORY_1012 =
    "Kad islaikytum testa, mases slankikli nustatyk i lengva arba sunku gala.\n"
    "Metimas per viduri i pazanga neiskaiciuojamas.\n"; // Context: Speaker: Ezekiel. Lesson 2 reminder to use a light or heavy ball.
TXL_STORY_1013 = "Gerai. Dabar perjunk slankikli i sunkų galą ir pataikyk i kėglius.\n"; // Context: Speaker: Ezekiel. Lesson 2 prompt to use the heavy ball.
TXL_STORY_1014 = "Gerai. Dabar perjunk slankikli i lengvą galą ir pataikyk i kėglius.\n"; // Context: Speaker: Ezekiel. Lesson 2 prompt to use the light ball.
TXL_STORY_1030 = "Į mokyklą gali grizti bet kada.\n"; // Context: Speaker: Ezekiel. General return-to-school message after a lesson.
TXL_STORY_1020 =
    "Sveikinu. Sukimosi testa islaikei.\n"
    "Kiekvienas kamuolys savaip reaguoja i sukima.\n"
    "Kiti parametrai, pvz. sukibimas, keicia reakcijos stipruma.\n"; // Context: Speaker: Ezekiel. Lesson 3 completion: spin.
TXL_STORY_1022 =
    "3 pamoka. Kamuolio sukimas.\n"
    "Iskart po metimo suk kamuoli judesiais ekrane.\n"
    "Tada kamuolys ims krypti pasirinkta puse.\n"
    "Numusk visus lengvus taikinius, kad islaikytum (2 lygiai).\n"; // Context: Speaker: Ezekiel. Lesson 3 introduction: ball spin.
TXL_STORY_1052 =
    "4 pamoka. Alyva ir slydimas.\n"
    "Takas ka tik alyvuotas. Maždaug puse ar du trecdaliai tako padengta pilna alyva.\n"
    "Sioje pamokoje alyva dyla labai greitai, todel po keliu metimu jausmas pasikeis.\n"
    "Kai kurios sales pacios slidesnes, o kamuoliai turi slydimo parametra.\n"; // Context: Speaker: Ezekiel. Lesson 4 introduction: oil and skid.
TXL_STORY_1054 =
    "Takui jau reikia naujos alyvos.\n"
    "Atidaryk alyvos langa ir peralyvuok pries kita metima.\n"; // Context: Speaker: Ezekiel. Lesson 4 prompt when the lane needs fresh oil.
TXL_STORY_1060 = "Puiku! Alyvos testa islaikei.\n"; // Context: Speaker: Ezekiel. Lesson 4 completion.
TXL_STORY_1032 =
    "1 pamoka. Taikymas.\n"
    "Dabar mokysimes mesti.\n"
    "Patrauk kamuoli iki galo atgal, laikyk per viduri ir paleisk.\n"
    "Jei pataikysi i bet kuri kegli, gausi taska.\n"; // Context: Speaker: Ezekiel. Lesson 1 introduction: aiming.
TXL_STORY_1040 = "Puiku! Taikymo testa islaikei.\n"; // Context: Speaker: Ezekiel. Lesson 1 completion.
TXL_STORY_1070 =
    "5 pamoka. Straiko linija.\n"
    "Sek auksą. Linija nukrypsta nuo vidurio ir grįžta į kišenę.\n"
    "Tikslas – išmesti straiką.\n"
    "Gali spausti „Keisti liniją“ ir treniruotis kitoje pusėje.\n"; // Context: Speaker: Ezekiel. Lesson 5 introduction: strike line.
TXL_STORY_1072 =
    "Puiku! Straikas.\n"
    "Baigei mokykla.\n"
    "Gali sugrizti bet kada.\n"; // Context: Speaker: Ezekiel. Lesson 5 and school completion.
TXL_STORY_1080 =
    "Matau, kad sunku.\n"
    "Gal pabandyk si kamuoli?\n"; // Context: Speaker: Ezekiel. Suggested ball after the player struggles in training.
TXL_STORY_1021 =
    "Sukimą gali dar treniruoti zigzago pamokoje.\n"
    "3 pamoka dabar atrakinta.\n"; // Context: Speaker: Ezekiel. Lesson 3 unlock notification after the zig-zag gold lesson.
TXL_STORY_40 = "Nelabai kaip sekasi? Gal verčiau atlikime praktiką, jeigu surinkti šimtą tau per sunku...\n"; // Legacy Level 1 replay introduction; no longer routed.

// Context: Speaker: Ezekiel. Before Level 2 (Wind Farm): Ezekiel introduces himself and promises that the player can save the world; the promise is deceptive (see lore.md).
TXL_STORY_3002 =
    "Aš, Ezekielis, pirmasis angelas didus.\n"
    "Aš stebėjau tave, ir tavo metimus\n"
    "Tavo talentas padės išgelbėti pasaulį.\n"
    "Kurį seniai užvaldė blogis ir apgaulė.\n";
TXL_STORY_3102 =
    "Turiu pasakyti kad tikrai neblogai\n"
    "Kamuolį per lentą tvirtai ridenai\n"
    "Toliau per dykumą veda šis kelias.\n"
    "Smėlis padengs nuodėmingas detales.\n"; // Context: Speaker: Ezekiel. Level 2 completion, leading to the Red Desert.
TXL_STORY_3003 =
    "Be lašo vandens, karšta ir sausa,\n"
    "Saulėje svyla ši plati dykuma.\n"
    "Sunkiai rieda per lentą, smėlis užbyra\n"
    "Teks gerai įtrinti smilkalais ir alyva\n"; // Context: Speaker: Ezekiel. Level 3 (Red Desert) introduction.
TXL_STORY_30031 =
    "Jei kalsi iš centro kėgliai išsižergs,\n"
    "Tarpas per vidurį kitam metimui pakenks.\n"
    "Gal iš šono apsimokėtų labiau,\n"
    "Suktą paleisti ir mušt dar stipriau?\n"; // Context: Speaker: Ezekiel. Level 3 side-throw harassment after repeated splits.
TXL_STORY_30032 =
    "Turi galvot ką darai kai aplink dykuma\n"
    "Alyva džiūsta greičiau kai žemė sausa.\n"
    "Jei nori kad slystų taip kaip anksčiau\n"
    "Sutepti tau lentą reikėtų dažniau\n"; // Context: Speaker: Ezekiel. Level 3 oil reminder when the player has not re-oiled.
TXL_STORY_3042 =
    "Gal tau patinka kai į tave krenta stiklai,\n"
    "Nes mest atgal, pastebėjau, bijai.\n"
    "Stabdomas stiklo kamuolys greitį numes,\n"
    "Tik paleisk laiku, nes teks gaudyt šukes.\n"; // Context: Speaker: Ezekiel. Level 5 glass reminder when the player has not thrown glass back.
TXL_STORY_3050 =
    "Runos padės tau įveikti mane,\n"
    "Bet tam jos turi atsidurti rate\n"
    "Tempk runą į ratą kad ją panaudoti\n"
    "Tau spresti ką taupyti, o ką išeikvoti\n"; // Context: Speaker: Ezekiel. First rune discovery during the campaign: generic guidance on dragging a rune into the casting ring.
TXL_STORY_3051 =
    "O, juk tai gali sukelti sprogimą?\n"
    "Aukoti kamuolį už žalos sukėlimą\n"
    "Kai ši runa atsidurs burtų rate,\n"
    "Viską išdaužys galinga banga.\n"; // Context: Speaker: Ezekiel. Follow-up tutorial after the player first finds a Boom rune.
TXL_STORY_3052 =
    "Jei mesi runą į ratą kol nevėlu,\n"
    "Degint mano kamuolį gali su trenksmu,\n"
    "Žaibu sustabdyti ilgą metimą;\n"
    "Bandysiu vis tiek pabėgt nuo likimo.\n"; // Context: Speaker: Ezekiel. Follow-up tutorial after the player first finds a Bolt rune.
TXL_STORY_3053 =
    "Runos padės tau įveikti mane,\n"
    "Bet tam jos turi atsidurti rate\n"
    "Tempk runą į ratą kad ją panaudoti\n"
    "Tau spresti ką taupyti, o ką išeikvoti\n"; 
    // Context: Speaker: Ezekiel. First Boom rune discovery during the campaign: casting-ring guidance, automatically followed by the Boom warning in story 3051.
TXL_STORY_3054 =
    "Runos padės tau įveikti mane,\n"
    "Bet tam jos turi atsidurti rate\n"
    "Tempk runą į ratą kad ją panaudoti\n"
    "Tau spresti ką taupyti, o ką išeikvoti\n"; 
TXL_STORY_3103 =
    "Tavęs nestabdo nei karštis, nei sausra\n"
    "Bausmė jau arti, greit ateis valanda\n"
    "Kai valėme pasaulį nuo blogio karščiu\n"
    "Greit pabandysime ją paveikti šalčiu.\n"; // Context: Speaker: Ezekiel. Level 3 completion, leading to the icy Level 4.
TXL_STORY_3004 =
    "Kad apsaugoti žemę ją padengsim ledu.\n"
    "Ko karštis pagailėjo, sunaikinsim šalčiu\n"
    "Kamuolys čiuožia greit per sustingusį taką\n"
    "Svarbu nenuslysti, kai suriečia į ragą\n"; // Context: Speaker: Ezekiel. Level 4 introduction.
TXL_STORY_3104 =
    "Viena aišku - ledas tau nebaisus.\n"
    "Kas moka juo čiuožti tikrai nepražus.\n"
    "Po šalčių sniegas ištirps pagaliau\n"
    "O blogį naikinti mes eisim toliau.\n"; // Context: Speaker: Ezekiel. Level 4 completion, leading to the broken-glass level.
TXL_STORY_3040 =
    "Link nuodėmių miesto mes vis arčiau\n"
    "Darbą atlikti mums reikia tačiau,\n"
    "Langus daužyti yra baisiai smagu,\n"
    "Jie to nusipelnė net su kaupu.\n"; // Context: Speaker: Ezekiel. Before Level 5 (Suburbia): Ezekiel introduces the broken-glass lesson; Cherubel is encountered in Level 6.
TXL_STORY_3041 =
    "Stiklą kurį aš metu į tave\n"
    "Pramušt nesunku - tai tau nebėda.\n"
    "Tik noriu priminti kad kelio gale\n"
    "Tu irgi gali mest stiklus į mane.\n"; // Context: Speaker: Ezekiel. Level 5 first-glass tutorial.
TXL_STORY_3140 =
    "Mano kelias čia baigtas, aš toliau negaliu.\n"
    "Kerubas nuo šiol tebūnie tavo vedliu.\n"
    "Leisiu tau eiti į miestą ir jį pasitikt,\n"
    "Blogio naikinimo darbus atlikt.\n"; // Context: Speaker: Ezekiel. Level 5 completion, before meeting Cherub.
TXL_STORY_3141 =
    "Prieš eidamas toliau, aplankyk parduotuvę.\n"
    "Skirtingi kamuoliai — kokių tik nebūna;\n"
    "Kiekvieno kamuolio yra unikalios savybės,\n"
    "Ir ne visi kamuoliai vienodos kokybės.\n"; // Context: Speaker: Ezekiel. First shop prompt before the Cherub encounter.
TXL_STORY_3005 =
    "Aš esu Kerubas. O kas esi tu?\n"
    "Čia nuodėmių miestas, Gomora vardu.\n"
    "Mes rinksime nitratus ir daužysime stiklus,\n"
    "Galiausiai nubausime niekšus visus.\n"; // Context: Speaker: Cherubel. Level 6 (Gomorrah) introduction by Cherub.
TXL_STORY_3105 =
    "Už tai kaip jie gyvena, ateis jų valanda\n"
    "Jie bus nubausti, tai angelų pareiga.\n"
    "Miestas netrukus ims virsti griuvėsiais.\n"
    "Išvykti greičiau mes todėl paskubėsim.\n"; // Context: Speaker: Cherubel. Level 6 completion, leading to the Energy Pyramid.
TXL_STORY_3006 =
    "Vartai į visatą atsiveria čia,\n"
    "Pro šią piramidę jėgą siubiančią,\n"
    "Mes semsime energiją iš žemės gelmių.\n"
    "Net jei ir turės tai baisių pasekmių.\n"; // Context: Speaker: Cherubel. Level 7 (Energy Pyramid) introduction.
TXL_STORY_30061 =
    "Degink nitratus, nieko nelauk.\n"
    "Spausk \"NITRO\" pedalą ir neapsigauk.\n"
    "Tegul kamuolys greitai iššauna,\n"
    "Kai didelį greitį jis pasigauna.\n"; // Context: Speaker: Cherubel. Level 7 NOS reminder when it has not been used.
TXL_STORY_3106 =
    "Kas buvo nulemta, tas jau padaryta.\n"
    "Didinga nitratų galia užvaldyta.\n"
    "Toliau ją naudosime savo reikmėms,\n"
    "Paruošime žmoniją žiaurioms pasekmėms.\n"; // Context: Speaker: Cherubel. Level 7 completion, leading to the forest.
TXL_STORY_3007 =
    "Be medžių - nėr oro; Be oro nėr vandens;\n"
    "Be vandens - nebus gyvybės; jie neišgyvens;\n"
    "Mes kirsime miškus, kol čia nieko nebus,\n"
    "Jie gyveno nuodėmingai ir todėl jie pražus.\n"; // Context: Speaker: Cherubel. Level 8 (Forest) introduction.
TXL_STORY_3107 =
    "Net gerą metimą stabdo blokas medinis,\n"
    "deja, tarp mūsų šis bus paskutinis.\n"
    "Toliau tu keliausi į Serafelės valdas,\n"
    "Tau teks nusileisti į tamsias kasyklas.\n"; // Context: Speaker: Cherubel. Level 8 completion, leading to Seraphel's mines.
TXL_STORY_3008 =
    "Tegul lieka tai mūsų, angelų, paslaptis:\n"
    "Už jų niekšybes jiems buvo atsiųsta mirtis.\n"
    "Gyventojų skaičius sumažintas maro būdu;\n"
    "Žmonės išlaisvinti nuo pražūtingų minčių.\n"; // Context: Speaker: Seraphel. Before Level 11 (Grey Desert / Cemetery): Seraphel describes the plague at Gomorrah.
TXL_STORY_3108 =
    "Mano darbas baigtas, bet tau tik pradžia.\n"
    "Gomora privalo sudegti nakčia.\n"
    "Atėjo laikas mums išsiskirti;\n"
    "Kitas angelas padės tau miestą naikinti.\n"; // Context: Speaker: Seraphel. After winning Level 11, before Level 12: Seraphel departs and sends the player to Thrones in Gomorrah.
TXL_STORY_3009 =
    "Aš — Serafelė, o čia kristalų saugyklos,\n"
    "Karaliaus Saliamono nitratų kasyklos.\n"
    "Nitratų sankloda giliai po žeme\n"
    "Prasiverš į paviršių, kai išauš valanda.\n"; // Context: Speaker: Seraphel. Level 9 (Mines) introduction by Seraphel.
TXL_STORY_3109 =
    "Avinėlis nuplėšia antspaudą dar vieną.\n"
    "Karalystė ateina arčiau kiekvieną dieną.\n"
    "Jų likimą nulėmė ženklai danguje —\n"
    "Šviečia aiškiai: tai išnykimo pradžia.\n"; // Context: Speaker: Seraphel. Level 9 completion, leading to the factory.
TXL_STORY_3010 =
    "Apsistatę mechanizmais, fabrikais ir mašinomis;\n"
    "Jie siurbia galią, kurią mes gaminame.\n"
    "Laikas atėjo viską atiduoti,\n"
    "Nubausti ir nedorybes pažaboti.\n"; // Context: Speaker: Seraphel. Before Level 10 (Gas Factory): Seraphel demands the return of stolen energy.
TXL_STORY_3110 =
    "Grįžta atgal į žemės gelmes.\n"
    "Jie privalo grąžinti sielas visas.\n"
    "O jei nespės iki tol sugrąžinti,\n"
    "Mes paleisime ugnį galutinai sunaikinti.\n"; // Context: Speaker: Seraphel. After winning Level 10, before Level 11 (Grey Desert / Cemetery).
TXL_STORY_3011 =
    "Aš esu ketvirtojo antspaudo angelas,\n"
    "Šitas miestas privalo degti kaip fakelas.\n"
    "Jie pagarsėjo savo nuodėmingais darbais.\n"
    "Laikas atėjo pavirst pelenais.\n"; // Context: Speaker: Thrones. Level 12 introduction: Thrones judges Gomorrah.
TXL_STORY_3111 =
    "Gomora liepsnoja, ir jos dūmai kyla.\n"
    "Pelenais virsta visa, kas žemėje gyva.\n"
    "Kitas darbas tau jau bus paskutinis —\n"
    "Galutinai sunaikinsime blogio šaltinį.\n"; // Context: Speaker: Thrones. Level 12 completion, leading to Judgment Day.
TXL_STORY_3012 =
    "Tai Teismo diena, pabaigos pradžia.\n"
    "Pasibaigs pagaliau jų amžinoji kančia.\n"
    "Žemė pasverta, jie pripažinti kaltais;\n"
    "Viskas sudegs ir pavirs pelenais.\n"; // Context: Speaker: Thrones. Before Level 13 (Ashland / Judgment Day): Thrones announces the final trial.
TXL_STORY_3112 =
    "Blogis sunaikintas, išbandymas baigtas,\n"
    "Senasis pasaulis buvo prakeiktas.\n"
    "Išvalytas ugnimi, dabar jau ramu;\n"
    "Neliko nuodėmės, nes nebeliko žmonių.\n"; // Context: Speaker: Thrones. Level 13 completion and campaign ending.
TXL_STORY_32000 =
    "Ar prisimeni mane? Aš sugrįžau.\n"
    "Blogis nugalėtas — už tai ačiū tik tau.\n"
    "Grįžau pasveikinti: tau pavyko sunaikinti.\n"
    "Ką darysi toliau — gali pasirinkti.\n"; // Context: Speaker: Ezekiel. Post-campaign result dialog before random-game free play.
TXL_STORY_OPTION_ENTER_TRAINING = "Eiti į praktiką"; // Context: Choice button in Level 1 introductions/outros and the Level 2 recovery introduction: enter school.
TXL_STORY_OPTION_YES = "Yes"; // Context: Confirmation button for switching school lessons; tutorial confirmation uses the separate YES_TUTORIAL key.
TXL_STORY_OPTION_NO = "No"; // Context: Cancel button for switching school lessons; tutorial refusal uses the separate NO_TUTORIAL key.
TXL_STORY_OPTION_NOT_NOW = "Ne dabar"; // Context: Choice after failing Level 1: decline training and try Level 1 again.
TXL_STORY_OPTION_COMPETE_VS_EZEKIEL = "Varžytis su Ezekieliu"; // Context: Choice after passing Level 1 or in the Level 2 recovery introduction: proceed to face Ezekiel in Level 2.
TXL_STORY_OPTION_OK = "Gerai"; // Context: Acknowledgement button for campaign dialogue and school instructions.
TXL_STORY_OPTION_NEXT = "Toliau"; // Context: School lesson completion button: advance to the next lesson; after Lesson 5, leave school.
TXL_STORY_OPTION_OK_TITLE = "Gerai"; // Context: Accept the suggested ball after struggling in school Lesson 5; English uses title-case Ok.
TXL_STORY_OPTION_DECLINE = "Atsisakyti"; // Context: Decline the suggested ball after struggling in school Lesson 5.
TXL_STORY_OPTION_OPEN_OIL = "Atidaryti alyvą"; // Context: Button on the Level 3 oil reminder (story 30032): open the oil window.
TXL_STORY_OPTION_NO_THANKS = "Ne, ačiū"; // Context: Decline opening the oil window (story 30032) or shop (story 3141).
TXL_STORY_OPTION_OPEN_SHOP = "Atidaryti parduotuvę"; // Context: Button after Level 5 and before Cherubel in Level 6 (story 3141): open the shop.
TXL_STORY_OPTION_KEEP_PLAYING = "Žaisti toliau"; // Context: Post-campaign button (story 32000): continue into random-game free play.
TXL_STORY_OPTION_COMPETE_VS_ANGEL = "Varžytis su angelu"; // Context: Legacy choice label for facing an angel; not currently used by a story option.
TXL_STORY_OPTION_RESET_CAMPAIGN = "Nauja kampanija"; // Context: Legacy campaign reset choice label; not currently used by a story option.
TXL_LOYALTY_TITLE = "KASDIENIS LOJALUMO APDOVANOJIMAS";
TXL_LOYALTY_STREAK_FMT = "Prisijungiate jau %d dienas iš eilės!";
TXL_LOYALTY_AWARD_FMT = "Jūsų lojalumo dovana: $%d";
TXL_LOYALTY_ACCEPT = "PRIIMTI";
TXL_STORY_OPTION_YES_TAKE_ME_TO_THE_NEXT_LESSON = "Taip, veskite mane į kitą pamoką"; // Context: Legacy school choice to advance to the next lesson; not currently used by a story option.
TXL_STORY_OPTION_NO_I_WANT_TO_LEAVE_SCHOOL = "Ne, noriu išeiti iš mokyklos"; // Context: Legacy school choice to leave school; not currently used by a story option.
TXL_STORY_OPTION_PRACTICE_MORE = "Praktikuotis dar"; // Context: Legacy school choice to repeat practice; not currently used by a story option.
TXL_STORY_OPTION_BACK_TO_GAME = "Atgal į žaidimą"; // Context: Legacy school choice to return to gameplay; not currently used by a story option.
TXL_STORY_OPTION_YES_TUTORIAL = "Taip"; // Context: Button following the first Level 1 introduction (story 2): accept the tutorial.
TXL_STORY_OPTION_NO_TUTORIAL = "Ne"; // Context: Button following the first Level 1 introduction (story 2): decline the tutorial.
TXL_STORY_SPEAKER_ANGEL = "ANGELAS"; // Context: Generic angel speaker label in the dialogue panel. Named campaign angels use their character names instead.
TXL_STORY_SPEAKER_DEVIL = "VELNIAS"; // Context: Devil speaker label in the dialogue panel.
TXL_STORY_SPEAKER_MYSELF = "AŠ"; // Context: Player/self speaker label in the dialogue panel.
TXL_STORY_SPEAKER_UNKNOWN = "???"; // Context: Fallback speaker label when no known speaker matches.
TXL_STORY_NAME_CHERUBEL = "Kerubas"; // Context: Character name shown above campaign dialogue: Levels 6-8.
TXL_STORY_NAME_SERAPHEL = "Serafelė"; // Context: Character name shown above campaign dialogue: Levels 9-11.
TXL_STORY_NAME_THRONES = "Ofanija"; // Context: Character name shown above campaign dialogue: Levels 12-13.
TXL_STORY_NAME_EZEKIEL = "Ezekielis"; // Context: Character name shown above campaign dialogue: Levels 1-5, school instructions, and the post-campaign result dialogue.

TXL_COMPLETED_TAB = "BAIGTA"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_TAB = "TĘSINYS"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_LATEST_FIVE = "NAUJAUSI ŽAIDIMAI"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_NO_GAMES = "Tęsinyje žaidimų dar nėra."; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_TOP_FIVE = "GERIAUSI REZULTATAI"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_FILTER_ON = "Tik dabartiniai nustatymai"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_FILTER_OFF = "Tik dabartiniai nustatymai"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_PLACE = "#"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_WHEN = "KADA"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_SETTINGS = "NUSTATYMAI"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_SCORE = "TAŠKAI"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_NO_SCORES = "Šiems nustatymams rezultatų nėra."; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_NEXT_SETTINGS = "KITO ŽAIDIMO NUSTATYMAI"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_JUST_NOW = "ką tik"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MINUTE_AGO = "prieš %d minutę"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MINUTES_AGO = "prieš %d minutes"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_HOUR_AGO = "prieš %d valandą"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_HOURS_AGO = "prieš %d valandas"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_DAY_AGO = "prieš %d dieną"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_DAYS_AGO = "prieš %d dienas"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_WEEK_AGO = "prieš %d savaitę"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_WEEKS_AGO = "prieš %d savaites"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MONTH_AGO = "prieš %d mėnesį"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_MONTHS_AGO = "prieš %d mėnesius"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_YEAR_AGO = "prieš %d metus"; // Used in: sequel screen (clayton/win_stack.h)
TXL_SEQUEL_YEARS_AGO = "prieš %d metus"; // Used in: sequel screen (clayton/win_stack.h)
