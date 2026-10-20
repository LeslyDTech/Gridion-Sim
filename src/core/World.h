#pragma once

#include "Ball.h"
#include "Collision.h"
#include "InputCommands.h"
#include "PlayDefinition.h"
#include "Player.h"
#include "SpatialHash.h"

#include <cstdint>
#include <vector>

namespace gridiron {

enum class PlayPhase : std::uint8_t { PreSnap, Live, Over };

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
    void setup(const PlayDefinition& offense, const PlayDefinition& defense, Vec2 ballSpot);

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
    void movePlayers(const InputCommands& input, float dt);
    void runRoute(Player& player, float dt);
    void updateBall(float dt);
    void resolveCollisions();
    void separate(Player& a, Player& b, const Contact& contact);
    void checkBallCarrierPosition(); // touchdown / out of bounds
    static bool isTackle(const Player& carrier, const Player& defender);
    void resolvePassArrival();
    void endPlay(PlayResult result, Vec2 spot);
    void broadcast(PlayerEvent event, int exceptId);

    Player& playerById(int id) { return m_players[static_cast<std::size_t>(id)]; }
    const Player& playerById(int id) const { return m_players[static_cast<std::size_t>(id)]; }

    std::vector<Player> m_players;
    std::vector<Vec2> m_desiredVelocities; // reused every step, one per player
    SpatialHash m_spatialHash;
    std::vector<SpatialHash::Pair> m_candidatePairs; // reused every step
    int m_collisionChecks = 0;
    Ball m_ball;
    PlayPhase m_phase = PlayPhase::PreSnap;
    PlayOutcome m_outcome;
    float m_lineOfScrimmageX = 0.0f;
    int m_snapReceiverId = Player::kNone; // who gets the ball at the snap
};

} // namespace gridiron
