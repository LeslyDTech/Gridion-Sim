#include "app/Settings.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

using namespace gridiron;

namespace {

// A file path in the system temp folder, removed when the test ends.
struct TempFile {
    std::filesystem::path path;
    explicit TempFile(const char* name) : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove(path);
    }
    ~TempFile() { std::filesystem::remove(path); }
};

} // namespace

TEST_CASE("Settings survive a save and load", "[settings]") {
    TempFile file("gridiron_settings_roundtrip.json");
    Settings settings;
    settings.masterVolume = 0.35f;
    settings.fullscreen = true;
    settings.showDebugOnStart = true;
    settings.reducedEffects = true;

    REQUIRE(saveSettings(settings, file.path));
    CHECK(loadSettings(file.path) == settings);
}

TEST_CASE("A missing settings file gives the defaults", "[settings]") {
    TempFile file("gridiron_settings_missing.json");
    CHECK(loadSettings(file.path) == Settings{});
}

TEST_CASE("A broken settings file gives the defaults", "[settings]") {
    TempFile file("gridiron_settings_broken.json");
    std::ofstream(file.path) << "{ this is not json";
    CHECK(loadSettings(file.path) == Settings{});
}

TEST_CASE("Missing or wrong-typed fields keep their defaults", "[settings]") {
    const Settings settings =
        settingsFromJson(R"({ "fullscreen": true, "reducedEffects": "yes" })");
    CHECK(settings.fullscreen);
    CHECK_FALSE(settings.reducedEffects); // wrong type: default kept
    CHECK(settings.masterVolume == Catch::Approx(Settings{}.masterVolume));
}

TEST_CASE("Master volume is clamped to 0..1", "[settings]") {
    CHECK(settingsFromJson(R"({ "masterVolume": 3.5 })").masterVolume == 1.0f);
    CHECK(settingsFromJson(R"({ "masterVolume": -1 })").masterVolume == 0.0f);
}
