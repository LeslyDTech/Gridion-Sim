#include "PlayLoader.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace gridiron {
namespace {

using nlohmann::json;

// Loading is a cold path (it runs once at startup), so internally we use an exception to bail
// out from deep inside the validation code. It never escapes this file: parsePlay() catches it
// and returns a ParseResult instead.
class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& where, const std::string& what) {
    throw ParseError(where + ": " + what);
}

std::string childPath(const std::string& parent, const char* key) {
    return parent.empty() ? key : parent + "." + key;
}

const json& requireField(const json& object, const char* key, const std::string& where) {
    const auto it = object.find(key);
    if (it == object.end()) {
        fail(childPath(where, key), "missing required field");
    }
    return *it;
}

std::string readString(const json& object, const char* key, const std::string& where) {
    const json& value = requireField(object, key, where);
    if (!value.is_string()) {
        fail(childPath(where, key), "expected a string");
    }
    return value.get<std::string>();
}

float readNumber(const json& object, const char* key, const std::string& where) {
    const json& value = requireField(object, key, where);
    if (!value.is_number()) {
        fail(childPath(where, key), "expected a number");
    }
    return value.get<float>();
}

int readInt(const json& object, const char* key, const std::string& where) {
    const json& value = requireField(object, key, where);
    if (!value.is_number_integer()) {
        fail(childPath(where, key), "expected an integer");
    }
    return value.get<int>();
}

// A point is written as a two-number array: [x, y].
Vec2 readPoint(const json& value, const std::string& where) {
    if (!value.is_array() || value.size() != 2 || !value[0].is_number() || !value[1].is_number()) {
        fail(where, "expected a point [x, y]");
    }
    return {value[0].get<float>(), value[1].get<float>()};
}

// Looks a string up in a small name -> enum table.
template <typename Enum, std::size_t N>
Enum parseName(const std::string& name, const std::array<std::pair<const char*, Enum>, N>& table,
               const std::string& where) {
    for (const auto& [text, value] : table) {
        if (name == text) {
            return value;
        }
    }
    fail(where, "unknown value '" + name + "'");
}

constexpr std::array<std::pair<const char*, Role>, 10> kRoleNames = {{
    {"QB", Role::QB},
    {"HB", Role::HB},
    {"FB", Role::FB},
    {"WR", Role::WR},
    {"TE", Role::TE},
    {"OL", Role::OL},
    {"DL", Role::DL},
    {"LB", Role::LB},
    {"CB", Role::CB},
    {"S", Role::S},
}};

constexpr std::array<std::pair<const char*, Assignment>, 6> kAssignmentNames = {{
    {"none", Assignment::None},
    {"route", Assignment::RunRoute},
    {"block", Assignment::Block},
    {"rush", Assignment::PassRush},
    {"man", Assignment::ManCoverage},
    {"pursue", Assignment::Pursue},
}};

bool isOffensiveRole(Role role) {
    return role == Role::QB || role == Role::HB || role == Role::FB || role == Role::WR ||
           role == Role::TE || role == Role::OL;
}

std::vector<Vec2> readRoute(const json& object, const std::string& where) {
    const json& value = requireField(object, "route", where);
    const std::string path = childPath(where, "route");
    if (!value.is_array() || value.empty()) {
        fail(path, "expected a non-empty array of points");
    }
    std::vector<Vec2> route;
    route.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        route.push_back(readPoint(value[i], path + "[" + std::to_string(i) + "]"));
    }
    return route;
}

SlotDefinition readSlot(const json& object, Team side, const std::string& where) {
    if (!object.is_object()) {
        fail(where, "expected an object");
    }
    SlotDefinition slot;
    slot.slot = readString(object, "slot", where);
    slot.role = parseName(readString(object, "role", where), kRoleNames, childPath(where, "role"));
    slot.assignment = parseName(readString(object, "assignment", where), kAssignmentNames,
                                childPath(where, "assignment"));
    slot.jersey = readInt(object, "jersey", where);
    slot.alignment = {readNumber(object, "x", where), readNumber(object, "y", where)};

    if (slot.jersey < 0 || slot.jersey > 99) {
        fail(childPath(where, "jersey"), "must be between 0 and 99");
    }
    if (isOffensiveRole(slot.role) != (side == Team::Offense)) {
        fail(childPath(where, "role"), "role does not belong on this side of the ball");
    }
    // Offense lines up behind the ball (x <= 0), defense in front of it (x > 0).
    if ((side == Team::Offense) ? slot.alignment.x > 0.0f : slot.alignment.x <= 0.0f) {
        fail(childPath(where, "x"), "player is lined up offside");
    }
    if (slot.assignment == Assignment::RunRoute) {
        slot.route = readRoute(object, where);
    }
    if (slot.assignment == Assignment::ManCoverage) {
        slot.coverSlot = readString(object, "cover", where);
    }
    return slot;
}

