#include "Renderer.h"

#include "Assets.h"
#include "Effects.h"
#include "FollowCamera.h"
#include "Theme.h"
#include "core/FieldGeometry.h"
#include "core/World.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gridiron::render {
namespace {

Vector2 toRaylib(Vec2 v) { return Vector2{v.x, v.y}; }

Vec2 interpolated(const Player& player, float alpha) {
    return lerp(player.previousPosition, player.position, alpha);
}

Color teamColor(Team team) { return team == Team::Offense ? theme::kHome : theme::kAway; }

// A small white triangle on the edge of the circle, pointing where the player faces.
void drawFacingIndicator(Vector2 center, float radius, Vec2 facing) {
    const Vec2 side{-facing.y, facing.x};
    const Vec2 c{center.x, center.y};
    const Vec2 tip = c + facing * (radius + 0.22f);
    const Vec2 left = c + facing * (radius - 0.04f) + side * 0.2f;
    const Vec2 right = c + facing * (radius - 0.04f) - side * 0.2f;
    // raylib only fills counter-clockwise triangles (as seen on screen); the screen's y axis
    // points down, so (tip, right, left) is the counter-clockwise order.
    DrawTriangle(toRaylib(tip), toRaylib(right), toRaylib(left), theme::kLineWhite);
}

// --- Sprite art (Kenney Sports Pack) ---------------------------------------------------------

// Player sprites are 21 x 31 px and face +x; the 31 px axis is the shoulders. The shoulders are
// drawn 2.6x the collision radius wide, a little wider than the collision circle (as real
// shoulders are) without hiding how close players actually are.
constexpr float kSpriteShoulderToRadius = 2.6f;
// The ball sprite's long axis is drawn at ~55 degrees (measured from its pixels), so that angle
// is subtracted to line the ball up with its direction of travel.
constexpr float kBallSpriteAxisDegrees = 54.7f;
constexpr float kBallYardsPerPixel = 0.05f; // 14 x 16 px -> 0.7 x 0.8 yards

float degrees(Vec2 direction) { return std::atan2(direction.y, direction.x) * RAD2DEG; }

// Draws `texture` centered on `center`, `yardsPerPixel` scale, rotated clockwise by `angle`.
void drawSprite(const Texture2D& texture, Vector2 center, float yardsPerPixel, float angle,
                Color tint) {
    const float width = static_cast<float>(texture.width) * yardsPerPixel;
    const float height = static_cast<float>(texture.height) * yardsPerPixel;
    DrawTexturePro(texture,
                   Rectangle{0.0f, 0.0f, static_cast<float>(texture.width),
                             static_cast<float>(texture.height)},
                   Rectangle{center.x, center.y, width, height},
                   Vector2{width / 2.0f, height / 2.0f}, angle, tint);
}

// Skin tone is picked from the jersey number, so a player looks the same every play.
const Texture2D* spriteFor(const Player& player, const Assets& assets) {
    return assets.playerSprite(player.team, player.jersey);
}

void drawCarrierRing(Vector2 center, float radius, double time) {
    const float pulse = 0.06f * static_cast<float>(std::sin(time * 6.0));
    DrawRing(center, radius + 0.12f, radius + 0.34f + pulse, 0.0f, 360.0f, 32,
             Fade(theme::kAccent, 0.9f));
}

void drawSpritePlayer(const Player& player, const Texture2D& sprite, Vector2 center, Vec2 facing,
                      double time) {
    const float radius = player.attributes.radius;
    const float yardsPerPixel =
        kSpriteShoulderToRadius * radius / static_cast<float>(sprite.height);

    // Soft drop shadow, offset down-right as if lit from the top-left.
    DrawCircleV(Vector2{center.x + 0.12f, center.y + 0.2f}, radius * 1.1f, theme::shadow(0.4f));
    if (player.state == PlayerState::BallCarrier) {
        drawCarrierRing(center, radius, time);
    }
    // A tackled player is drawn darker. (WHITE = the sprite's own colors, untinted.)
    const Color tint =
        player.state == PlayerState::Tackled ? ColorBrightness(WHITE, -0.35f) : WHITE;
    drawSprite(sprite, center, yardsPerPixel, degrees(facing), tint);
}

// --- Code-drawn shapes (fallback when the art is missing) -------------------------------------

void drawPolishedPlayer(const Player& player, Vector2 center, Vec2 facing, double time) {
    const float radius = player.attributes.radius;
    constexpr int kSegments = 32;

    // Soft drop shadow, offset down-right as if lit from the top-left.
    DrawCircleV(Vector2{center.x + 0.12f, center.y + 0.2f}, radius * 1.05f, theme::shadow(0.4f));

    if (player.state == PlayerState::BallCarrier) {
        const float pulse = 0.06f * static_cast<float>(std::sin(time * 6.0));
        DrawRing(center, radius + 0.12f, radius + 0.34f + pulse, 0.0f, 360.0f, kSegments,
                 Fade(theme::kAccent, 0.9f));
    }

    DrawCircleSector(center, radius, 0.0f, 360.0f, kSegments, teamColor(player.team));
    DrawRing(center, radius - 0.09f, radius, 0.0f, 360.0f, kSegments, theme::kLineWhite);
    drawFacingIndicator(center, radius, facing);

    if (player.state == PlayerState::Tackled) {
        DrawRing(center, radius + 0.1f, radius + 0.2f, 0.0f, 360.0f, kSegments, theme::kAccent);
    }
}

void drawPlainPlayer(const Player& player, Vector2 center) {
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

// Playbook-style route: straight legs with every cut rounded off by a short quadratic Bezier,
// finished with an arrowhead. (A Catmull-Rom spline through the waypoints looks smoother but
// overshoots at sharp cuts, which misrepresents the route.)
void drawCurvedRoute(const Player& player, Color color) {
    constexpr float kThickness = 0.2f;
    constexpr float kMaxCornerRadius = 1.5f; // yards

    Vec2 legStart = player.homePosition;
    Vec2 corner = player.homePosition;
    for (std::size_t i = 0; i < player.route.size(); ++i) {
        const Vec2 previous = i == 0 ? player.homePosition : player.route[i - 1];
        corner = player.route[i];
        if (i + 1 == player.route.size()) {
            break; // last leg is drawn after the loop
        }
        const Vec2 next = player.route[i + 1];
        // Round the cut, but never use more than 40% of either adjoining leg.
        const float radius = std::min(
            {kMaxCornerRadius, 0.4f * distance(previous, corner), 0.4f * distance(corner, next)});
        const Vec2 entry = corner - normalized(corner - previous) * radius;
        const Vec2 exit = corner + normalized(next - corner) * radius;
        DrawLineEx(toRaylib(legStart), toRaylib(entry), kThickness, color);
        DrawSplineSegmentBezierQuadratic(toRaylib(entry), toRaylib(corner), toRaylib(exit),
                                         kThickness, color);
        legStart = exit;
    }
    const Vec2 end = player.route.back();
    DrawLineEx(toRaylib(legStart), toRaylib(end), kThickness, color);

    // Arrowhead pointing along the final leg.
    const Vec2 dir = normalized(end - legStart);
    const Vec2 side{-dir.y, dir.x};
    const Vec2 tip = end + dir * 0.7f;
    const Vec2 left = end - dir * 0.1f + side * 0.5f;
    const Vec2 right = end - dir * 0.1f - side * 0.5f;
    DrawTriangle(toRaylib(tip), toRaylib(right), toRaylib(left), color);
}

void drawStraightRoute(const Player& player, Color color) {
    Vec2 from = player.homePosition;
    for (const Vec2 waypoint : player.route) {
        DrawLineEx(toRaylib(from), toRaylib(waypoint), 0.18f, color);
        from = waypoint;
    }
    DrawCircleV(toRaylib(player.route.back()), 0.3f, color);
}

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

void drawFirstDownLine(float x, const RenderContext& context) {
    if (context.polish) {
        // Broadcast-style: a soft glow under a solid gold line.
        DrawRectangleRec(Rectangle{x - 0.35f, 0.0f, 0.7f, field::kWidth},
                         Fade(theme::kAccent, 0.2f));
    }
    DrawLineEx(Vector2{x, 0.0f}, Vector2{x, field::kWidth}, 0.22f, Fade(theme::kAccent, 0.9f));
}

void drawRoutes(const World& world, const RenderContext& context, float opacity) {
    // Plain mode: routes simply disappear at the snap.
    if (!context.polish && world.phase() != PlayPhase::PreSnap) {
        return;
    }
    if (opacity <= 0.0f) {
        return;
    }
    const Color color = Fade(theme::kAccent, 0.7f * opacity);
    for (const Player& player : world.players()) {
        if (player.route.empty()) {
            continue;
        }
        if (context.polish) {
            drawCurvedRoute(player, color);
        } else {
            drawStraightRoute(player, color);
        }
    }
}

void drawPlayers(const World& world, const RenderContext& context, const Effects& effects) {
    for (const Player& player : world.players()) {
        const Vector2 center = toRaylib(interpolated(player, context.alpha));
        const Texture2D* sprite = spriteFor(player, context.assets);
        if (context.polish && sprite != nullptr) {
            drawSpritePlayer(player, *sprite, center, effects.facing(player.id), context.time);
        } else if (context.polish) {
            drawPolishedPlayer(player, center, effects.facing(player.id), context.time);
        } else {
            drawPlainPlayer(player, center);
        }
    }
}

void drawBall(const World& world, const RenderContext& context) {
    const Ball& ball = world.ball();
    const Vec2 ground = lerp(ball.previousPosition, ball.position, context.alpha);
    if (ball.state == BallState::Held) {
        return; // the gold carrier ring shows who has it
    }
    const Texture2D* sprite = context.polish ? context.assets.ballSprite() : nullptr;
    if (ball.state != BallState::InAir) {
        if (sprite != nullptr) { // resting on the ground, pointing downfield
            drawSprite(*sprite, toRaylib(ground), kBallYardsPerPixel, -kBallSpriteAxisDegrees,
                       WHITE);
            return;
        }
        DrawCircleV(toRaylib(ground), 0.3f, theme::kUiDark);
        DrawCircleLinesV(toRaylib(ground), 0.3f, theme::kLineWhite);
        return;
    }

    // In the air the ball is drawn lifted by its arc height, above a shadow on the ground.
    const float height = ball.height();
    const Vec2 lifted = ground - Vec2{0.0f, height};
    if (!context.polish) {
        DrawCircleV(toRaylib(ground), 0.25f, theme::shadow());
        DrawCircleV(toRaylib(lifted), 0.3f, theme::kUiDark);
        DrawCircleLinesV(toRaylib(lifted), 0.3f, theme::kLineWhite);
        return;
    }
    // Higher ball: smaller, fainter shadow and a slightly bigger ball (closer to the camera).
    const float rise = std::clamp(height / 4.0f, 0.0f, 1.0f);
    DrawCircleV(toRaylib(ground), 0.32f * (1.0f - 0.45f * rise),
                theme::shadow(0.45f - 0.25f * rise));
    const float radius = 0.3f * (1.0f + 0.35f * rise);
    if (sprite != nullptr) {
        // Nose points along the flight path.
        const float angle = degrees(ball.target - ball.launch) - kBallSpriteAxisDegrees;
        drawSprite(*sprite, toRaylib(lifted), kBallYardsPerPixel * (1.0f + 0.35f * rise), angle,
                   WHITE);
        return;
    }
    DrawCircleV(toRaylib(lifted), radius, theme::kUiDark);
    DrawRing(toRaylib(lifted), radius - 0.06f, radius, 0.0f, 360.0f, 20, theme::kLineWhite);
    // Laces, perpendicular to the direction of flight.
    const Vec2 dir = normalized(ball.target - ball.launch);
    const Vec2 lace{-dir.y * radius * 0.5f, dir.x * radius * 0.5f};
    DrawLineEx(toRaylib(lifted - lace), toRaylib(lifted + lace), 0.05f, theme::kLineWhite);
}

// ================================================================ screen space

void drawPlayerLabels(const World& world, const RenderContext& context, bool showStates) {
    const float pixelsPerYard = context.camera.pixelsPerYard();
    const bool canThrow =
        world.ball().state == BallState::Held || world.phase() == PlayPhase::PreSnap;
    char buffer[16];

    for (const Player& player : world.players()) {
        const Vector2 center = context.camera.worldToScreen(interpolated(player, context.alpha));

        // Jersey number: inside the circle, or under the sprite (the sprite's head would be
        // covered otherwise), with a dark shadow so it reads on the bright grass.
        const bool sprites = context.polish && spriteFor(player, context.assets) != nullptr;
        std::snprintf(buffer, sizeof(buffer), "%d", player.jersey);
        if (sprites) {
            const Vector2 below{center.x,
                                center.y + (player.attributes.radius + 0.45f) * pixelsPerYard};
            drawCenteredText(buffer, Vector2{below.x + 1.0f, below.y + 1.0f}, 12,
                             theme::shadow(0.8f));
            drawCenteredText(buffer, below, 12, theme::kLineWhite);
        } else {
            const int fontSize = static_cast<int>(player.attributes.radius * pixelsPerYard * 1.1f);
            drawCenteredText(buffer, center, fontSize, theme::kLineWhite);
        }

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
            const float offset = sprites ? 1.3f : 0.5f; // stay clear of the jersey number
            const Vector2 below{center.x,
                                center.y + (player.attributes.radius + offset) * pixelsPerYard};
            drawCenteredText(toString(player.state), below, 12, theme::kLineWhite);
        }
    }
}

} // namespace gridiron::render
