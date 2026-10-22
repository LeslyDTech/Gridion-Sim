#pragma once

#include "AppContext.h"
#include "app/Scene.h"

namespace gridiron {

// Title (Figma 8:369): the logo, subtitle and a pulsing PRESS START over the attract mode. The two
// "PRESS START" layers in the design are the bright and dim states of one line; it fades between
// them once per second. Any key, Enter, a click or gamepad A goes to the main menu.
class TitleScene : public Scene {
public:
    explicit TitleScene(AppContext& context) : m_context(context) {}

    void update(float frameSeconds) override;
    void draw() override;

private:
    AppContext& m_context;
    float m_time = 0.0f;
};

} // namespace gridiron
