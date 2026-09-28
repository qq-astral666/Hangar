#pragma once

// "Sort into folders" without moving anything: builds
//   ~/Projects/Боты/kbzh -> ~/PycharmProjects/kbzh
//   ~/Projects/Сайты/last -> ~/PycharmProjects/last
// as symlinks. Real folders stay where they are, so .venv (absolute paths
// inside), CMake build dirs and IDE project lists keep working.

#include "Project.h"

#include <string>
#include <vector>

namespace hangar {

struct Link {
    fs::path link;     // ~/Projects/Боты/kbzh
    fs::path target;   // the real project folder
};

struct OrganizeResult {
    int created = 0;     // new links
    int kept = 0;        // already correct
    int removed = 0;     // stale links cleaned up (only symlinks, never real files)
    std::vector<std::string> errors;
};

// Where every project should appear. Name clashes get " (2)", " (3)"...
std::vector<Link> plan(const std::vector<Project>& projects, const fs::path& targetRoot);

// Creates the category folders and links, and removes symlinks in them that
// point nowhere or to a project that now belongs elsewhere. Anything that
// isn't a symlink is never touched.
OrganizeResult organize(const std::vector<Project>& projects, const fs::path& targetRoot);

// Removes links in the organised tree that point to `project` (it was
// deleted). Returns how many were removed.
int removeLinksTo(const fs::path& targetRoot, const fs::path& project);

} // namespace hangar
