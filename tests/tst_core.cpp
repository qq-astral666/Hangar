// Classifier and organiser tests on a fake home folder that mirrors a real
// mix of projects: CMake/Qt apps, aiogram bots, Mini Apps, landings, labs.

#include "Organizer.h"
#include "Project.h"

#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

using namespace hangar;

namespace {

int g_failed = 0;
int g_checks = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        ++g_checks;                                                                    \
        if (!(cond)) {                                                                 \
            ++g_failed;                                                                \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);              \
        }                                                                              \
    } while (0)

#define CHECK_CAT(proj, expected)                                                                    \
    do {                                                                                             \
        ++g_checks;                                                                                  \
        if ((proj).category != (expected)) {                                                         \
            ++g_failed;                                                                              \
            std::printf("  FAIL %s:%d: %s is %s (%s), expected %s\n", __FILE__, __LINE__,            \
                        (proj).name.c_str(), categoryKey((proj).category), (proj).reason.c_str(),    \
                        categoryKey(expected));                                                      \
        }                                                                                            \
    } while (0)

struct Test {
    const char* name;
    std::function<void()> fn;
};
std::vector<Test>& registry()
{
    static std::vector<Test> t;
    return t;
}
struct Register {
    Register(const char* n, std::function<void()> f) { registry().push_back({ n, std::move(f) }); }
};
#define TEST(name)                           \
    static void name();                      \
    static Register reg_##name(#name, name); \
    static void name()

fs::path g_root;

void write(const fs::path& file, const std::string& text = "")
{
    fs::create_directories(file.parent_path());
    std::ofstream(file) << text;
}

fs::path home()
{
    return g_root / "home";
}

// A fake home that looks like the real one.
void buildTree()
{
    const fs::path c = home() / "CLionProjects";
    write(c / "Isle/CMakeLists.txt", "find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Quick)\n");
    write(c / "Isle/src/main.cpp", "int main(){}");
    write(c / "Isle/.git/HEAD", "ref: refs/heads/main\n");
    write(c / "Isle/.git/logs/HEAD",
          "0000 1111 Vlad <v@x.y> 1790000000 +1000\tcommit (initial): x\n"
          "1111 2222 Vlad <v@x.y> 1790500000 +1000\tcommit: y\n");
    write(c / "Isle/.git/config", "[core]\n\tbare = false\n[remote \"origin\"]\n\turl = git@github.com:qq-astral666/isle.git\n");
    write(c / "Prism/CMakeLists.txt", "FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git)\n");
    write(c / "Prism/src/main.cpp", "#include <raylib.h>");
    write(c / "2laba/CMakeLists.txt", "add_executable(2laba main.cpp)\n");
    write(c / "2laba/main.cpp", "int main(){}");
    write(c / "mdk1/CMakeLists.txt", "add_executable(mdk1 main.cpp)\n");
    write(c / "mdk1/main.cpp", "int main(){}");
    write(c / "build/CMakeCache.txt", "");   // not a project
    write(c / "tool/CMakeLists.txt", "add_executable(tool main.cpp)\n");
    write(c / "tool/main.cpp", "int main(){}");

    const fs::path p = home() / "PycharmProjects";
    write(p / "Beuty/bot.py", "from aiogram import Bot\n");
    write(p / "Beuty/admin.html", "<script src=\"https://telegram.org/js/telegram-web-app.js\"></script>");
    write(p / "Beuty/.venv/bin/python", "");
    write(p / "kbzh/bot.py", "from aiogram import Bot\nfrom fastapi import FastAPI\n");
    write(p / "kbzh/frontend/app.js", "const tg = window.Telegram.WebApp;");
    write(p / "PythonProject2/main.py", "import telebot\n");
    write(p / "knb/main.py", "import random\nimport telebot\n");
    write(p / "xz/main.py", "from aiogram import Bot\nfrom fastapi import FastAPI\n");
    write(p / "xz/requirements.txt", "aiogram==3.4\nfastapi\n");
    write(p / "xz/Dockerfile", "FROM python:3.12");
    write(p / "PythonProject3/main.py", "print(1)\n");
    write(p / "PythonProject5/.idea/misc.xml", "");
    write(p / "PythonProject5/.venv/bin/python", "");
    write(p / "axaxax/main.py", "print('hi')\n");
    write(p / "tkinder/main.py", "import tkinter as tk\n");
    write(p / "spotify/player.py", "import flet as ft\n");
    write(p / "last/index.html", "<html></html>");
    write(p / "last/style.css", "");
    write(p / "last/.git/HEAD", "ref: refs/heads/master\n");
    write(p / "my-landing/index.html", "<html></html>");
    write(p / "my-landing — копия/index.html", "<html></html>");
    write(p / "landing_backup/index.html", "<html></html>");
    write(p / "newland/package.json", "{ \"dependencies\": { \"react\": \"^18\" }, \"devDependencies\": { \"vite\": \"^5\" } }");
    write(p / "newland/index.html", "<div id=root></div>");
    write(p / "newland/node_modules/react/index.js", "from aiogram import X  // must be ignored");
    write(p / "Агенство/index.html", "<html></html>");
    write(p / "Агенство/about.html", "<html></html>");

    write(home() / "my_code/miss-you-app/package.json",
          "{ \"dependencies\": { \"express\": \"^4\", \"node-telegram-bot-api\": \"^0.66\" } }");
    write(home() / "my_code/miss-you-app/server.js", "const express = require('express')");
    write(home() / "my_code/miss-you-app/public/index.html", "<html></html>");

    // A root that is itself a project (loose React sources).
    write(home() / "src/App.jsx", "import React from 'react'");
    write(home() / "src/main.jsx", "import React from 'react'");
}

