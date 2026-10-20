#include "Effects.h"

#include "Assets.h"
#include "Easing.h"
#include "FollowCamera.h"
#include "Theme.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gridiron {
namespace {

constexpr float kTurnSharpness = 12.0f; // how quickly the facing indicator follows velocity
constexpr float kMinTurnSpeed = 1.0f;   // below this speed (yd/s) keep the old facing
constexpr float kRouteFadeSeconds = 0.5f;

constexpr float kDeepPassYards = 15.0f; // passes longer than this zoom the camera out
constexpr float kDeepPassZoom = 0.72f;
constexpr float kZoomSharpness = 3.0f;

constexpr float kMaxShakeYards = 0.6f;
constexpr float kTraumaDecayPerSecond = 1.6f;

constexpr float kPopupSeconds = 2.2f;
constexpr float kPopupPopSeconds = 0.35f;
constexpr float kPopupFadeSeconds = 0.6f;

// Exponential smoothing factor that behaves the same at any frame rate.
float smoothing(float sharpness, float seconds) { return 1.0f - std::exp(-sharpness * seconds); }

Vec2 defaultFacing(Team team) {
    return team == Team::Offense ? Vec2{1.0f, 0.0f} : Vec2{-1.0f, 0.0f};
}

// Short popup label for a finished play, e.g. "+7 YDS". Writes into a caller-owned buffer.
void formatPopup(const PlayOutcome& outcome, char* buffer, std::size_t size) {
    switch (outcome.result) {
    case PlayResult::Touchdown:
        std::snprintf(buffer, size, "TOUCHDOWN");
        return;
    case PlayResult::Safety:
        std::snprintf(buffer, size, "SAFETY");
        return;
    case PlayResult::Incomplete:
        std::snprintf(buffer, size, "INCOMPLETE");
        return;
    case PlayResult::Tackled:
    case PlayResult::OutOfBounds:
    case PlayResult::None:
        break;
    }
    const long yards = std::lround(outcome.yardsGained);
    std::snprintf(buffer, size, "%+ld YDS", yards);
}

Color popupColor(const PlayOutcome& outcome) {
    switch (outcome.result) {
    case PlayResult::Touchdown:
        return theme::kLineWhite;
    case PlayResult::Incomplete:
        return theme::kLineWhite;
    case PlayResult::Safety:
        return theme::kAway;
    case PlayResult::Tackled:
    case PlayResult::OutOfBounds:
    case PlayResult::None:
        break;
    }
    return outcome.yardsGained >= 0.0f ? theme::kLineWhite : theme::kAway; // losses in orange
}

} // namespace

Effects::Effects() { m_facing.fill(Vec2{1.0f, 0.0f}); }

void Effects::update(const World& world, float frameSeconds) {
    m_time += frameSeconds;

    const PlayPhase phase = world.phase();
    if (m_previousPhase == PlayPhase::Live && phase == PlayPhase::Over) {
        onPlayEnded(world.outcome());
    }
    if (phase == PlayPhase::PreSnap) {
        if (m_previousPhase != PlayPhase::PreSnap) {
            m_popup.active = false; // a new play has been set up
        }
    }
    m_previousPhase = phase;

    m_secondsSinceSnap = phase == PlayPhase::PreSnap ? 0.0f : m_secondsSinceSnap + frameSeconds;
    m_trauma = std::max(0.0f, m_trauma - kTraumaDecayPerSecond * frameSeconds);
    if (m_popup.active) {
        m_popup.age += frameSeconds;
        m_popup.active = m_popup.age < kPopupSeconds;
    }

    updateFacing(world, frameSeconds);
    updateParticles(frameSeconds);
    updateZoom(world, frameSeconds);
}

void Effects::onPlayEnded(const PlayOutcome& outcome) {
    m_popup = Popup{true, 0.0f, outcome.spot, outcome};

    if (outcome.result == PlayResult::Tackled || outcome.result == PlayResult::Safety) {
        m_trauma = std::min(1.0f, m_trauma + 0.75f);
        spawnBurst(outcome.spot, 36, 7.0f);
    } else if (outcome.result == PlayResult::OutOfBounds) {
        m_trauma = std::min(1.0f, m_trauma + 0.3f);
        spawnBurst(outcome.spot, 16, 4.0f);
    }
}

