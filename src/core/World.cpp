#include "World.h"

#include "FieldGeometry.h"
#include "PlayerAi.h"
#include "Route.h"
#include "Steering.h"

#include <algorithm>
#include <string>
#include <utility>

namespace gridiron {
namespace {

constexpr std::size_t kMaxPlayers = 2 * PlayDefinition::kPlayersPerSide;
constexpr float kPassSpeed = 22.0f;  // yards/second along the ground (~45 mph)
constexpr float kCatchRadius = 1.3f; // how close a player must be to the ball to catch it

// Broad-phase grid: covers the field plus a margin for players who run out of bounds. Cells must
// be at least one player diameter (largest radius is 0.65 yd); 2.5 yd keeps the debug grid legible.
constexpr Vec2 kGridMin{-10.0f, -10.0f};
constexpr Vec2 kGridMax{field::kLength + 10.0f, field::kWidth + 10.0f};
constexpr float kGridCellSize = 2.5f;
// Worst case is every player near every other: 22 * 21 / 2 pairs.
constexpr std::size_t kMaxPairs = kMaxPlayers * (kMaxPlayers - 1) / 2;

Player makePlayer(const SlotDefinition& slot, Team team, int id, Vec2 ballSpot) {
    Player player;
    player.id = id;
    player.team = team;
    player.role = slot.role;
    player.assignment = slot.assignment;
    player.jersey = slot.jersey;
    player.attributes = attributesFor(slot.role);
    player.position = ballSpot + slot.alignment;
    player.previousPosition = player.position;
    player.homePosition = player.position;

    // Route waypoints are stored relative to the player's alignment; convert to field space once.
    player.route.reserve(slot.route.size());
    for (const Vec2 waypoint : slot.route) {
        player.route.push_back(player.position + waypoint);
    }
    return player;
}

int findSlot(const PlayDefinition& play, const std::string& slotName) {
    for (std::size_t i = 0; i < play.players.size(); ++i) {
        if (play.players[i].slot == slotName) {
            return static_cast<int>(i);
        }
    }
    return Player::kNone;
}

// Lead the receiver: aim where they will be when the ball arrives. Two refinement passes are
// plenty, since flight time depends only weakly on the exact aim point.
Vec2 leadTarget(Vec2 passer, const Player& receiver) {
    Vec2 aim = receiver.position;
    for (int i = 0; i < 2; ++i) {
        const float flightTime = distance(passer, aim) / kPassSpeed;
        aim = receiver.position + receiver.velocity * flightTime;
    }
    return aim;
}

} // namespace

World::World() : m_spatialHash(kGridMin, kGridMax, kGridCellSize) {
    // Reserve every per-step buffer up front, so step() never allocates.
    m_players.reserve(kMaxPlayers);
    m_desiredVelocities.resize(kMaxPlayers);
    m_candidatePairs.reserve(kMaxPairs);
}

void World::setup(const PlayDefinition& offense, const PlayDefinition& defense, Vec2 ballSpot) {
    m_players.clear();
    m_phase = PlayPhase::PreSnap;
    m_outcome = {};
    m_lineOfScrimmageX = ballSpot.x;

    // Offense first, so an offensive player's id equals its index in offense.players.
    int receiverNumber = 0;
    for (const SlotDefinition& slot : offense.players) {
        Player player = makePlayer(slot, Team::Offense, static_cast<int>(m_players.size()), ballSpot);
        if (player.assignment == Assignment::RunRoute) {
            player.receiverNumber = receiverNumber++; // throw keys follow file order
        }
        m_players.push_back(std::move(player));
    }
    m_snapReceiverId = findSlot(offense, offense.ballCarrierSlot);

    for (const SlotDefinition& slot : defense.players) {
        Player player = makePlayer(slot, Team::Defense, static_cast<int>(m_players.size()), ballSpot);
        if (player.assignment == Assignment::ManCoverage) {
            const int target = findSlot(offense, slot.coverSlot);
            if (target == Player::kNone) {
                // This formation has no such player (e.g. no WR3): play the ball instead.
                player.assignment = Assignment::Pursue;
            } else {
                // Line up across from the man at the depth the defense file asks for.
                player.coverTargetId = target;
                player.position.y = playerById(target).position.y;
                player.previousPosition = player.homePosition = player.position;
            }
        }
        m_players.push_back(std::move(player));
    }

    m_ball = Ball{};
    m_ball.position = m_ball.previousPosition = ballSpot;
}

void World::step(const InputCommands& input, float dt) {
    // Remember where everything was, so the renderer can interpolate toward the new state.
    for (Player& player : m_players) {
        player.previousPosition = player.position;
    }
    m_ball.previousPosition = m_ball.position;

    switch (m_phase) {
    case PlayPhase::PreSnap:
        if (input.snap) {
            snap();
        }
        break;
    case PlayPhase::Live:
        if (input.throwTarget != InputCommands::kNoThrow) {
            tryThrow(input.throwTarget);
        }
        movePlayers(input, dt);
        resolveCollisions();
        if (m_phase == PlayPhase::Live) { // a tackle during collisions ends the play
            updateBall(dt);
            checkBallCarrierPosition();
        }
        break;
    case PlayPhase::Over:
        movePlayers(InputCommands{}, dt); // everyone coasts to a stop
        resolveCollisions();
        break;
    }
}

void World::resolveCollisions() {
    // Broad phase: rebuild the grid from scratch. 22 inserts is cheaper than tracking movement.
    m_spatialHash.clear();
    for (const Player& player : m_players) {
        m_spatialHash.insert(player.id, player.position);
    }
    m_spatialHash.findCandidatePairs(m_candidatePairs);
    m_collisionChecks = static_cast<int>(m_candidatePairs.size());

    // Narrow phase: exact circle test on each candidate pair.
    for (const auto& [idA, idB] : m_candidatePairs) {
        Player& a = playerById(idA);
        Player& b = playerById(idB);
        const std::optional<Contact> contact =
            findContact({a.position, a.attributes.radius}, {b.position, b.attributes.radius});
        if (!contact) {
            continue;
        }
        separate(a, b, *contact);
        if (m_phase == PlayPhase::Live && (isTackle(a, b) || isTackle(b, a))) {
            Player& carrier = a.state == PlayerState::BallCarrier ? a : b;
            carrier.handle(PlayerEvent::Tackled);
            endPlay(PlayResult::Tackled, carrier.position);
        }
    }
}

void World::checkBallCarrierPosition() {
    if (m_ball.state != BallState::Held) {
        return;
    }
    const Vec2 position = playerById(m_ball.carrierId).position;
    if (field::isInOpponentEndZone(position) && field::isInBounds(position)) {
        endPlay(PlayResult::Touchdown, {field::kOpponentGoalLine, position.y});
    } else if (!field::isInBounds(position)) {
        // The ball is dead where the carrier crossed the sideline.
        const Vec2 crossing{std::clamp(position.x, 0.0f, field::kLength),
                            std::clamp(position.y, 0.0f, field::kWidth)};
        endPlay(PlayResult::OutOfBounds, crossing);
    }
}

bool World::isTackle(const Player& carrier, const Player& defender) {
    return carrier.state == PlayerState::BallCarrier && defender.team == Team::Defense &&
           (defender.state == PlayerState::Pursuing || defender.state == PlayerState::Covering);
}

void World::separate(Player& a, Player& b, const Contact& contact) {
    // Push both players apart by half the overlap each, so neither has priority. A single pass
    // can leave small overlaps in a pile-up; they are resolved over the next few steps, which
    // reads as players jostling.
    const Vec2 push = contact.normal * (contact.penetration * 0.5f);
    a.position -= push;
    b.position += push;
}

void World::snap() {
    m_phase = PlayPhase::Live;
    for (Player& player : m_players) {
        player.handle(player.id == m_snapReceiverId ? PlayerEvent::ReceivedBall : PlayerEvent::Snap);
    }
    // Simplification: the ball goes straight to the snap receiver (QB, or HB on a handoff).
    m_ball.state = BallState::Held;
    m_ball.carrierId = m_snapReceiverId;
}

void World::tryThrow(int receiverNumber) {
    if (m_ball.state != BallState::Held) {
        return;
    }
    const Player& passer = playerById(m_ball.carrierId);
    // Only the quarterback passes, and only from behind the line of scrimmage.
    if (passer.role != Role::QB || passer.position.x > m_lineOfScrimmageX) {
        return;
    }
    for (Player& receiver : m_players) {
        const bool eligible = receiver.receiverNumber == receiverNumber &&
                              (receiver.state == PlayerState::RunningRoute ||
                               receiver.state == PlayerState::Idle);
        if (eligible) {
            throwBall(m_ball, passer.position, leadTarget(passer.position, receiver), kPassSpeed,
                      receiver.id);
            playerById(passer.id).handle(PlayerEvent::ThrewBall);
            receiver.handle(PlayerEvent::TargetedByPass);
            return;
        }
    }
}

void World::movePlayers(const InputCommands& input, float dt) {
    // Phase 1: every AI decides based on the same snapshot of the world. Deciding and moving in
    // one loop would let player 5 react to where player 4 is *after* moving this step, making
    // results depend on vector order.
    const ai::Context context{m_players, m_ball, m_lineOfScrimmageX};
    for (std::size_t i = 0; i < m_players.size(); ++i) {
        const Player& player = m_players[i];
        if (m_phase == PlayPhase::Over) {
            m_desiredVelocities[i] = {};
        } else if (player.state == PlayerState::BallCarrier) {
            m_desiredVelocities[i] = input.move * player.attributes.maxSpeed;
        } else {
            m_desiredVelocities[i] = ai::desiredVelocity(player, context);
        }
    }

    // Phase 2: move everyone.
    for (std::size_t i = 0; i < m_players.size(); ++i) {
        Player& player = m_players[i];
        if (player.state == PlayerState::RunningRoute && m_phase == PlayPhase::Live) {
            runRoute(player, dt);
            continue;
        }
        player.velocity = steering::accelerateToward(player.velocity, m_desiredVelocities[i],
                                                     player.attributes.acceleration, dt);
        player.position += player.velocity * dt;
    }
}

void World::runRoute(Player& player, float dt) {
    const RouteStep step = followRoute(player.route, player.nextWaypoint, player.position,
                                       player.attributes.maxSpeed * dt);
    player.velocity = (step.position - player.position) / dt;
    player.position = step.position;
    player.nextWaypoint = step.nextWaypoint;
    if (step.finished) {
        player.handle(PlayerEvent::RouteFinished);
    }
}

void World::updateBall(float dt) {
    switch (m_ball.state) {
    case BallState::Held:
        m_ball.position = playerById(m_ball.carrierId).position;
        break;
    case BallState::InAir:
        if (advanceFlight(m_ball, dt)) {
            resolvePassArrival();
        }
        break;
    case BallState::Spotted:
    case BallState::Dead:
        break;
    }
}

void World::resolvePassArrival() {
    Player& receiver = playerById(m_ball.intendedReceiverId);
    const float receiverDistance = distance(receiver.position, m_ball.target);

    // The closest defender gets a chance to knock the ball away if they're nearer than the
    // receiver. (Interceptions are a Milestone 2 feature.)
    float closestDefender = kCatchRadius;
    for (const Player& player : m_players) {
        if (player.team == Team::Defense) {
            closestDefender = std::min(closestDefender, distance(player.position, m_ball.target));
        }
    }

    const bool caught = receiverDistance <= kCatchRadius && receiverDistance <= closestDefender;
    if (!caught) {
        endPlay(PlayResult::Incomplete, {m_lineOfScrimmageX, m_ball.target.y});
        return;
    }
    m_ball.state = BallState::Held;
    m_ball.carrierId = receiver.id;
    receiver.handle(PlayerEvent::ReceivedBall);
    broadcast(PlayerEvent::PassCompleted, receiver.id);
}

void World::endPlay(PlayResult result, Vec2 spot) {
    // Downed (or out of bounds) in your own end zone is a safety.
    if ((result == PlayResult::Tackled || result == PlayResult::OutOfBounds) &&
        field::isInOwnEndZone(spot)) {
        result = PlayResult::Safety;
    }
    m_phase = PlayPhase::Over;
    m_ball.state = BallState::Dead;
    if (result != PlayResult::Incomplete) {
        m_ball.position = spot; // an incomplete pass stays where it fell
    }
    m_outcome.result = result;
    m_outcome.spot = spot;
    m_outcome.yardsGained = result == PlayResult::Incomplete ? 0.0f : spot.x - m_lineOfScrimmageX;
}

void World::broadcast(PlayerEvent event, int exceptId) {
    for (Player& player : m_players) {
        if (player.id != exceptId) {
            player.handle(event);
        }
    }
}

} // namespace gridiron
