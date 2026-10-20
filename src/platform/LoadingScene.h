#pragma once

#include "AppContext.h"
#include "AssetLoader.h"
#include "app/Scene.h"

namespace gridiron {

// Loading (Figma 8:344): streams every asset in through the AssetLoader with a real progress bar,
// the file being loaded, and a rotating tip. Stays up at least kLoadingMinimumSeconds so it never
// just flashes. If files are missing it lists them instead of crashing: missing art or fonts can be
// skipped (the game falls back to shapes / the built-in font); missing plays can't.
class LoadingScene : public Scene {
public:
    explicit LoadingScene(AppContext& context);

    void enter() override;
    void update(float frameSeconds) override;
    void draw() override;

private:
    void proceed();

    AppContext& m_context;
    AssetLoader m_loader;
    float m_elapsed = 0.0f;
    float m_shownProgress = 0.0f; // eases toward the real progress so the bar moves smoothly
    bool m_proceeding = false;
};

} // namespace gridiron
