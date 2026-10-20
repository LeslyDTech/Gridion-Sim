#pragma once

#include "Ball.h"
#include "Collision.h"
#include "InputCommands.h"
#include "PlayDefinition.h"
#include "Player.h"
#include "SpatialHash.h"

#include <cstdint>
#include <random>
#include <vector>

namespace gridiron {

enum class PlayPhase : std::uint8_t { PreSnap, Live, Over };

// Blocking: how long a block holds a defender before he sheds it. Stronger blockers hold longer,
// better pass rushers get off sooner: an offensive lineman holds a defensive lineman ~1.75 s.
float blockHoldSeconds(const PlayerAttributes& blocker, const PlayerAttributes& defender);

// Tackling: the chance that contact brings the ball carrier down, from the defender's rating, the
// angle (head-on beats chasing from behind), the carrier's speed (harder to stop at full speed)
// and the defender's closing speed. Clamped to [0.15, 0.95]; pure, so it can be tested directly.
//   toDefender: unit vector from the ball carrier to the defender.
float tackleChance(const PlayerAttributes& defender, Vec2 defenderVelocity, Vec2 carrierVelocity,
                   Vec2 toDefender);

enum class PlayResult : std::uint8_t {
    None,
    Tackled,
    OutOfBounds,
    Touchdown,
    Safety,
    Incomplete,
};

struct PlayOutcome {
    PlayResult result = PlayResult::None;
    float yardsGained = 0.0f;
    Vec2 spot; // where the ball is dead
};

// The simulation of a single play: 22 players and a ball, advanced in fixed steps.
//
// World owns no rendering and no input devices; it is driven by InputCommands and read through
// const accessors, so it can run headless in tests. Players are referenced by index (Player::id).
class World {
public:
    World();

    // Lines up both teams for a new play with the ball at `ballSpot`. Allocates (copies routes);
    // this is setup time, not the per-frame path.
    // `seed` makes broken tackles reproducible: the same seed and inputs give the same play.
    void setup(const PlayDefinition& offense, const PlayDefinition& defense, Vec2 ballSpot,
               unsigned seed = 1);

    // Advances the simulation by one fixed step.
    void step(const InputCommands& input, float dt);

    PlayPhase phase() const { return m_phase; }
    const PlayOutcome& outcome() const { return m_outcome; }
    const std::vector<Player>& players() const { return m_players; }
    const Ball& ball() const { return m_ball; }
    float lineOfScrimmageX() const { return m_lineOfScrimmageX; }

    // Debug view: the broad-phase grid and how many narrow-phase circle tests the last step ran.
    const SpatialHash& spatialHash() const { return m_spatialHash; }
    int collisionChecks() const { return m_collisionChecks; }

private:
    void snap();
    void tryThrow(int receiverNumber);
    void assignBlocks();
    void movePlayers(const InputCommands& input, float dt);
    void runRoute(Player& player, float dt);
    void updateBall(float dt);
    void resolveCollisions(float dt);
    void separate(Player& a, Player& b, const Contact& contact);
    void recordBlockContact(const Player& a, const Player& b);
    void updateBlockEngagement(float dt);
    void checkBallCarrierPosition(); // touchdown / out of bounds
    static bool canAttemptTackle(const Player& carrier, const Player& defender);
    void attemptTackle(Player& carrier, Player& defender);
    void updateTackleTimers(float dt);
    void resolvePassArrival();
    void endPlay(PlayResult result, Vec2 spot);
    void broadcast(PlayerEvent event, int exceptId);

    Player& playerById(int id) { return m_players[static_cast<std::size_t>(id)]; }
    const Player& playerById(int id) const { return m_players[static_cast<std::size_t>(id)]; }

    std::vector<Player> m_players;
    std::vector<Vec2> m_desiredVelocities; // reused every step, one per player
    struct BlockPair {
        float distanceSquared;
        int blocker;
        int defender;
    };
    std::vector<BlockPair> m_blockPairs; // reused every step by assignBlocks
    std::vector<float> m_blockThisStep;  // per defender: strongest blocker in contact (0 = none)
    SpatialHash m_spatialHash;
    std::vector<SpatialHash::Pair> m_candidatePairs; // reused every step
    int m_collisionChecks = 0;
    std::mt19937 m_random{1}; // reseeded every play; drives tackle rolls only
    Ball m_ball;
    PlayPhase m_phase = PlayPhase::PreSnap;
    PlayOutcome m_outcome;
    float m_lineOfScrimmageX = 0.0f;
    int m_snapReceiverId = Player::kNone; // who gets the ball at the snap
};

} // namespace gridiron
