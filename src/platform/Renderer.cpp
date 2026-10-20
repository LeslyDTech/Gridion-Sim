#include "Renderer.h"

#include "Assets.h"
#include "Effects.h"
#include "FollowCamera.h"
#include "Text.h"
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
// drawn 2.08x the collision radius wide: slightly wider than the collision circle (as real
// shoulders are), and small enough that linemen with 1-yard splits never visually overlap.
constexpr float kSpriteShoulderToRadius = 2.08f;
// When the camera pulls back to frame a wide formation, sprites would shrink to ~15 px. They are
// never drawn smaller than this (long side, on screen). Purely visual: collisions still use the
// physical radius, and the 1-yard line splits keep linemen apart even at this minimum.
constexpr float kMinSpritePixels = 26.0f;

// The sprite's long side (shoulders) in yards as drawn: physical size, or the on-screen minimum.
float spriteShoulderYards(const Player& player, float pixelsPerYard) {
    return std::max(kSpriteShoulderToRadius * player.attributes.radius,
                    kMinSpritePixels / pixelsPerYard);
}
// The ball sprite's long axis is drawn at ~55 degrees (measured from its pixels), so that angle
// is subtracted to line the ball up with its direction of travel.
constexpr float kBallSpriteAxisDegrees = 54.7f;
constexpr float kBallYardsPerPixel = 0.05f; // 14 x 16 px -> 0.7 x 0.8 yards

// Player shadows: a soft round shadow slightly larger than the sprite (at 70% it was hidden under
// it) at 25% opacity, offset 2.5 px down-right as if lit from the top-left. The offset is in
// screen pixels, so it looks the same at every camera zoom.
constexpr float kShadowSizeOfSprite = 1.05f;
constexpr float kShadowOpacity = 0.25f;
constexpr float kShadowOffsetPixels = 2.5f;

void drawSoftShadow(const RenderContext& context, Vector2 center, float spriteSize) {
    const Texture2D& texture = context.assets.softShadow();
    const float size = spriteSize * kShadowSizeOfSprite;
    const float offset = kShadowOffsetPixels / context.camera.pixelsPerYard();
    DrawTexturePro(texture,
                   Rectangle{0.0f, 0.0f, static_cast<float>(texture.width),
                             static_cast<float>(texture.height)},
                   Rectangle{center.x + offset, center.y + offset, size, size},
                   Vector2{size / 2.0f, size / 2.0f}, 0.0f, Fade(WHITE, kShadowOpacity));
}

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
             Fade(theme::kLineWhite, 0.9f));
}

void drawSpritePlayer(const Player& player, const Texture2D& sprite, Vector2 center, Vec2 facing,
                      const RenderContext& context) {
    const float radius = player.attributes.radius;
    const float shoulderWidth = spriteShoulderYards(player, context.camera.pixelsPerYard());
    const float yardsPerPixel = shoulderWidth / static_cast<float>(sprite.height);

    drawSoftShadow(context, center, shoulderWidth);
    if (player.state == PlayerState::BallCarrier) {
        // Ring sized to the drawn sprite, so it still surrounds it at the minimum size.
        drawCarrierRing(center, std::max(radius, shoulderWidth * 0.45f), context.time);
    }
    // A tackled player is drawn darker. (WHITE = the sprite's own colors, untinted.)
    const Color tint =
        player.state == PlayerState::Tackled ? ColorBrightness(WHITE, -0.35f) : WHITE;
    drawSprite(sprite, center, yardsPerPixel, degrees(facing), tint);
}

// --- Code-drawn shapes (fallback when the art is missing) -------------------------------------

void drawPolishedPlayer(const Player& player, Vector2 center, Vec2 facing,
                        const RenderContext& context) {
    const float radius = player.attributes.radius;
    const double time = context.time;
    constexpr int kSegments = 32;

    drawSoftShadow(context, center, 2.0f * radius);

    if (player.state == PlayerState::BallCarrier) {
        const float pulse = 0.06f * static_cast<float>(std::sin(time * 6.0));
        DrawRing(center, radius + 0.12f, radius + 0.34f + pulse, 0.0f, 360.0f, kSegments,
                 Fade(theme::kLineWhite, 0.9f));
    }

    DrawCircleSector(center, radius, 0.0f, 360.0f, kSegments, teamColor(player.team));
    DrawRing(center, radius - 0.09f, radius, 0.0f, 360.0f, kSegments, theme::kLineWhite);
    drawFacingIndicator(center, radius, facing);

    if (player.state == PlayerState::Tackled) {
        DrawRing(center, radius + 0.1f, radius + 0.2f, 0.0f, 360.0f, kSegments, theme::kUiDark);
    }
}

