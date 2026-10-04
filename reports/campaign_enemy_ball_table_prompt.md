# Campaign enemy-ball table prompt

Use this prompt to generate an up-to-date, read-only report of campaign enemy
balls. Do not copy values from an older report.

```text
Read the campaign configuration in
/Users/lape/workspace/bowling/game.cpp, specifically kCampaignLevels near line
509. For every level, report the initial enemy ball configured by enemyBallId.

Read ball names, themes/families, prices, and core stats from g_ballCatalog in
/Users/lape/workspace/bowling/shop.h. Read lane types from
Campaign_ApplyBiomePreset in game.cpp: laneTextureIdx 0 = Classic House, 1 =
Dry Fronts, 2 = Long Oil, 3 = Asym Split.

Produce one Markdown table with exactly these columns:

Level | Biome | Lane type | Opponent | Initial ball | Ball ID | Ball family |
Price | Mass | Radius | Spin | Skid | Bite

Rules:
- Level 1 is solo. Show em dashes for its opponent and ball fields instead of
  treating its unused configuration value as an enemy ball.
- Use player-facing biome names: Timber Zone for CampaignBiome::JUNGLE and
  Cemetery for CampaignBiome::GREY_DESERT.
- Preserve the configuration order; do not sort levels or infer a replacement
  ball.
- This is about the initial ball only. Do not substitute from the cheap enemy
  replacement pool used after a ball is destroyed.
- After the table, summarize the price progression separately for Ezekiel,
  Dog, Beak, and Cow. Flag any descending price step, but do not modify code.
- State the exact source files and symbols used.
- Do not alter source files or build the project.
```
