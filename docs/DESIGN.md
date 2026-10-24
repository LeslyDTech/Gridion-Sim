# Gridiron Sim: Technical Design

## 1. Goals and non-goals

**Goals (Milestone 1)**

- Simulate one play of 11-on-11 football from snap to whistle, with a user-controlled ball
  carrier and AI for the other 21 players.
- Clean separation between game logic (`gridiron_core`, no rendering dependency) and the platform
  layer (`raylib`), so the logic can be unit-tested and reused with another renderer.
- Deterministic simulation: the same inputs always produce the same play.
- Data-driven content: plays and defenses are JSON files, not code.
- Engineering discipline: no warnings at `/W4` / `-Wall -Wextra`, CI on Windows and Linux,
  RAII, no allocations in the per-step update.

**Non-goals (for now)**

- Realistic physics (mass, momentum transfer, broken tackles) or a ratings system.
- Game clock, penalties, special teams, playing defense. Downs and score are tracked by
  `Drive`; after a score or a turnover on downs the offense simply starts a new drive.
- Networked play, replays, audio, art beyond simple shapes.
- Multithreading. 22 agents cost about 0.01 ms per step, so threads would add synchronization
  complexity and nondeterminism with nothing to gain at this scale.

## 2. Architecture

```
                 +-----------------------------------------------------+
                 |                src/platform (raylib)                |
                 |                                                     |
 keyboard  --->  |  InputDevice --InputCommands-->+                    |
 gamepad         |                                |                    |
                 |  App: main loop                v                    |
                 |    poll input                 Game::step(dt) x N    |
                 |    N = FixedTimestep.advance(frame time)            |
                 |    render(alpha) -----> Renderer, FollowCamera,     |
                 |                         DebugOverlay (const reads)  |
                 +----------------------------|------------------------+
                                              | const World&
                 +----------------------------v------------------------+
                 |                src/core (pure C++17)                |
                 |                                                     |
                 |  Game ---- owns ----> World ---- owns ----> Players |
                 |   |                     |  \                Ball    |
                 |   | PlayDefinitions     |   +--> SpatialHash        |
                 |   |                     |   +--> PlayerAi (steering)|
                 |   +<-- PlayLoader <-- data/plays/*.json             |
                 |                                                     |
                 |  Leaf modules: Vec2, Route, Steering, Collision,    |
                 |  PlayerState, FixedTimestep, FieldGeometry          |
                 +-----------------------------------------------------+
```

Dependencies only point downward: platform code depends on core, and core depends only on the
standard library and nlohmann/json, which is private to `PlayLoader.cpp`. The test executable
links `gridiron_core` and never raylib, and CI builds that configuration on a machine with no
graphics packages installed.

## 3. Systems

### Fixed timestep and interpolation (`FixedTimestep`, `App`)
Each rendered frame adds its real duration to an accumulator. `advance()` returns how many whole
1/60 s steps to simulate, at most 5 per frame. After a long stall the backlog is dropped rather
than chased, which avoids the spiral of death. Every step saves `previousPosition` before it
moves anything, and the renderer draws `lerp(previous, current, alpha)`, where
`alpha = accumulator / step`. That keeps motion smooth at 30, 144 or uncapped FPS. F3 cycles the
cap to show it.

Input edges such as "Space pressed" are latched by `InputDevice` until a simulation step consumes
them, so a tap is never lost on a frame that runs zero steps and never handled twice on a frame
that runs three.

### Play data (`PlayDefinition`, `PlayLoader`)
A play is 11 `SlotDefinition`s: a slot name, role, jersey number, alignment relative to the
ball, an assignment (`route`, `block`, `rush`, `man`, `pursue`, `none`), plus a route or a man
coverage target. Positions are relative, so a play can be run from any spot on the field. The
loader validates structure and football rules (11 players, unique slots, nobody offside, roles on
the correct side) and reports errors by JSON path. Internally it uses exceptions to unwind out of
nested validation. They never cross the API, which returns a `ParseResult`.

### Players and the state machine (`Player`, `PlayerState`)
`Player` is a plain struct stored by value in `std::vector<Player>` and referenced by index, never
by pointer. Behavior is selected by a `PlayerState` enum. Every legal transition lives in the one
pure function `nextState(state, event, assignment)`. Events that don't apply are ignored, so the
World can broadcast an event such as `PassCompleted` to everyone and each player reacts according
to its own state. Snap example:

```
PreSnap --Snap--> RunningRoute | Blocking | Covering | Pursuing | Idle   (by assignment)
PreSnap --ReceivedBall--> BallCarrier --ThrewBall--> Idle
RunningRoute --TargetedByPass--> ReceivingPass --ReceivedBall--> BallCarrier --Tackled--> Tackled
Covering --PassCompleted--> Pursuing          RunningRoute/Idle --PassCompleted--> Blocking
```

