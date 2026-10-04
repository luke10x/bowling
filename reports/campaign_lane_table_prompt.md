# Campaign lane table prompt

Use this prompt to produce the current campaign-lane balance table directly from
the code, without guessing from earlier reports.

```text
Read the campaign lane presets in /Users/lape/workspace/bowling/game.cpp,
inside Campaign_ApplyBiomePreset (the switch beginning around line 12033).
For each of the 13 entries in kCampaignLevels (around line 509), produce one
Markdown table row with these columns:

Level | Biome | Lane type | Intrinsic slip | Oil min | Oil max | Oil decay / m | Pushback

Rules:
- Use the actual biome-to-preset mapping from Campaign_ApplyBiomePreset, not
  the selectable-house catalog, because campaign overrides house values.
- Lane type is determined by laneTextureIdx: 0 = Classic House, 1 = Dry
  Fronts, 2 = Long Oil, 3 = Asym Split.
- Oil min is always 0, because lane oil thickness is clamped to zero.
- Oil max is houseLane.laneOilThickness.
- Pushback is houseLane.lanePushbackStrength.
- Calculate intrinsic slip using the same UI scale as
  /Users/lape/workspace/bowling/oil/oil_clay.h around line 148:
  intrinsic slip (%) = round(100 * (1 - laneFriction / 0.15)).
  State that higher is slipperier.
- Calculate Oil decay / m using the authoritative helper in game.cpp,
  OilWearDecayPerTravelEffective (around line 10171):
  max(0, oilThicknessDecayPerBallTravel) * 2.0.
- Use the player-facing name “Timber Zone” for CampaignBiome::JUNGLE and
  “Cemetery” for CampaignBiome::GREY_DESERT.
- Do not alter source files, balance values, or build the project. This is a
  read-only reporting task.

After the table, list the exact source files and functions used. Flag any
surprising value for review, but do not change it. In particular, call out
Gas Factory’s pushback if it is materially higher than the other Asym Split
campaign lanes.
```
