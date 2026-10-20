#include "InputDevice.h"

#include <raylib.h>

#include <array>
#include <cmath>

namespace gridiron {
namespace {

constexpr int kGamepad = 0;
constexpr float kStickDeadZone = 0.2f;

// Keys 1-5 select receivers 0-4.
constexpr std::array<int, 5> kThrowKeys = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE};

// Gamepad face buttons for receivers 0-4. Xbox layout: X, Y, B, RB, LB.
// A is reserved for the snap, the same split Madden uses (snap vs. pass icons).
constexpr std::array<int, 5> kThrowButtons = {
    GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_LEFT_TRIGGER_1};

bool gamepadPressed(int button) {
    return IsGamepadAvailable(kGamepad) && IsGamepadButtonPressed(kGamepad, button);
}

Vec2 keyboardMove() {
    Vec2 move;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move.x += 1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move.x -= 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) move.y += 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) move.y -= 1.0f;
    return normalized(move); // diagonal is not faster than straight
}

Vec2 stickMove() {
    if (!IsGamepadAvailable(kGamepad)) {
        return {};
    }
    // raylib's stick y axis points down, the same as our world y, so no flip is needed.
    const Vec2 stick{GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_X),
                     GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_Y)};
    return length(stick) < kStickDeadZone ? Vec2{} : clampLength(stick, 1.0f);
}

int pressedThrowTarget() {
    for (int i = 0; i < static_cast<int>(kThrowKeys.size()); ++i) {
        if (IsKeyPressed(kThrowKeys[i]) || gamepadPressed(kThrowButtons[i])) {
            return i;
        }
    }
    return InputCommands::kNoThrow;
}

} // namespace

void InputDevice::poll() {
    // Held state: always the latest value.
    const Vec2 keys = keyboardMove();
    m_pending.move = lengthSquared(keys) > 0.0f ? keys : stickMove();

    // Edge events: OR them in so they survive until a simulation step consumes them.
    m_pending.snap =
        m_pending.snap || IsKeyPressed(KEY_SPACE) || gamepadPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    m_pending.nextPlay = m_pending.nextPlay || IsKeyPressed(KEY_TAB) ||
                         gamepadPressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    if (const int target = pressedThrowTarget(); target != InputCommands::kNoThrow) {
        m_pending.throwTarget = target;
    }

    m_app.toggleDebug = IsKeyPressed(KEY_F1) || gamepadPressed(GAMEPAD_BUTTON_MIDDLE_LEFT);
    m_app.togglePolish = IsKeyPressed(KEY_F2);
    m_app.cycleFpsCap = IsKeyPressed(KEY_F3);
}

InputCommands InputDevice::consumeForStep() {
    const InputCommands commands = m_pending;
    m_pending.snap = false;
    m_pending.nextPlay = false;
    m_pending.throwTarget = InputCommands::kNoThrow;
    return commands;
}

} // namespace gridiron
