#pragma once

// Finding projects on disk and working out what they are.
// Plain C++20 (std::filesystem), no Qt: unit-tested on a fake tree.

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace hangar {

namespace fs = std::filesystem;

// Path from UTF-8 text (fs::u8path is deprecated in C++20).
inline fs::path utf8Path(const std::string& s)
{
    return fs::path(std::u8string(s.begin(), s.end()));
}

enum class Category {
    MiniApp,   // Telegram bot + web front (Mini App)
    Bot,       // Telegram bot
    Website,   // landing, site, SPA
    Desktop,   // Qt, Flet, Tkinter... applications
    Game,
    Script,    // everything else with code in it
    Study,     // labs, PythonProject7, empty sandboxes
    Archive,   // copies, backups
    Count,
};

constexpr int kCategoryCount = int(Category::Count);

// Folder name in the organised tree ("Боты", "Сайты"...). UTF-8.
const char* folderName(Category c);
const char* categoryKey(Category c);   // stable id for settings: "bot", "site"...
bool categoryFromKey(const std::string& key, Category* out);

struct Project {
    std::string name;
    fs::path path;
    Category category = Category::Script;
    std::vector<std::string> tags;   // "Python", "aiogram", "FastAPI"...
    std::string reason;              // why this category, for the card
    std::int64_t modified = 0;       // unix seconds, newest source file
    // git
    bool hasGit = false;
    std::string branch;
    std::int64_t lastCommit = 0;     // unix seconds, 0 = unknown
    std::string remoteUrl;           // https:// form when it's GitHub/GitLab
    // Suggested editors, best first ("CLion", "PyCharm", "Visual Studio Code").
    std::vector<std::string> editors;
};

// Does this folder look like a project (not an empty dir, not a build dir)?
bool looksLikeProject(const fs::path& dir);

// Projects inside the given folders. A folder that is itself a project
// (and has no project subfolders) counts as one. Symlinks are followed but
// duplicates (same real path) are dropped, so the organised tree can be
// added as a root without doubling everything.
std::vector<fs::path> findProjects(const std::vector<fs::path>& roots, const std::vector<fs::path>& extraProjects = {});

// Reads the project and classifies it.
Project inspect(const fs::path& dir);

} // namespace hangar