void Effects::spawnBurst(Vec2 at, int count, float speed) {
    std::uniform_real_distribution<float> angle(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    const std::array<Color, 4> colors = {theme::kGrassLight, theme::kGrassDark,
                                         ColorBrightness(theme::kGrassDark, -0.4f),
                                         theme::kLineWhite};
    for (int i = 0; i < count; ++i) {
        // Ring buffer: when the pool is full the oldest particle is overwritten.
        Particle& particle = m_particles[m_nextParticle];
        m_nextParticle = (m_nextParticle + 1) % m_particles.size();

        const float a = angle(m_random);
        const float s = speed * (0.3f + 0.7f * unit(m_random));
        particle.position = at;
        particle.velocity = Vec2{std::cos(a), std::sin(a)} * s;
        particle.age = 0.0f;
        particle.lifetime = 0.45f + 0.5f * unit(m_random);
        particle.size = 0.08f + 0.14f * unit(m_random);
        particle.color = colors[static_cast<std::size_t>(i) % colors.size()];
    }
}

void Effects::updateParticles(float frameSeconds) {
    const float drag = std::exp(-5.0f * frameSeconds);
    for (Particle& particle : m_particles) {
        if (particle.age >= particle.lifetime) {
            continue;
        }
        particle.age += frameSeconds;
        particle.position += particle.velocity * frameSeconds;
        particle.velocity *= drag;
    }
}

void Effects::updateZoom(const World& world, float frameSeconds) {
    const Ball& ball = world.ball();
    const bool deepPass =
        ball.state == BallState::InAir && distance(ball.launch, ball.target) > kDeepPassYards;
    const float target = deepPass ? kDeepPassZoom : 1.0f;
    m_zoom += (target - m_zoom) * smoothing(kZoomSharpness, frameSeconds);
}

void Effects::updateFacing(const World& world, float frameSeconds) {
    const float t = smoothing(kTurnSharpness, frameSeconds);
    for (const Player& player : world.players()) {
        const auto index = static_cast<std::size_t>(player.id);
        if (index >= m_facing.size()) {
            continue;
        }
        Vec2& facing = m_facing[index];
        if (world.phase() == PlayPhase::PreSnap) {
            facing = defaultFacing(player.team); // lined up looking across the ball
        } else if (lengthSquared(player.velocity) > kMinTurnSpeed * kMinTurnSpeed) {
            facing = normalized(lerp(facing, normalized(player.velocity), t));
        }
    }
}

Vec2 Effects::facing(int playerId) const {
    const auto index = static_cast<std::size_t>(playerId);
    return index < m_facing.size() ? m_facing[index] : Vec2{1.0f, 0.0f};
}

float Effects::routeOpacity() const {
    return std::clamp(1.0f - m_secondsSinceSnap / kRouteFadeSeconds, 0.0f, 1.0f);
}

Vec2 Effects::shakeOffset() const {
    // Squaring trauma makes small hits subtle and big hits punchy. Summed sines at unrelated
    // frequencies give a cheap, jittery but smooth "noise".
    const float strength = kMaxShakeYards * m_trauma * m_trauma;
    return {strength * (std::sin(m_time * 47.0f) + 0.5f * std::sin(m_time * 91.0f)) / 1.5f,
            strength * (std::cos(m_time * 53.0f) + 0.5f * std::sin(m_time * 79.0f)) / 1.5f};
}

void Effects::drawParticles() const {
    for (const Particle& particle : m_particles) {
        if (particle.age >= particle.lifetime) {
            continue;
        }
        const float life = 1.0f - particle.age / particle.lifetime;
        DrawCircleV(Vector2{particle.position.x, particle.position.y}, particle.size * life,
                    Fade(particle.color, life));
    }
}

void Effects::drawResultPopup(const RenderContext& context) const {
    if (!m_popup.active) {
        return;
    }
    char text[24];
    formatPopup(m_popup.outcome, text, sizeof(text));

    const float t = m_popup.age;
    const float scale = 0.4f + 0.6f * easing::outBack(t / kPopupPopSeconds);
    const float rise = 60.0f * easing::outCubic(t / kPopupSeconds);
    const float fadeStart = kPopupSeconds - kPopupFadeSeconds;
    const float opacity = t < fadeStart ? 1.0f : 1.0f - (t - fadeStart) / kPopupFadeSeconds;

    const Font& font = context.assets.font();
    const float fontSize = 64.0f * scale;
    const float spacing = 2.0f * scale;
    const Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    const Vector2 anchor = context.camera.worldToScreen(m_popup.spot);
    const Vector2 position{anchor.x - size.x / 2.0f, anchor.y - size.y - 30.0f - rise};

    // Dark drop shadow first so the text reads on any background.
    DrawTextEx(font, text, Vector2{position.x + 3.0f, position.y + 3.0f}, fontSize, spacing,
               theme::shadow(0.7f * opacity));
    DrawTextEx(font, text, position, fontSize, spacing, Fade(popupColor(m_popup.outcome), opacity));
}

} // namespace gridiron
