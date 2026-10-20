#include "app/SceneManager.h"
#include "app/SceneStack.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace gridiron;

namespace {

// Records every call, so tests can check the exact order of enter/exit/update/draw.
class TestScene : public Scene {
public:
    TestScene(std::string name, std::vector<std::string>& log, bool overlay = false)
        : m_name(std::move(name)), m_log(log), m_overlay(overlay) {}

    void enter() override { m_log.push_back(m_name + ".enter"); }
    void exit() override { m_log.push_back(m_name + ".exit"); }
    void update(float) override { m_log.push_back(m_name + ".update"); }
    void draw() override { m_log.push_back(m_name + ".draw"); }
    bool isOverlay() const override { return m_overlay; }

private:
    std::string m_name;
    std::vector<std::string>& m_log;
    bool m_overlay;
};

std::unique_ptr<Scene> scene(const char* name, std::vector<std::string>& log,
                             bool overlay = false) {
    return std::make_unique<TestScene>(name, log, overlay);
}

} // namespace

TEST_CASE("SceneStack push and pop call enter and exit", "[scenes]") {
    std::vector<std::string> log;
    SceneStack stack;
    stack.push(scene("game", log));
    stack.push(scene("pause", log, true));
    CHECK(stack.size() == 2);
    stack.pop();
    CHECK(stack.size() == 1);
    CHECK(log == std::vector<std::string>{"game.enter", "pause.enter", "pause.exit"});
}

TEST_CASE("SceneStack replace swaps only the top scene", "[scenes]") {
    std::vector<std::string> log;
    SceneStack stack;
    stack.push(scene("title", log));
    stack.push(scene("menu", log));
    stack.replace(scene("game", log));
    CHECK(stack.size() == 2);
    CHECK(log == std::vector<std::string>{"title.enter", "menu.enter", "menu.exit", "game.enter"});
}

TEST_CASE("SceneStack reset exits everything top first", "[scenes]") {
    std::vector<std::string> log;
    SceneStack stack;
    stack.push(scene("game", log));
    stack.push(scene("pause", log, true));
    stack.reset(scene("menu", log));
    CHECK(stack.size() == 1);
    CHECK(log == std::vector<std::string>{"game.enter", "pause.enter", "pause.exit", "game.exit",
                                          "menu.enter"});
}

TEST_CASE("Only the top scene updates; overlays draw over the scene below", "[scenes]") {
    std::vector<std::string> log;
    SceneStack stack;
    stack.push(scene("menu", log));
    stack.push(scene("game", log));
    stack.push(scene("pause", log, true));
    log.clear();

    stack.updateTop(0.016f);
    stack.draw();
    // The game is frozen (no update) but still drawn under the pause overlay; the menu below
    // the opaque game scene is not drawn at all.
    CHECK(log == std::vector<std::string>{"pause.update", "game.draw", "pause.draw"});
}

TEST_CASE("Popping an empty stack is harmless", "[scenes]") {
    SceneStack stack;
    stack.pop();
    CHECK(stack.empty());
    CHECK(stack.top() == nullptr);
}

TEST_CASE("SceneManager applies a change at full black, halfway through the fade", "[scenes]") {
    std::vector<std::string> log;
    SceneManager manager(0.3f);
    manager.push(scene("title", log), true); // instant
    CHECK(manager.stack().size() == 1);

    manager.replace(scene("menu", log));
    manager.update(0.1f); // fading out: not applied yet
    CHECK(manager.fadeAlpha() > 0.5f);
    CHECK(manager.stack().size() == 1);
    CHECK(log.back() == "title.update");

    manager.update(0.06f); // passes the midpoint: applied, now fading in
    CHECK(log == std::vector<std::string>{"title.enter", "title.update", "title.exit", "menu.enter",
                                          "menu.update"});
    CHECK(manager.transitioning());

    manager.update(0.2f);
    CHECK_FALSE(manager.transitioning());
    CHECK(manager.fadeAlpha() == 0.0f);
}

TEST_CASE("SceneManager ignores a second request while one is pending", "[scenes]") {
    std::vector<std::string> log;
    SceneManager manager(0.3f);
    manager.push(scene("game", log), true);
    manager.push(scene("pause", log, true));
    manager.push(scene("pause2", log, true)); // e.g. Esc pressed twice: ignored
    manager.update(0.2f);
    manager.update(0.2f);
    CHECK(manager.stack().size() == 2);
}
