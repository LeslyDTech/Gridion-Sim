#include "Game.h"

#include "FieldGeometry.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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

    if (m_world.phase() == PlayPhase::PreSnap) {
        if (input.selectPlay >= 0 &&
            static_cast<std::size_t>(input.selectPlay) < m_playbook.size()) {
            selectPlay(static_cast<std::size_t>(input.selectPlay));
            m_playChosen = true; // a number key picks the play (even the one already highlighted)
            return;
        }
        if (input.nextPlay) {
            selectPlay((m_playIndex + 1) % m_playbook.size());
            m_playChosen = false; // browsing: back to selection
            return;
        }
        if (input.snap) {
            m_playChosen = true; // snapping runs the highlighted play
        }
        m_playClock = std::max(0.0f, m_playClock - dt);
    }

    const PlayPhase before = m_world.phase();
    m_world.step(input, dt);
    if (before == PlayPhase::Live && m_world.phase() == PlayPhase::Over) {
        onPlayEnded();
    }

    if (m_world.phase() == PlayPhase::Over) {
        m_resultTimer += dt;
        // Snap skips the banner, but not instantly: a held/mashed snap button shouldn't skip it.
        const bool skip = input.snap && m_resultTimer > 0.5f;
        if (m_resultTimer >= kResultSeconds || skip) {
            advanceToNextPlay();
        }
    }
}

int Game::playClockSeconds() const { return static_cast<int>(std::ceil(m_playClock)); }

void Game::setupPlay() {
    // A different (but reproducible) seed for every play, so broken tackles vary play to play.
    m_world.setup(m_playbook[m_playIndex], m_defense, m_ballSpot,
                  static_cast<unsigned>(m_playsRun) + 1u);
    m_resultTimer = 0.0f;
    m_playJustStarted = true;
}

void Game::selectPlay(std::size_t index) {
    if (index == m_playIndex) {
        return; // re-selecting the current play changes nothing (the clock keeps running)
    }
    m_playIndex = index;
    setupPlay(); // re-line up in the new formation; the play clock is not reset
}

void Game::onPlayEnded() {
    // Spot the ball and update down & distance right away, so the scorebug and the result
    // popup reflect this play while it is still on screen.
    m_ballSpot = nextBallSpot(m_world.outcome(), m_ballSpot);
    m_lastDriveEvent = m_drive.recordPlay(m_world.outcome(), m_ballSpot.x);
    if (m_lastDriveEvent == DriveEvent::TurnoverOnDowns) {
        // We only play offense, so a turnover simply starts a new drive.
        m_ballSpot = {Drive::kNewDriveLineX, field::kCenterY};
    }
}

void Game::advanceToNextPlay() {
    ++m_playsRun;
    m_playClock = kPlayClockSeconds;
    m_playChosen = false; // every new play starts with play selection
    setupPlay();          // the same play stays highlighted
}

} // namespace gridiron