void drawPlainPlayer(const Player& player, Vector2 center) {
    const float radius = player.attributes.radius;
    if (player.state == PlayerState::BallCarrier) {
        DrawCircleV(center, radius + 0.3f, theme::kLineWhite);
    }
    DrawCircleV(center, radius, teamColor(player.team));
    DrawCircleLinesV(center, radius, theme::kLineWhite);
    if (player.state == PlayerState::Tackled) {
        DrawCircleV(center, radius * 0.4f, theme::kUiDark);
    }
}

// Playbook-style route: straight legs with every cut rounded off by a short quadratic Bezier,
// finished with an arrowhead. (A Catmull-Rom spline through the waypoints looks smoother but
// overshoots at sharp cuts, which misrepresents the route.)
//
// `grow` widens the line and arrowhead; the route is drawn twice, a slightly grown dark pass
// first (the outline) and then the white route on top.
void drawCurvedRoute(const Player& player, Color color, float grow) {
    const float thickness = 0.12f + 2.0f * grow;
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
        DrawLineEx(toRaylib(legStart), toRaylib(entry), thickness, color);
        DrawSplineSegmentBezierQuadratic(toRaylib(entry), toRaylib(corner), toRaylib(exit),
                                         thickness, color);
        legStart = exit;
    }
    const Vec2 end = player.route.back();
    DrawLineEx(toRaylib(legStart), toRaylib(end), thickness, color);

    // Arrowhead pointing along the final leg.
    const Vec2 dir = normalized(end - legStart);
    const Vec2 side{-dir.y, dir.x};
    const Vec2 tip = end + dir * (0.55f + 1.5f * grow);
    const Vec2 left = end - dir * (0.1f + grow) + side * (0.38f + grow);
    const Vec2 right = end - dir * (0.1f + grow) - side * (0.38f + grow);
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

// The players a viewer tracks: the ones who can carry or catch the ball. Linemen and defenders
// stay unlabeled in the polished view to keep the screen readable.
bool isSkillPosition(Role role) {
    switch (role) {
    case Role::QB:
    case Role::HB:
    case Role::FB:
    case Role::WR:
    case Role::TE:
        return true;
    case Role::OL:
    case Role::DL:
    case Role::LB:
    case Role::CB:
    case Role::S:
        break;
    }
    return false;
}

bool showJerseyNumber(const Player& player, const RenderContext& context, bool debug) {
    return !context.polish || debug || isSkillPosition(player.role);
}

void drawCenteredText(const char* text, Vector2 center, int fontSize, Color color);

// Inside the circle, or under the sprite (the sprite's head would be covered otherwise), with a
// dark shadow so it reads on the bright grass. With polish on the TTF font is used; plain mode
// keeps raylib's default font.
void drawJerseyNumber(const Player& player, Vector2 center, float pixelsPerYard, bool sprites,
                      const RenderContext& context) {
    char number[4];
    std::snprintf(number, sizeof(number), "%d", player.jersey);
    if (!context.polish) {
        const int fontSize = static_cast<int>(player.attributes.radius * pixelsPerYard * 1.1f);
        drawCenteredText(number, center, fontSize, theme::kLineWhite);
        return;
    }
    const Font& font = context.assets.font();
    if (sprites) {
        // Scales with the camera zoom, but never below a readable size when the camera pulls back
        // to keep a wide formation in frame.
        constexpr float kMinNumberPixels = 12.0f;
        const float size = std::max(kMinNumberPixels, 0.5f * pixelsPerYard);
        // Just under the drawn sprite (its shoulders run vertically when facing downfield).
        const float spriteHalfPixels =
            spriteShoulderYards(player, pixelsPerYard) * pixelsPerYard / 2.0f;
        const Vector2 below{center.x, center.y + spriteHalfPixels + size * 0.5f};
        text::drawCentered(font, number, Vector2{below.x + 1.5f, below.y + 1.5f}, size,
                           theme::shadow(0.8f));
        text::drawCentered(font, number, below, size, theme::kLineWhite);
    } else {
        text::drawCentered(font, number, center, player.attributes.radius * pixelsPerYard * 1.3f,
                           theme::kLineWhite);
    }
}