std::map<std::string, Project> scanAll()
{
    std::map<std::string, Project> out;
    const auto paths = findProjects({ home() / "CLionProjects", home() / "PycharmProjects", home() / "my_code", home() / "src" });
    for (const fs::path& path : paths) {
        Project p = inspect(path);
        out[p.name] = p;
    }
    return out;
}

} // namespace

TEST(findsProjectsAndSkipsNonProjects)
{
    const auto all = scanAll();
    CHECK(all.count("Isle") && all.count("kbzh") && all.count("miss-you-app"));
    CHECK(!all.count("build"));          // CMake build folder
    CHECK(all.count("PythonProject5"));  // empty but has .idea: a (study) project
    CHECK(all.count("src"));             // root that is itself a project
    CHECK(all.size() == 23);
}

TEST(classifiesRealWorldProjects)
{
    auto all = scanAll();
    CHECK_CAT(all["Isle"], Category::Desktop);
    CHECK_CAT(all["Prism"], Category::Game);
    CHECK_CAT(all["2laba"], Category::Study);
    CHECK_CAT(all["mdk1"], Category::Study);
    CHECK_CAT(all["tool"], Category::Script);
    CHECK_CAT(all["Beuty"], Category::MiniApp);
    CHECK_CAT(all["kbzh"], Category::MiniApp);
    CHECK_CAT(all["xz"], Category::MiniApp);        // bot + FastAPI
    CHECK_CAT(all["PythonProject2"], Category::Bot);
    CHECK_CAT(all["knb"], Category::Bot);
    CHECK_CAT(all["PythonProject3"], Category::Study);
    CHECK_CAT(all["PythonProject5"], Category::Study);
    CHECK_CAT(all["axaxax"], Category::Script);
    CHECK_CAT(all["tkinder"], Category::Desktop);
    CHECK_CAT(all["spotify"], Category::Desktop);
    CHECK_CAT(all["last"], Category::Website);
    CHECK_CAT(all["my-landing"], Category::Website);
    CHECK_CAT(all["my-landing — копия"], Category::Archive);
    CHECK_CAT(all["landing_backup"], Category::Archive);
    CHECK_CAT(all["newland"], Category::Website);
    CHECK_CAT(all["Агенство"], Category::Website);
    CHECK_CAT(all["miss-you-app"], Category::MiniApp);
    CHECK_CAT(all["src"], Category::Website);
}

TEST(tagsAndReasons)
{
    auto all = scanAll();
    const auto hasTag = [](const Project& p, const char* t) {
        return std::find(p.tags.begin(), p.tags.end(), t) != p.tags.end();
    };
    CHECK(hasTag(all["kbzh"], "aiogram") && hasTag(all["kbzh"], "FastAPI") && hasTag(all["kbzh"], "Telegram Mini App"));
    CHECK(all["kbzh"].reason.find("frontend/app.js") != std::string::npos);
    CHECK(hasTag(all["newland"], "React") && hasTag(all["newland"], "Vite"));
    CHECK(!hasTag(all["newland"], "aiogram"));   // node_modules ignored
    CHECK(hasTag(all["Isle"], "Qt") && hasTag(all["Isle"], "C++"));
    CHECK(hasTag(all["xz"], "Docker"));
    CHECK(all["PythonProject2"].reason == "Telegram-бот на pyTelegramBotAPI");
}

