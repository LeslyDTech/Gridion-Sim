# Asset notes

## Players — `sprites/players/`
- 21 x 31 px each, top-down view. Shoulders run along the long axis, so the sprite **faces right (+X) by default**. Rotate from there to match movement direction.
- `blue_1..5`, `orange_1..5`, `white_1..5`: same jersey color, five skin tones. Mix them per team.
- At 1x a player is about 1.6 yards wide on the suggested field scale. Drawing them at 1.5x looks good; use point filtering (raylib's default) so edges stay crisp.
- Suggested teams: blue (offense) vs orange (defense). White is a spare (alternate jerseys or practice squad).

## Ball — `sprites/ball_football.png`
- 14 x 16 px. Rotate along the flight path.

## Field — `tiles/field_grass.png`
- Tilesheet, 832 x 1024 px, **64 x 64 px tiles** (13 columns x 16 rows), no spacing or margin.
- Plain mown grass tile: column 0, row 0 → source rect (0, 0, 64, 64).
- Digit tiles 0–9: row 15, column = digit → source rect (digit * 64, 960, 64, 64).
  These tiles include the grass background, so draw them at exactly 64 x 64 on tile boundaries. Scaled or offset, they show a visible box. For smaller or rotated yard numbers, use a TTF font instead.
- The sheet's line tiles are soccer markings, so draw yard lines, hash marks, sidelines, and end-zone lines in code on top of the grass.
- Scale suggestion: 1 tile = 5 yards, so 1 yard = 12.8 px at 1x zoom. A 120-yard field (with end zones) is 24 tiles long; 53.3 yards wide is about 11 tiles.

## How Gridiron Sim uses these

- **Players:** blue = offense, orange = defense; white is unused. Skin tone = jersey number % 5,
  so a player looks the same every play. Each sprite is scaled to its player's collision circle
  (shoulders = 2.6 x radius) instead of the suggested 1.5x, so what you see matches the physics,
  and rotated to the player's facing direction. Trilinear filtering with mipmaps is used instead
  of point filtering, because the sprites are drawn rotated and smaller than 1x.
- **Ball:** the sprite's long axis is at ~55 degrees, so that angle is subtracted when it is
  rotated along the flight path.
- **Grass:** tile (0, 0) only, at the suggested 1 tile = 5 yards. Yard lines, hashes and
  boundaries are drawn in code.
- **Digit tiles: not used.** The camera scale (15 px/yard, 75 px per tile) and the deep-pass
  zoom mean tiles are never drawn at exactly 64 px, so yard numbers use the Bebas Neue TTF
  font instead, as recommended above.
- Every texture is optional: if a file is missing, the game falls back to code-drawn shapes.
  F2 switches to that plain look at any time.