// Text centered on a screen point.
void drawCenteredText(const char* text, Vector2 center, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, static_cast<int>(center.x) - width / 2,
             static_cast<int>(center.y) - fontSize / 2, fontSize, color);
}

// Throw-button badges: a dark disc with the receiver's number, at a fixed offset diagonally
// above-left of the player, just outside the sprite's edge, so it never covers the player and
// reads the same at every zoom.
constexpr float kBadgeRadiusPixels = 9.0f;
constexpr float kBadgeGapPixels = 2.0f;

bool showsBadge(const Player& player, const World& world) {
    const bool canThrow =
        world.ball().state == BallState::Held || world.phase() == PlayPhase::PreSnap;
    return player.receiverNumber != Player::kNone && canThrow &&
           (player.state == PlayerState::PreSnap || player.state == PlayerState::RunningRoute ||
            player.state == PlayerState::Idle);
}

// Draws the badge disc with the receiver's number, centered on `at`.
void drawBadge(const Player& player, Vector2 at, float radius, const RenderContext& context) {
    using theme::hud::Weight;
    DrawCircleV(at, radius, theme::panel(0.85f));
    char number[4];
    std::snprintf(number, sizeof(number), "%d", player.receiverNumber + 1);
    if (context.polish) {
        text::drawCentered(context.assets.hudFont(Weight::Bold, 17.0f), number, at, 17.0f,
                           theme::kLineWhite);
    } else {
        drawCenteredText(number, at, 14, theme::kLineWhite);
    }
}

// The HUD-free part of the screen, in pixels.
Rectangle safeRectangle(const RenderContext& context) {
    const SafeArea& safe = context.camera.safeArea();
    return {safe.side, safe.top, static_cast<float>(GetScreenWidth()) - 2.0f * safe.side,
            static_cast<float>(GetScreenHeight()) - safe.top - safe.bottom};
}

bool contains(Rectangle rect, Vector2 point) {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

// Edge arrow for a receiver outside the safe area: a badge pinned to the safe area's edge on the
// line from its center toward the receiver, with a triangle pointing the way.
constexpr float kArrowBadgeRadius = 11.0f;
constexpr float kArrowEdgeInset = 20.0f; // keeps badge + arrow fully inside the safe area

void drawEdgeArrow(const Player& player, Vector2 target, Rectangle safe,
                   const RenderContext& context) {
    const Vector2 middle{safe.x + safe.width / 2.0f, safe.y + safe.height / 2.0f};
    const Vec2 toTarget{target.x - middle.x, target.y - middle.y};
    const float halfWidth = safe.width / 2.0f - kArrowEdgeInset;
    const float halfHeight = safe.height / 2.0f - kArrowEdgeInset;
    // Scale the direction so it just reaches the inset rectangle's edge.
    const float tx = std::abs(toTarget.x) > 1e-3f ? halfWidth / std::abs(toTarget.x) : 1.0e9f;
    const float ty = std::abs(toTarget.y) > 1e-3f ? halfHeight / std::abs(toTarget.y) : 1.0e9f;
    const float t = std::min(tx, ty);
    const Vector2 at{std::round(middle.x + toTarget.x * t), std::round(middle.y + toTarget.y * t)};

    const Vec2 dir = normalized(toTarget);
    const Vec2 side{-dir.y, dir.x};
    const Vec2 base{at.x + dir.x * (kArrowBadgeRadius + 1.0f),
                    at.y + dir.y * (kArrowBadgeRadius + 1.0f)};
    const Vec2 tip = base + dir * 7.0f;
    const Vec2 left = base + side * 6.0f;
    const Vec2 right = base - side * 6.0f;
    DrawTriangle(toRaylib(tip), toRaylib(right), toRaylib(left), Fade(theme::kLineWhite, 0.9f));
    drawBadge(player, at, kArrowBadgeRadius, context);
}

void drawReceiverBadges(const World& world, const RenderContext& context) {
    const float pixelsPerYard = context.camera.pixelsPerYard();
    const Rectangle safe = safeRectangle(context);
    for (const Player& player : world.players()) {
        if (!showsBadge(player, world)) {
            continue;
        }
        const Vector2 center = context.camera.worldToScreen(interpolated(player, context.alpha));
        // Distance from the player's center to the sprite's edge along the diagonal, plus the
        // badge radius and a small gap: the badge sits just beyond the top-left of the sprite.
        const float spriteHalf = spriteShoulderYards(player, pixelsPerYard) / 2.0f;
        const float diagonal =
            (spriteHalf * pixelsPerYard + kBadgeGapPixels) * 0.70710678f + kBadgeRadiusPixels;
        const Vector2 badge{std::round(center.x - diagonal), std::round(center.y - diagonal)};
        if (contains(safe, center)) {
            drawBadge(player, badge, kBadgeRadiusPixels, context);
        } else {
            drawEdgeArrow(player, center, safe, context); // off-screen or under the HUD
        }
    }
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
        // Broadcast-style: a soft glow under a solid yellow line.
        DrawRectangleRec(Rectangle{x - 0.35f, 0.0f, 0.7f, field::kWidth},
                         Fade(theme::kFirstDownYellow, 0.2f));
    }
    DrawLineEx(Vector2{x, 0.0f}, Vector2{x, field::kWidth}, 0.22f,
               Fade(theme::kFirstDownYellow, 0.9f));
}

