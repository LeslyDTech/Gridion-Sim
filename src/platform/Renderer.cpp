#include "Renderer.h"

#include "FollowCamera.h"
#include "Theme.h"
#include "core/FieldGeometry.h"
#include "core/World.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron::render {
namespace {

Vector2 toRaylib(Vec2 v) { return Vector2{v.x, v.y}; }

Vec2 interpolated(const Player& player, float alpha) {
    return lerp(player.previousPosition, player.position, alpha);
}

Color teamColor(Team team) { return team == Team::Offense ? theme::kHome : theme::kAway; }

// Text centered on a screen point.
void drawCenteredText(const char* text, Vector2 center, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, static_cast<int>(center.x) - width / 2,
             static_cast<int>(center.y) - fontSize / 2, fontSize, color);
}

} // namespace

// ================================================================ world space

void drawLineOfScrimmage(const World& world, const RenderContext& /*context*/) {
    const float x = world.lineOfScrimmageX();
    DrawLineEx(Vector2{x, 0.0f}, Vector2{x, field::kWidth}, 0.2f,
               Fade(ColorBrightness(theme::kHome, 0.5f), 0.9f));
}

void drawRoutes(const World& world, const RenderContext& /*context*/) {
    if (world.phase() != PlayPhase::PreSnap) {
        return;
    }
    const Color color = Fade(theme::kAccent, 0.6f);
    for (const Player& player : world.players()) {
        Vec2 from = player.position;
        for (const Vec2 waypoint : player.route) {
            DrawLineEx(toRaylib(from), toRaylib(waypoint), 0.18f, color);
            from = waypoint;
        }
        if (!player.route.empty()) {
            DrawCircleV(toRaylib(player.route.back()), 0.3f, color);
        }
    }
}

void drawPlayers(const World& world, const RenderContext& context) {
    for (const Player& player : world.players()) {
        const Vector2 center = toRaylib(interpolated(player, context.alpha));
        const float radius = player.attributes.radius;

        if (player.state == PlayerState::BallCarrier) {
            DrawCircleV(center, radius + 0.3f, theme::kAccent);
        }
        DrawCircleV(center, radius, teamColor(player.team));
        DrawCircleLinesV(center, radius, theme::kLineWhite);
        if (player.state == PlayerState::Tackled) {
            DrawCircleV(center, radius * 0.4f, theme::kAccent);
        }
    }
}

void drawBall(const World& world, const RenderContext& context) {
    const Ball& ball = world.ball();
    const Vec2 ground = lerp(ball.previousPosition, ball.position, context.alpha);
    if (ball.state == BallState::InAir) {
        // Shadow stays on the ground; the ball itself is lifted by its flight height.
        DrawCircleV(toRaylib(ground), 0.25f, theme::shadow());
        const Vec2 lifted = ground - Vec2{0.0f, ball.height()};
        DrawCircleV(toRaylib(lifted), 0.3f, theme::kUiDark);
        DrawCircleLinesV(toRaylib(lifted), 0.3f, theme::kLineWhite);
    } else if (ball.state == BallState::Spotted || ball.state == BallState::Dead) {
        DrawCircleV(toRaylib(ground), 0.3f, theme::kUiDark);
        DrawCircleLinesV(toRaylib(ground), 0.3f, theme::kLineWhite);
    }
    // While Held, the gold carrier ring shows who has it.
}

// ================================================================ screen space

void drawPlayerLabels(const World& world, const RenderContext& context, bool showStates) {
    const float pixelsPerYard = context.camera.pixelsPerYard();
    const bool canThrow =
        world.ball().state == BallState::Held || world.phase() == PlayPhase::PreSnap;
    char buffer[16];

    for (const Player& player : world.players()) {
        const Vector2 center = context.camera.worldToScreen(interpolated(player, context.alpha));

        // Jersey number, sized to fit inside the circle.
        const int fontSize = static_cast<int>(player.attributes.radius * pixelsPerYard * 1.1f);
        std::snprintf(buffer, sizeof(buffer), "%d", player.jersey);
        drawCenteredText(buffer, center, fontSize, theme::kLineWhite);

        // Throw-button badge above each eligible receiver.
        if (player.receiverNumber != Player::kNone && canThrow &&
            (player.state == PlayerState::PreSnap || player.state == PlayerState::RunningRoute ||
             player.state == PlayerState::Idle)) {
            const Vector2 badge{center.x,
                                center.y - (player.attributes.radius + 0.7f) * pixelsPerYard};
            DrawCircleV(badge, 9.0f, theme::panel(0.85f));
            std::snprintf(buffer, sizeof(buffer), "%d", player.receiverNumber + 1);
            drawCenteredText(buffer, badge, 14, theme::kAccent);
        }

        if (showStates) {
            const Vector2 below{center.x,
                                center.y + (player.attributes.radius + 0.5f) * pixelsPerYard};
            drawCenteredText(toString(player.state), below, 12, theme::kLineWhite);
        }
    }
}

} // namespace gridiron::render
