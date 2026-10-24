# Asset notes

## Players — `sprites/players.png` (Figma)
- Exported from the Figma frame "PlayerSprite / Transparent Export Atlas / 576 Base Tiles"
  (file bsPx98QvHlBZm4zPhRb66P, page node 11:3), 1920 x 4320 px, transparent background.
- 576 tiles: 2 teams (Comets, Blaze) x 9 positions (QB RB WR TE OL DL LB DB K) x 8 poses
  (idle run block tackle catch throw fall kick) x 4 facing directions (down up left right).
- Each tile is 96 x 96 px on a 120 px pitch, 16 tiles per row, in exactly that order. The movement
  anchor is the tile center (48, 48). Directly overhead, no drop shadows.
- Exported with Skin/Medium, no ball in hand, and the design's default jersey number per position.

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

- **Players:** Comets = offense, Blaze = defense. The pose comes from the simulation state (see
  src/platform/PlayerSprites.h). The nearest of the four facing tiles is drawn, rotated by the
  remaining angle (at most 45 degrees), so turning is smooth. Skin tone = jersey number % 5,
  so a player looks the same every play: a shader swaps the exported Skin/Medium for one of the
  five Figma skin tones. Each sprite is scaled so its shoulders are 2.08 x the collision radius,
  so what you see matches the physics. Trilinear filtering with mipmaps keeps the rotated,
  scaled-down tiles smooth; the 24 px gaps between tiles keep mip levels from bleeding.
- **Ball:** the sprite's long axis is at ~55 degrees, so that angle is subtracted when it is
  rotated along the flight path.
- **Grass:** tile (0, 0) only, at the suggested 1 tile = 5 yards. Yard lines, hashes and
  boundaries are drawn in code.
- **Digit tiles: not used.** The camera scale (15 px/yard, 75 px per tile) and the deep-pass
  zoom mean tiles are never drawn at exactly 64 px, so yard numbers use the Bebas Neue TTF
  font instead, as recommended above.
- Every texture is optional: if a file is missing, the game falls back to code-drawn shapes.
  F2 switches to that plain look at any time.