void drawRoutes(const World& world, const RenderContext& context, float opacity) {
    // Plain mode: routes simply disappear at the snap.
    if (!context.polish && world.phase() != PlayPhase::PreSnap) {
        return;
    }
    if (opacity <= 0.0f) {
        return;
    }
    // White at ~85% with a thin dark outline so routes read on bright grass. (Yellow is
    // reserved for the first-down line.) The outline is drawn first, so a translucent white on
    // top would let it show through and turn the route grey. Instead the route uses the opaque
    // color that 85% white over the grass produces: it looks the same on the field.
    const Color whiteOnGrass =
        ColorAlphaBlend(theme::kGrassLight, Fade(theme::kLineWhite, 0.85f), WHITE);
    const Color color = Fade(whiteOnGrass, opacity);
    const Color outline = theme::shadow(0.55f * opacity);
    for (const Player& player : world.players()) {
        if (player.route.empty()) {
            continue;
        }
        if (context.polish) {
            drawCurvedRoute(player, outline, 0.04f);
            drawCurvedRoute(player, color, 0.0f);
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
            drawSpritePlayer(player, *sprite, center, effects.facing(player.id), context);
        } else if (context.polish) {
            drawPolishedPlayer(player, center, effects.facing(player.id), context);
        } else {
            drawPlainPlayer(player, center);
        }
    }
}

void drawBall(const World& world, const RenderContext& context) {
    const Ball& ball = world.ball();
    const Vec2 ground = lerp(ball.previousPosition, ball.position, context.alpha);
    if (ball.state == BallState::Held) {
        return; // the carrier ring shows who has it
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

void drawPlayerLabels(const World& world, const RenderContext& context, bool debug) {
    const float pixelsPerYard = context.camera.pixelsPerYard();

    // Pass 1: jersey numbers and (debug) state names.
    for (const Player& player : world.players()) {
        const Vector2 center = context.camera.worldToScreen(interpolated(player, context.alpha));
        const bool sprites = context.polish && spriteFor(player, context.assets) != nullptr;
        if (showJerseyNumber(player, context, debug)) {
            drawJerseyNumber(player, center, pixelsPerYard, sprites, context);
        }
        if (debug) {
            const float offset = sprites ? 1.3f : 0.5f; // stay clear of the jersey number
            const Vector2 below{center.x,
                                center.y + (player.attributes.radius + offset) * pixelsPerYard};
            drawCenteredText(toString(player.state), below, 12, theme::kLineWhite);
        }
    }

    // Pass 2: throw-button badges, after everything else so no sprite or label can cover one.
    drawReceiverBadges(world, context);
}

} // namespace gridiron::render
