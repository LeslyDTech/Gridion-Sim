#include "AttractMode.h"

#include "Assets.h"
#include "RenderContext.h"
#include "Renderer.h"

#include <raylib.h>

#include <cmath>
#include <utility>

namespace gridiron {
namespace {

constexpr double kSimulationHz = 60.0;
constexpr float kVisibleYardsAcross = 40.0f; // a little wider than the game camera
constexpr float kCameraSharpness = 1.2f;     // lazy pan: the camera drifts after the ball
constexpr float kFramingRadius = 10.0f;      // keep players this close to the ball in view

// Autopilot timing, in seconds within each phase.
constexpr float kSelectPlayAt = 0.8f;
constexpr float kSnapAt = 1.6f;
constexpr float kThrowAt = 1.4f; // after the snap; the pocket holds ~3 s

Bounds framing(const World& world, Vec2 focus) {
    Bounds bounds{focus, focus};
    for (const Player& player : world.players()) {
        if (distanceSquared(player.position, focus) <= kFramingRadius * kFramingRadius) {
            bounds.min = {std::min(bounds.min.x, player.position.x),
                          std::min(bounds.min.y, player.position.y)};
            bounds.max = {std::max(bounds.max.x, player.position.x),
                          std::max(bounds.max.y, player.position.y)};
        }
    }
    return bounds;
}

} // namespace

AttractMode::AttractMode(std::vector<PlayDefinition> playbook, PlayDefinition defense)
    : m_game(std::move(playbook), std::move(defense), GameMode::Practice),
      m_timestep(1.0 / kSimulationHz), m_camera(kVisibleYardsAcross) {
    m_camera.setFollowSharpness(kCameraSharpness);
    const World& world = m_game.world();
    m_camera.snapTo(world.ball().position, framing(world, world.ball().position), GetScreenWidth(),
                    GetScreenHeight());
}

void AttractMode::update(float frameSeconds) {
    m_time += frameSeconds;
    const float dt = static_cast<float>(m_timestep.stepSeconds());
    const int steps = m_timestep.advance(frameSeconds);
    for (int i = 0; i < steps; ++i) {
        m_game.step(autopilot(dt), dt);
    }
    m_alpha = static_cast<float>(m_timestep.alpha());

    const World& world = m_game.world();
    m_effects.update(world, frameSeconds);

    // Follow the ball with a slow sideways drift, so the background never sits perfectly still.
    const Ball& ball = world.ball();
    const Vec2 ballPosition = lerp(ball.previousPosition, ball.position, m_alpha);
    const Vec2 drift{static_cast<float>(std::sin(m_time * 0.11)) * 5.0f,
                     static_cast<float>(std::sin(m_time * 0.07)) * 3.0f};
    m_camera.follow(ballPosition + drift, framing(world, ballPosition), frameSeconds,
                    GetScreenWidth(), GetScreenHeight());
}

InputCommands AttractMode::autopilot(float dt) {
    const World& world = m_game.world();
    if (world.phase() != m_lastPhase) {
        m_lastPhase = world.phase();
        m_phaseSeconds = 0.0f;
    }
    const float before = m_phaseSeconds;
    m_phaseSeconds += dt;
    auto crossed = [&](float at) { return before < at && m_phaseSeconds >= at; };

    InputCommands input;
    switch (world.phase()) {
    case PlayPhase::PreSnap:
        if (crossed(kSelectPlayAt)) {
            // A different play every snap.
            input.selectPlay = static_cast<int>(m_nextPlay % m_game.playbook().size());
            ++m_nextPlay;
        } else if (crossed(kSnapAt)) {
            input.snap = true;
        }
        break;
    case PlayPhase::Live: {
        const Ball& ball = world.ball();
        if (ball.state != BallState::Held) {
            break;
        }
        const Player& carrier = world.players()[static_cast<std::size_t>(ball.carrierId)];
        if (carrier.role == Role::QB) {
            if (m_phaseSeconds >= kThrowAt) {
                input.throwTarget = mostOpenReceiver();
            }
            // Drift back a little in the pocket, like a real drop.
            input.move = {-0.25f, 0.0f};
        } else {
            input.move = runDirection(carrier);
        }
        break;
    }
    case PlayPhase::Over:
        break; // Game moves on to the next play by itself
    }
    return input;
}

// Downfield, bending away from the nearest defender.
Vec2 AttractMode::runDirection(const Player& carrier) const {
    const Player* nearest = nullptr;
    float best = 1.0e9f;
    for (const Player& player : m_game.world().players()) {
        if (player.team != Team::Defense) {
            continue;
        }
        const float d = distanceSquared(player.position, carrier.position);
        if (d < best) {
            best = d;
            nearest = &player;
        }
    }
    Vec2 direction{1.0f, 0.0f};
    if (nearest != nullptr && best < 6.0f * 6.0f) {
        direction.y = carrier.position.y < nearest->position.y ? -0.6f : 0.6f;
    }
    return normalized(direction);
}

// The eligible receiver whose nearest defender is farthest away.
int AttractMode::mostOpenReceiver() const {
    const auto& players = m_game.world().players();
    int bestNumber = InputCommands::kNoThrow;
    float bestSeparation = -1.0f;
    for (const Player& receiver : players) {
        if (receiver.receiverNumber == Player::kNone ||
            (receiver.state != PlayerState::RunningRoute && receiver.state != PlayerState::Idle)) {
            continue;
        }
        float separation = 1.0e9f;
        for (const Player& defender : players) {
            if (defender.team == Team::Defense) {
                separation =
                    std::min(separation, distanceSquared(receiver.position, defender.position));
            }
        }
        if (separation > bestSeparation) {
            bestSeparation = separation;
            bestNumber = receiver.receiverNumber;
        }
    }
    return bestNumber;
}

void AttractMode::draw(const Assets& assets) {
    const RenderContext context{m_camera, assets, m_alpha, true, GetTime()};
    render::drawWorldLayers(m_game, context, m_effects, nullptr);
}

} // namespace gridiron