### AI (`PlayerAi`, `Steering`, `Route`)
Each step runs in two phases. First every AI player computes a desired velocity from the same
snapshot of the world. Then everyone moves, with acceleration limited. Splitting the phases
removes any dependence on the order players are stored in.

| State | Behavior |
|---|---|
| RunningRoute | `followRoute()` moves the receiver exactly `speed * dt` along the waypoints, carrying leftover distance through each cut |
| Covering | Shadow a point just downfield of the receiver; break on the ball when it's thrown to them |
| Pursuing | Aim at the ball carrier's predicted intercept point. `pursue` defenders hold their zone while the QB is still behind the line |
| Blocking | Step into the nearest defender's path toward the carrier; the collision does the blocking |
| ReceivingPass | Run to the pass's landing spot |

Passes are led: the target is where the receiver will be after the ball's flight time, refined
twice. On arrival the ball is caught if the receiver is within 1.3 yd and no defender is closer.

### Collision (`SpatialHash`, `Collision`)
Players are inserted into a uniform grid (2.5 yd cells, at least one player diameter) by their
center. Each occupied cell is paired with itself and 4 of its 8 neighbors (right and the three
below), so every candidate pair comes out exactly once with no deduplication pass. Overlapping
pairs are pushed apart by half the overlap each. Contact between the ball carrier and an engaged
defender is a tackle. With 22 players this typically produces 40–90 candidate pairs instead of
231, and the F1 overlay shows both numbers.

### Play flow (`World`, `Game`)
`World` reports `PreSnap → Live → Over` together with a `PlayOutcome` (tackled, out of bounds,
touchdown, safety, incomplete, plus yards and spot). `Game` holds the result screen for 2.5 s,
then computes the next spot with the pure `nextBallSpot()`: tackled spots go between the hashes,
and touchdowns and safeties restart at the 25. It then sets up the next play in the playbook.

### Drive (`Drive`)
Down, distance, first-down line and score. `Game` reports every finished play to it. Reaching the
line to gain resets to 1st & 10, inside the opponent's 10 it becomes "& Goal", failing on 4th
down is a turnover, touchdowns score 7 (the extra point is assumed) and safeties score 2. These
are rules, so they live in core and are unit-tested, not in the HUD.

### Presentation layer (`Effects`, `Hud`, `FieldRenderer`, `Theme`)
The renderer never changes the simulation. Effects that react to game events, such as shake and
particles on a tackle or the "+N YDS" popup, are driven by `Effects::update()`, which watches the
const `World` once per rendered frame and detects transitions (for example `Live → Over` with a
`Tackled` outcome). The core needs no callbacks or event queue for presentation, and the
simulation stays deterministic. Particles come from a fixed 256-slot ring buffer, and every
effect is time-based, so it looks the same at any frame rate. F2 switches all of it off for
before/after comparisons. The camera centers the field in the area above the scorebug, so the
HUD never hides a player.