TEST(readsGit)
{
    auto all = scanAll();
    const Project& isle = all["Isle"];
    CHECK(isle.hasGit);
    CHECK(isle.branch == "main");
    CHECK(isle.lastCommit == 1790500000);
    CHECK(isle.remoteUrl == "https://github.com/qq-astral666/isle");
    CHECK(all["last"].hasGit && all["last"].branch == "master" && all["last"].remoteUrl.empty());
    CHECK(!all["kbzh"].hasGit);
}

TEST(suggestsEditors)
{
    auto all = scanAll();
    CHECK(all["Isle"].editors.front() == "CLion");
    CHECK(all["kbzh"].editors.front() == "PyCharm");
    CHECK(all["newland"].editors.front() == "Visual Studio Code");
}

TEST(organizeCreatesLinksAndIsIdempotent)
{
    auto all = scanAll();
    std::vector<Project> list;
    for (auto& [n, p] : all)
        list.push_back(p);
    const fs::path target = home() / "Projects";

    const OrganizeResult r1 = organize(list, target);
    CHECK(r1.errors.empty());
    CHECK(r1.created == int(list.size()));
    const fs::path link = target / utf8Path("Мини-аппы") / "kbzh";
    CHECK(fs::is_symlink(link));
    CHECK(fs::exists(link / "bot.py"));   // link resolves
    CHECK(fs::exists(home() / "PycharmProjects/kbzh/bot.py"));   // original untouched

    const OrganizeResult r2 = organize(list, target);
    CHECK(r2.created == 0 && r2.removed == 0 && r2.kept == int(list.size()));

    // Re-categorise: link moves, old one is removed.
    for (Project& p : list)
        if (p.name == "kbzh")
            p.category = Category::Bot;
    const OrganizeResult r3 = organize(list, target);
    CHECK(r3.created == 1 && r3.removed == 1);
    CHECK(!fs::exists(fs::symlink_status(link)));
    CHECK(fs::is_symlink(target / utf8Path("Боты") / "kbzh"));

    // A real folder in the tree is never touched.
    write(target / utf8Path("Боты") / "notes" / "todo.txt", "keep me");
    const OrganizeResult r4 = organize(list, target);
    CHECK(fs::exists(target / utf8Path("Боты") / "notes" / "todo.txt"));
    CHECK(r4.removed == 0);

    // Adding the organised tree as a root doesn't duplicate projects.
    const auto paths = findProjects({ home() / "PycharmProjects", target / utf8Path("Боты") });
    int kbzh = 0;
    for (const fs::path& p : paths)
        kbzh += p.filename() == "kbzh";
    CHECK(kbzh == 1);
}

TEST(removeLinksToDeletedProject)
{
    auto all = scanAll();
    std::vector<Project> list;
    for (auto& [n, p] : all)
        list.push_back(p);
    const fs::path target = home() / "Projects2";
    organize(list, target);
    const fs::path project = home() / "PycharmProjects/knb";
    CHECK(fs::is_symlink(target / utf8Path("Боты") / "knb"));
    // The project goes away first (Trash), then its links.
    fs::rename(project, g_root / "trashed_knb");
    CHECK(removeLinksTo(target, project) == 1);
    CHECK(!fs::exists(fs::symlink_status(target / utf8Path("Боты") / "knb")));
    CHECK(fs::exists(g_root / "trashed_knb/main.py"));   // only the link was removed
    CHECK(fs::is_symlink(target / utf8Path("Боты") / "PythonProject2"));
    fs::rename(g_root / "trashed_knb", project);
}

TEST(nameClashesGetSuffixes)
{
    Project a;
    a.name = "bot";
    a.path = "/x/bot";
    a.category = Category::Bot;
    Project b = a;
    b.path = "/y/bot";
    const auto links = plan({ a, b }, "/p");
    CHECK(links.size() == 2);
    CHECK(links[0].link.filename() == "bot");
    CHECK(links[1].link.filename() == "bot (2)");
}

TEST(categoryKeysRoundTrip)
{
    for (int i = 0; i < kCategoryCount; ++i) {
        Category c;
        CHECK(categoryFromKey(categoryKey(Category(i)), &c) && c == Category(i));
    }
    Category c;
    CHECK(!categoryFromKey("nope", &c));
}

int main()
{
    g_root = fs::temp_directory_path() / ("hangar_test_" + std::to_string(std::rand() ^ int(std::time(nullptr))));
    fs::remove_all(g_root);
    buildTree();
    for (const Test& t : registry()) {
        const int before = g_failed;
        t.fn();
        std::printf("%s %s\n", g_failed == before ? "PASS" : "FAIL", t.name);
    }
    fs::remove_all(g_root);
    std::printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