void validateUniqueSlots(const std::vector<SlotDefinition>& players) {
    for (std::size_t i = 0; i < players.size(); ++i) {
        for (std::size_t j = i + 1; j < players.size(); ++j) {
            if (players[i].slot == players[j].slot) {
                fail("players[" + std::to_string(j) + "].slot",
                     "duplicate slot '" + players[j].slot + "'");
            }
        }
    }
}

// No two players may line up overlapping: their collision circles (radius by role, see
// attributesFor) must not intersect. This keeps spacing decisions in the formation data and stops
// a formation from starting with players pushing each other apart at the snap.
void validateNoOverlap(const std::vector<SlotDefinition>& players) {
    for (std::size_t i = 0; i < players.size(); ++i) {
        for (std::size_t j = i + 1; j < players.size(); ++j) {
            const float minDistance =
                attributesFor(players[i].role).radius + attributesFor(players[j].role).radius;
            if (distanceSquared(players[i].alignment, players[j].alignment) <
                minDistance * minDistance) {
                fail("players[" + std::to_string(j) + "]",
                     "'" + players[j].slot + "' overlaps '" + players[i].slot + "'");
            }
        }
    }
}

bool hasSlot(const std::vector<SlotDefinition>& players, const std::string& name) {
    return std::any_of(players.begin(), players.end(),
                       [&](const SlotDefinition& p) { return p.slot == name; });
}

PlayDefinition readPlay(const json& root) {
    if (!root.is_object()) {
        fail("(root)", "expected a JSON object");
    }
    PlayDefinition play;
    play.name = readString(root, "name", "");
    const std::string side = readString(root, "side", "");
    if (side != "offense" && side != "defense") {
        fail("side", "expected 'offense' or 'defense'");
    }
    play.side = side == "offense" ? Team::Offense : Team::Defense;

    const json& players = requireField(root, "players", "");
    if (!players.is_array() || players.size() != PlayDefinition::kPlayersPerSide) {
        fail("players", "expected an array of exactly 11 players");
    }
    play.players.reserve(players.size());
    for (std::size_t i = 0; i < players.size(); ++i) {
        play.players.push_back(
            readSlot(players[i], play.side, "players[" + std::to_string(i) + "]"));
    }
    validateUniqueSlots(play.players);
    validateNoOverlap(play.players);

    if (play.side == Team::Offense) {
        play.ballCarrierSlot = readString(root, "ballCarrier", "");
        if (!hasSlot(play.players, play.ballCarrierSlot)) {
            fail("ballCarrier", "no player with slot '" + play.ballCarrierSlot + "'");
        }
    }
    return play;
}

} // namespace

ParseResult<PlayDefinition> parsePlay(std::string_view jsonText) {
    ParseResult<PlayDefinition> result;
    // allow_exceptions = false: a syntax error returns a "discarded" value instead of throwing.
    const json root = json::parse(jsonText.begin(), jsonText.end(), nullptr, false);
    if (root.is_discarded()) {
        result.error = "invalid JSON syntax";
        return result;
    }
    try {
        result.value = readPlay(root);
    } catch (const ParseError& error) {
        result.error = error.what();
    }
    return result;
}

ParseResult<PlayDefinition> loadPlayFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return {std::nullopt, path.string() + ": cannot open file"};
    }
    std::ostringstream text;
    text << file.rdbuf();

    ParseResult<PlayDefinition> result = parsePlay(text.str());
    if (!result.ok()) {
        result.error = path.filename().string() + ": " + result.error;
    }
    return result;
}

Playbook loadPlaysFromDirectory(const std::filesystem::path& directory) {
    Playbook playbook;
    std::error_code ec;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    if (ec) {
        playbook.errors.push_back(directory.string() + ": " + ec.message());
        return playbook;
    }
    std::sort(files.begin(), files.end());

    for (const auto& file : files) {
        ParseResult<PlayDefinition> result = loadPlayFile(file);
        if (result.ok()) {
            playbook.plays.push_back(std::move(*result.value));
        } else {
            playbook.errors.push_back(std::move(result.error));
        }
    }
    return playbook;
}

} // namespace gridiron
