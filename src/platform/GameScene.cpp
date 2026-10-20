#include "GameScene.h"

#include "Assets.h"
#include "FieldRenderer.h"
#include "Hud.h"
#include "InputDevice.h"
#include "Renderer.h"
#include "Theme.h"

#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace gridiron {
namespace {

constexpr double kSimulationHz = 60.0;
constexpr float kVisibleYardsAcross = 32.0f; // close, broadcast-style framing on the ball

// Room around each framed player that must stay out from under the HUD: half a sprite, the jersey
// number below it and the receiver badge above it.
constexpr float kPlayerFramingMargin = 1.5f; // yards

// Grows `bounds` to include `point`.
void include(Bounds& bounds, Vec2 point) {
    bounds.min = {std::min(bounds.min.x, point.x), std::min(bounds.min.y, point.y)};
    bounds.max = {std::max(bounds.max.x, point.x), std::max(bounds.max.y, point.y)};
}

constexpr Bounds kEmptyBounds{{1.0e9f, 1.0e9f}, {-1.0e9f, -1.0e9f}};

Bounds withMargin(Bounds bounds) {
    const Vec2 margin{kPlayerFramingMargin, kPlayerFramingMargin};
    return {bounds.min - margin, bounds.max + margin};
}

Vec2 drawnPosition(const Player& player, float alpha) {
    return lerp(player.previousPosition, player.position, alpha);
}

// What the camera must keep clear of the HUD.
//   Before the snap: the whole formation, so the user sees every player and route.
//   From the snap on: the action. The ball (and its carrier), everyone within kFocusRadius of
//   it, and while a pass is in the air its landing spot and the intended receiver. Players
//   outside the focus may leave the screen; receivers that do get edge arrows (Renderer).
constexpr float kFocusRadius = 8.0f; // yards

Bounds cameraBounds(const World& world, float alpha) {
    Bounds bounds = kEmptyBounds;
    if (world.phase() == PlayPhase::PreSnap) {
        for (const Player& player : world.players()) {
            include(bounds, drawnPosition(player, alpha));
        }
        return withMargin(bounds);
    }

    const Ball& ball = world.ball();
    const Vec2 ballPosition = lerp(ball.previousPosition, ball.position, alpha);
    include(bounds, ballPosition);
    if (ball.state == BallState::InAir) {
        include(bounds, ball.target);
        if (ball.intendedReceiverId != Player::kNone) {
            const auto& receiver =
                world.players()[static_cast<std::size_t>(ball.intendedReceiverId)];
            include(bounds, drawnPosition(receiver, alpha));
        }
    }
    for (const Player& player : world.players()) {
        const Vec2 position = drawnPosition(player, alpha);
        if (distanceSquared(position, ballPosition) <= kFocusRadius * kFocusRadius) {
            include(bounds, position);
        }
    }
    return withMargin(bounds);
}

} // namespace

GameScene::GameScene(AppContext& context, GameMode mode)
    : m_context(context), m_game(context.playbook, context.defense, mode),
      m_timestep(1.0 / kSimulationHz), m_camera(kVisibleYardsAcross) {
    m_camera.setSafeArea(render::hudSafeArea(m_context.polish));
    const World& world = m_game.world();
    m_camera.snapTo(world.ball().position, cameraBounds(world, 1.0f), GetScreenWidth(),
                    GetScreenHeight());
    // Keys pressed in the menu that opened this scene must not leak into the first play.
    m_context.input.consumeForStep();
}

void GameScene::update(float frameSeconds) {
    handleCameraZoom();
    runSimulationSteps(m_timestep.advance(frameSeconds));
    m_alpha = static_cast<float>(m_timestep.alpha());

    const World& world = m_game.world();
    m_effects.update(world, frameSeconds);
    m_hudAnimator.update(m_game, frameSeconds);
    updateCamera(frameSeconds);

    m_context.playerCount = static_cast<int>(world.players().size());
    m_context.frame.collisionChecks = world.collisionChecks();
}

void GameScene::handleCameraZoom() {
    const AppCommands& commands = m_context.input.appCommands();
    if (commands.zoomNotches != 0.0f) {
        // ~15% per wheel notch, multiplicative so zooming feels even at any level.
        constexpr float kZoomPerNotch = 1.15f;
        constexpr float kMinUserZoom = 0.5f;
        constexpr float kMaxUserZoom = 3.0f;
        const float zoom = m_camera.userZoom() * std::pow(kZoomPerNotch, commands.zoomNotches);
        m_camera.setUserZoom(std::clamp(zoom, kMinUserZoom, kMaxUserZoom));
    }
    if (commands.resetZoom) {
        m_camera.setUserZoom(1.0f);
    }
}

void GameScene::runSimulationSteps(int steps) {
    m_context.frame.steps = steps;
    m_context.frame.updateSeconds = 0.0;
    if (steps == 0) {
        return;
    }
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    const float dt = static_cast<float>(m_timestep.stepSeconds());
    for (int i = 0; i < steps; ++i) {
        m_game.step(m_context.input.consumeForStep(), dt);
        if (m_game.playJustStarted()) {
            const World& world = m_game.world();
            m_camera.snapTo(world.ball().position, cameraBounds(world, 1.0f), GetScreenWidth(),
                            GetScreenHeight());
        }
    }
    const std::chrono::duration<double> elapsed = Clock::now() - start;
    m_context.frame.updateSeconds = elapsed.count();
}

void GameScene::updateCamera(float frameSeconds) {
    const World& world = m_game.world();
    // Follow the ball where it is *drawn* (interpolated), so the camera never jitters against
    // the players. Reduced effects (Settings) turns off the screen shake.
    const Ball& ball = world.ball();
    const bool shake = m_context.polish && !m_context.settings.reducedEffects;
    m_camera.setSafeArea(render::hudSafeArea(m_context.polish));
    m_camera.follow(lerp(ball.previousPosition, ball.position, m_alpha),
                    cameraBounds(world, m_alpha), frameSeconds, GetScreenWidth(), GetScreenHeight(),
                    m_context.polish ? m_effects.zoomFactor() : 1.0f,
                    shake ? m_effects.shakeOffset() : Vec2{});
}

void GameScene::draw() {
    const World& world = m_game.world();
    const bool polish = m_context.polish;
    const RenderContext context{m_camera, m_context.assets, m_alpha, polish, GetTime()};

    BeginMode2D(m_camera.raylibCamera());
    render::drawField(context);
    render::drawLineOfScrimmage(world, context);
    if (world.phase() == PlayPhase::PreSnap) {
        render::drawFirstDownLine(m_game.drive().firstDownLineX(), context);
    }
    m_context.overlay.drawGrid(world.spatialHash());
    render::drawRoutes(world, context, m_effects.routeOpacity());
    render::drawPassTarget(world, context);
    if (polish) {
        m_effects.drawParticles();
    }
    render::drawPlayers(world, context, m_effects);
    render::drawBall(world, context);
    EndMode2D();

    render::drawVignette(context);
    render::drawPlayerLabels(world, context, m_context.overlay.enabled());
    render::drawHud(m_game, context, m_hudAnimator);
}

} // namespace gridiron
