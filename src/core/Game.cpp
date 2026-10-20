#include "Game.h"

#include "FieldGeometry.h"

#include <algorithm>
#include <cassert>
#include <utility>

namespace gridiron {
namespace {

// The ball is always spotted between the hash marks, and never inside the 1-yard line.
Vec2 clampToPlayableSpot(Vec2 spot) {
    return {std::clamp(spot.x, field::kOwnGoalLine + 1.0f, field::kOpponentGoalLine - 1.0f),
            std::clamp(spot.y, field::kNearHashY, field::kFarHashY)};
}

} // namespace

Vec2 nextBallSpot(const PlayOutcome& outcome, Vec2 previousSpot) {
    switch (outcome.result) {
    case PlayResult::Tackled:
    case PlayResult::OutOfBounds:
        return clampToPlayableSpot(outcome.spot);
    case PlayResult::Touchdown:
    case PlayResult::Safety:
        // No scoring or kickoffs yet: start a fresh drive.
        return {field::kStartingLineOfScrimmageX, field::kCenterY};
    case PlayResult::Incomplete:
    case PlayResult::None:
        break;
    }
    return previousSpot;
}

Game::Game(std::vector<PlayDefinition> playbook, PlayDefinition defense)
    : m_playbook(std::move(playbook)), m_defense(std::move(defense)),
      m_ballSpot{field::kStartingLineOfScrimmageX, field::kCenterY} {
    assert(!m_playbook.empty() && "Game needs at least one play");
    setupPlay();
}

void Game::step(const InputCommands& input, float dt) {
    m_playJustStarted = false;

    if (m_world.phase() == PlayPhase::PreSnap && input.nextPlay) {
        m_playIndex = (m_playIndex + 1) % m_playbook.size();
        setupPlay();
        return;
    }

    m_world.step(input, dt);

    if (m_world.phase() == PlayPhase::Over) {
        m_resultTimer += dt;
        // Snap skips the banner, but not instantly: a held/mashed snap button shouldn't skip it.
        const bool skip = input.snap && m_resultTimer > 0.5f;
        if (m_resultTimer >= kResultSeconds || skip) {
            advanceToNextPlay();
        }
    }
}

void Game::setupPlay() {
    m_world.setup(m_playbook[m_playIndex], m_defense, m_ballSpot);
    m_resultTimer = 0.0f;
    m_playJustStarted = true;
}

void Game::advanceToNextPlay() {
    m_ballSpot = nextBallSpot(m_world.outcome(), m_ballSpot);
    if (m_drive.recordPlay(m_world.outcome(), m_ballSpot.x) == DriveEvent::TurnoverOnDowns) {
        // We only play offense, so a turnover simply starts a new drive.
        m_ballSpot = {Drive::kNewDriveLineX, field::kCenterY};
    }
    m_playIndex = (m_playIndex + 1) % m_playbook.size();
    ++m_playsRun;
    setupPlay();
}

} // namespace gridiron