The broadcast HUD (`BroadcastHud`) is a translation of a Figma frame, not generated web code:
the Figma MCP reference code was used only as a measurement source. Each group is anchored to a
screen edge (banner top center, playbook panel top right, scorebug and controls bar bottom
center, popup screen center), so the layout holds at any window size. Figma font sizes convert to
raylib sizes x1.2 (Barlow Condensed's ascender - descender is 1.2 em), and HUD fonts are
rasterized at the exact sizes used. raylib truncates glyph advances to whole pixels, which made
lines ~10% narrower than the design; `Assets` re-derives advances from an 8x rasterization and
rounds them. The popup animation is driven by `Game::secondsSincePlayEnded()`, so it needs no
state of its own, and down & distance are measured from `Game::lineOfScrimmageX()`, the spot of
the next snap once a play has ended.

### Blocking and tackling (`World`)
Players have mass and blocking, shedding and tackling ratings per role (`attributesFor`).
Contact pushes players apart in inverse proportion to mass, so a lineman holds his ground against
a lighter player. Blockers get assignments in a "zone" scheme: every blocker/rusher pair in range
is sorted by distance and taken greedily (one blocker per rusher first, spare blockers double),
and an assignment sticks until the rusher sheds or leaves range. A defender within a short reach
of his blocker is engaged and moves at 20% speed until he sheds the block after
`blockHoldSeconds()` (base 1.4 s x blocker rating / shed rating, ~1.75 s line vs line), then slips
free for 0.8 s. Contact with the ball carrier is a tackle attempt: `tackleChance()` combines the
defender's rating, the angle (head-on beats from behind), the carrier's speed and the closing
speed, clamped to 15-95%. A miss makes the defender stumble for 0.6 s. Rolls come from a
`std::mt19937` reseeded every play from the play count, so the simulation stays deterministic and
reproducible. With no input, the shotgun pocket now holds ~3.8 s (it was ~1.2 s).

The camera (`FollowCamera`) keeps a bounding box inside the HUD-free safe area: zoom is the closer
of a 32-yard view and the zoom that fits the box, and the center follows the ball within those
limits. Before the snap the box is all 22 players (sprites are never drawn below 26 px, so the
zoomed-out formation stays readable); from the snap on it is the action (the ball, players within
8 yards of it, and a pass's landing spot and intended receiver), with edge arrows for receivers
outside the view. The limits are re-applied after smoothing each frame, so a fast player
can never slip under the scorebug while the camera catches up. Formation spacing (1-yard line
splits) lives in the play JSON, and the loader rejects formations whose players overlap.

Art (the Figma player atlas, plus Kenney Sports Pack ball and grass, CC0) is loaded by `Assets`
(RAII) and is optional: if a texture is missing, the renderer draws shapes instead. The team
colors in `Theme.h` are the sprites' Figma jersey colors, so the code-drawn elements and the art
share one palette. All 576 player tiles live in one texture; a player's tile is pure arithmetic on
(team, position, pose, direction) (`PlayerSprites.h`), and the five skin tones come from one
shader uniform instead of five copies of a 33 MB atlas.

## 4. Key trade-offs

| Decision | Alternative | Why |
|---|---|---|
| **Fixed 60 Hz timestep + interpolation** | Variable `dt` | Determinism (reproducible tests and future replays), stable steering and collision response, and behavior independent of the player's monitor. The cost is one frame of visual latency and storing previous positions. |
| **Uniform-grid spatial hash** | Brute force, quadtree, BVH | Players are near-uniform circles on a fixed rectangle, the ideal case for a grid: O(1) insert, no rebalancing, and rebuilding every step is trivial. At 22 players brute force is also cheap; the grid matters for scaling (crowds, particles) and as a pattern, and the overlay makes the saving visible. |
| **Data-driven plays (JSON)** | Plays in C++ | Designers can add plays without recompiling, plays become reviewable diffs, and the loader's validation catches errors at load time. nlohmann/json is header-only and stays private to one `.cpp`. |
| **Enum + transition function** | State pattern (class per state) | Every transition is visible in one switch, players stay copyable values, and it is trivially testable. The cost is a central switch that grows with states. Hierarchical states can come later if it grows. |
| **Index ids, not pointers** | `Player*` links | Ids survive vector reallocation and reset, are trivially serializable, and make ownership obvious. |
| **Kinematic routes** | Routes through steering | Exact, testable route geometry that doesn't depend on the timestep. The cost is that cuts have no deceleration (see M2). |
| **Single-threaded** | Job system | See non-goals: deterministic and simple at this scale. |

## 5. Performance rules

- Every per-step buffer (`m_desiredVelocities`, `m_candidatePairs`, grid cells) is reserved when
  `World` is constructed. `SpatialHash::clear()` keeps capacity, and it clears only occupied
  cells.
- `World::setup()` allocates (it copies routes). That happens once per play, not per frame.
- HUD text is formatted into stack buffers with `snprintf`.

## 6. Roadmap

Estimates assume part-time work (about 10 h/week) and include tests.

### Milestone 2: Football depth (about 3 weeks)

(Down & distance and score were pulled forward into the polish milestone.)

| Task | Estimate |
|---|---|
| Players accelerate/decelerate through route cuts (speed loss scales with cut angle) | 4 h |
| Player ratings per player in the play JSON (today they are per role, in code) | 3 h |
| Zone coverage: defenders drop to zone landmarks and match receivers entering their area | 8 h |
| Interceptions and contested catches | 4 h |
| Real handoff animation instead of a teleporting ball | 2 h |
| Play-selection screen with route diagrams | 4 h |

### Milestone 3: Tooling and production quality (about 3 weeks)

| Task | Estimate |
|---|---|
| Deterministic replays: record `InputCommands` per step, play back, and assert identical outcomes in CI | 6 h |
| Hot-reload plays when a JSON file changes on disk | 3 h |
| In-game play editor: drag waypoints and save JSON | 10 h |
| Profiling markers (Tracy) and a stress mode with 200+ agents to demonstrate the spatial hash | 5 h |
| clang-tidy and sanitizers (ASan/UBSan) in CI | 3 h |
| Defensive play-calling: several defenses chosen by down and distance | 5 h |
