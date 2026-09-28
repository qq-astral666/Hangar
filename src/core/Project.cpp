#include "Project.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace hangar {

namespace {

struct CategoryInfo {
    const char* folder;
    const char* key;
};

constexpr std::array<CategoryInfo, kCategoryCount> kCategories { {
    { "Мини-аппы", "miniapp" },
    { "Боты", "bot" },
    { "Сайты", "site" },
    { "Приложения", "desktop" },
    { "Игры", "game" },
    { "Скрипты", "script" },
    { "Учёба", "study" },
    { "Архив", "archive" },
} };

// Never descended into: dependencies, environments, build output, IDE data.
bool isHeavyDir(const std::string& name)
{
    static const std::set<std::string> kSkip {
        "node_modules", ".venv", "venv", "env", ".env", ".git", ".idea", ".vscode", "__pycache__",
        "dist", "build", "out", "target", ".next", ".cache", "Pods", "DerivedData", ".gradle",
    };
    return kSkip.count(name) > 0 || name.rfind("cmake-build-", 0) == 0 || name.rfind("build-", 0) == 0;
}

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

std::string extensionOf(const fs::path& p)
{
    return lower(p.extension().string());
}

bool isSourceExt(const std::string& ext)
{
    static const std::set<std::string> kSource {
        ".py", ".cpp", ".cc", ".cxx", ".c", ".h", ".hpp", ".mm", ".m", ".swift", ".js", ".jsx", ".ts", ".tsx",
        ".mjs", ".html", ".htm", ".css", ".scss", ".vue", ".svelte", ".go", ".rs", ".java", ".kt", ".cs", ".php",
        ".rb", ".lua", ".qml", ".dart",
    };
    return kSource.count(ext) > 0;
}

bool isMarkerFile(const std::string& name)
{
    static const std::set<std::string> kMarkers {
        "CMakeLists.txt", "package.json", "requirements.txt", "pyproject.toml", "setup.py", "Pipfile",
        "Cargo.toml", "go.mod", "Dockerfile", "docker-compose.yml", "Makefile", "manage.py", "pom.xml",
        "build.gradle", "Package.swift", "pubspec.yaml", "composer.json", "Gemfile",
    };
    return kMarkers.count(name) > 0;
}

// Files worth reading for signals.
bool isReadable(const std::string& name, const std::string& ext)
{
    return isSourceExt(ext) || name == "CMakeLists.txt" || name == "package.json" || name == "requirements.txt"
        || name == "pyproject.toml" || name == "Pipfile" || name == "Dockerfile";
}

std::int64_t toUnix(fs::file_time_type t)
{
    using namespace std::chrono;
    const auto sys = time_point_cast<seconds>(t - fs::file_time_type::clock::now() + system_clock::now());
    return sys.time_since_epoch().count();
}

std::string readHead(const fs::path& p, std::size_t limit)
{
    std::ifstream in(p, std::ios::binary);
    if (!in)
        return {};
    std::string s(limit, '\0');
    in.read(s.data(), std::streamsize(limit));
    s.resize(std::size_t(in.gcount()));
    return s;
}

// What we learned from the files.
struct Evidence {
    std::set<std::string> names;       // file names seen (top level + one level down)
    std::set<std::string> topNames;    // top level only
    int py = 0, cpp = 0, js = 0, html = 0, other = 0;
    std::string text;                  // concatenated heads of readable files (lower-case)
    std::string webAppFile;            // where the Telegram WebApp signal was found
    std::set<std::string> pyModules;   // imported top-level modules + declared dependencies
    std::int64_t newest = 0;
    bool hasGit = false;
    bool hasIdea = false;
};

bool isNameChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
}

// "import a, b.c as d" / "from x.y import z" -> a, b / x. Plain string work:
// std::regex in libc++ (macOS) is slow and deeply recursive on megabytes of
// text and could stall the whole scan.
void collectPython(const std::string& text, std::set<std::string>& out)
{
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::size_t i = line.find_first_not_of(" \t");
        if (i == std::string::npos)
            continue;
        const bool isImport = line.compare(i, 7, "import ") == 0;
        const bool isFrom = line.compare(i, 5, "from ") == 0;
        if (!isImport && !isFrom)
            continue;
        i += isImport ? 7 : 5;
        while (i < line.size()) {
            while (i < line.size() && (line[i] == ' ' || line[i] == ',' || line[i] == '('))
                ++i;
            std::size_t j = i;
            while (j < line.size() && (std::isalnum(static_cast<unsigned char>(line[j])) || line[j] == '_'))
                ++j;
            if (j > i)
                out.insert(line.substr(i, j - i));
            if (isFrom)
                break;
            // next name after a comma; skip ".sub" and "as alias"
            const std::size_t comma = line.find(',', j);
            if (comma == std::string::npos)
                break;
            i = comma + 1;
        }
    }
}

// requirements.txt / pyproject.toml / Pipfile: leading package name of each line.
void collectRequirements(const std::string& text, std::set<std::string>& out)
{
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::size_t i = line.find_first_not_of(" \t\"'");
        if (i == std::string::npos || line[i] == '#' || line[i] == '[')
            continue;
        std::size_t j = i;
        while (j < line.size() && isNameChar(line[j]))
            ++j;
        if (j > i)
            out.insert(line.substr(i, j - i));
    }
}

void scanDir(const fs::path& dir, const fs::path& root, int depth, Evidence& ev, int& budget)
{
    std::error_code ec;
    for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end; it != end && !ec;
         it.increment(ec)) {
        const fs::directory_entry& e = *it;
        const std::string name = e.path().filename().string();
        std::error_code ec2;
        if (e.is_directory(ec2)) {
            if (depth == 0 && name == ".git")
                ev.hasGit = true;
            if (depth == 0 && name == ".idea")
                ev.hasIdea = true;
            if (depth < 2 && !isHeavyDir(name) && (name.empty() || name[0] != '.'))
                scanDir(e.path(), root, depth + 1, ev, budget);
            continue;
        }
        if (!e.is_regular_file(ec2))
            continue;
        const std::string ext = extensionOf(e.path());
        ev.names.insert(name);
        if (depth == 0)
            ev.topNames.insert(name);
        if (ext == ".py") ++ev.py;
        else if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".h" || ext == ".hpp" || ext == ".c") ++ev.cpp;
        else if (ext == ".js" || ext == ".jsx" || ext == ".ts" || ext == ".tsx" || ext == ".mjs" || ext == ".vue") ++ev.js;
        else if (ext == ".html" || ext == ".htm") ++ev.html;
        else if (isSourceExt(ext)) ++ev.other;

        if (isSourceExt(ext) || isMarkerFile(name)) {
            const auto t = e.last_write_time(ec2);
            if (!ec2)
                ev.newest = std::max(ev.newest, toUnix(t));
        }
        if (budget > 0 && isReadable(name, ext)) {
            --budget;
            std::error_code ec3;
            const auto size = e.file_size(ec3);
            // Minified bundles and huge generated files carry no useful signals.
            if (ec3 || size > 1024 * 1024 || name.find(".min.") != std::string::npos)
                continue;
            const std::string head = lower(readHead(e.path(), 24 * 1024));
            if (ext == ".py")
                collectPython(head, ev.pyModules);
            else if (name == "requirements.txt" || name == "pyproject.toml" || name == "Pipfile")
                collectRequirements(head, ev.pyModules);
            if (ev.webAppFile.empty()
                && (head.find("telegram-web-app.js") != std::string::npos
                    || head.find("telegram.webapp") != std::string::npos || head.find("@twa-dev") != std::string::npos
                    || head.find("@telegram-apps") != std::string::npos
                    || head.find("webappinfo") != std::string::npos || head.find("web_app=") != std::string::npos)) {
                ev.webAppFile = fs::relative(e.path(), root, ec2).generic_string();
            }
            ev.text += head;
            ev.text += '\n';
        }
    }
}

bool has(const Evidence& ev, const char* needle)
{
    return ev.text.find(needle) != std::string::npos;
}

// A Python import (`import x`, `from x ...`) or a dependency line.
bool usesPy(const Evidence& ev, const std::string& module)
{
    return ev.pyModules.count(module) > 0;
}

// An npm dependency or an import from it.
bool usesJs(const Evidence& ev, const std::string& pkg)
{
    return has(ev, ("\"" + pkg + "\"").c_str()) || has(ev, ("'" + pkg + "'").c_str())
        || has(ev, ("from \"" + pkg + "/").c_str()) || has(ev, ("from '" + pkg + "/").c_str());
}

bool nameMatches(const std::string& name, const char* pattern)
{
    return std::regex_search(lower(name), std::regex(pattern));
}

void readGit(const fs::path& dir, Project& p)
{
    const fs::path git = dir / ".git";
    std::error_code ec;
    if (!fs::is_directory(git, ec))
        return;
    p.hasGit = true;

    const std::string head = readHead(git / "HEAD", 512);
    const std::string refPrefix = "ref: refs/heads/";
    if (head.rfind(refPrefix, 0) == 0) {
        p.branch = head.substr(refPrefix.size());
        p.branch.erase(p.branch.find_last_not_of(" \r\n\t") + 1);
    } else if (head.size() >= 7) {
        p.branch = head.substr(0, 7);   // detached
    }

    // Last line of the reflog: "<old> <new> Name <mail> <unix> <tz>\t<message>"
    std::ifstream log(git / "logs" / "HEAD");
    std::string line, last;
    while (std::getline(log, line))
        if (!line.empty())
            last = line;
    const std::size_t mailEnd = last.find("> ");
    if (mailEnd != std::string::npos) {
        std::istringstream rest(last.substr(mailEnd + 2));
        std::int64_t ts = 0;
        if (rest >> ts)
            p.lastCommit = ts;
    }

    // [remote "origin"] url = ...
    const std::string config = readHead(git / "config", 16 * 1024);
    const std::size_t origin = config.find("[remote \"origin\"]");
    if (origin != std::string::npos) {
        std::smatch m;
        const std::string tail = config.substr(origin);
        if (std::regex_search(tail, m, std::regex("url\\s*=\\s*(\\S+)"))) {
            std::string url = m[1];
            // git@github.com:user/repo.git -> https://github.com/user/repo
            std::smatch s;
            if (std::regex_match(url, s, std::regex("git@([^:]+):(.+)")))
                url = "https://" + s[1].str() + "/" + s[2].str();
            if (url.size() > 4 && url.compare(url.size() - 4, 4, ".git") == 0)
                url.resize(url.size() - 4);
            if (url.rfind("https://", 0) == 0)
                p.remoteUrl = url;
        }
    }
}

std::string pagesWord(int n)
{
    const int m10 = n % 10, m100 = n % 100;
    if (m10 == 1 && m100 != 11)
        return "страница";
    if (m10 >= 2 && m10 <= 4 && (m100 < 10 || m100 >= 20))
        return "страницы";
    return "страниц";
}

void addTag(Project& p, const std::string& tag)
{
    if (std::find(p.tags.begin(), p.tags.end(), tag) == p.tags.end())
        p.tags.push_back(tag);
}

} // namespace

const char* folderName(Category c)
{
    return kCategories[std::size_t(c)].folder;
}

const char* categoryKey(Category c)
{
    return kCategories[std::size_t(c)].key;
}

bool categoryFromKey(const std::string& key, Category* out)
{
    for (int i = 0; i < kCategoryCount; ++i) {
        if (key == kCategories[std::size_t(i)].key) {
            *out = Category(i);
            return true;
        }
    }
    return false;
}

bool looksLikeProject(const fs::path& dir)
{
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
        return false;
    const std::string name = dir.filename().string();
    if (name.empty() || name[0] == '.' || isHeavyDir(name))
        return false;
    for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end; it != end && !ec;
         it.increment(ec)) {
        const std::string n = it->path().filename().string();
        if (n == ".git" || n == ".idea" || n == ".venv" || isMarkerFile(n))
            return true;
        std::error_code ec2;
        if (it->is_regular_file(ec2) && isSourceExt(extensionOf(it->path())))
            return true;
    }
    return false;
}

std::vector<fs::path> findProjects(const std::vector<fs::path>& roots, const std::vector<fs::path>& extra)
{
    std::vector<fs::path> result;
    std::set<fs::path> seen;
    auto add = [&](const fs::path& p) {
        std::error_code ec;
        const fs::path real = fs::weakly_canonical(p, ec);
        if (ec || !seen.insert(real).second)
            return;
        result.push_back(p);
    };

    for (const fs::path& root : roots) {
        std::error_code ec;
        std::vector<fs::path> children;
        for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
             it != end && !ec; it.increment(ec)) {
            std::error_code ec2;
            if (it->is_directory(ec2) && looksLikeProject(it->path()))
                children.push_back(it->path());
        }
        std::sort(children.begin(), children.end());
        if (children.empty()) {
            if (looksLikeProject(root))
                add(root);
            continue;
        }
        for (const fs::path& c : children)
            add(c);
    }
    for (const fs::path& p : extra)
        if (looksLikeProject(p))
            add(p);
    return result;
}

Project inspect(const fs::path& dir)
{
    Project p;
    p.path = dir;
    p.name = dir.filename().string();

    Evidence ev;
    int budget = 80;
    scanDir(dir, dir, 0, ev, budget);
    p.modified = ev.newest;
    readGit(dir, p);

    // ---- languages
    std::error_code ignored;
    const bool cmake = ev.topNames.count("CMakeLists.txt") > 0;
    const bool python = ev.py > 0 || ev.names.count("requirements.txt") || ev.names.count("pyproject.toml");
    // A .venv alone (PyCharm makes one for any project) only hints the editor.
    const bool pyEnv = fs::is_directory(dir / ".venv", ignored) || fs::is_directory(dir / "venv", ignored);
    const bool node = ev.names.count("package.json") > 0;
    if (python) addTag(p, "Python");
    if (ev.cpp > 0 || cmake) addTag(p, "C++");
    if (ev.js > 0 || node) addTag(p, ev.names.count("tsconfig.json") ? "TypeScript" : "JavaScript");
    if (ev.html > 0 && !node) addTag(p, "HTML/CSS");

    // ---- frameworks
    const bool aiogram = usesPy(ev, "aiogram");
    const bool telebot = usesPy(ev, "telebot") || has(ev, "pytelegrambotapi");
    const bool ptb = has(ev, "telegram.ext") || has(ev, "python-telegram-bot");
    const bool nodeBot = (node || ev.js > 0)
        && (usesJs(ev, "node-telegram-bot-api") || usesJs(ev, "telegraf") || usesJs(ev, "grammy"));
    const bool tgBot = aiogram || telebot || ptb || nodeBot;
    if (aiogram) addTag(p, "aiogram");
    if (telebot) addTag(p, "pyTelegramBotAPI");
    if (ptb) addTag(p, "python-telegram-bot");
    if (usesJs(ev, "node-telegram-bot-api")) addTag(p, "node-telegram-bot-api");
    if (usesJs(ev, "telegraf")) addTag(p, "Telegraf");
    if (usesJs(ev, "grammy")) addTag(p, "grammY");

    const bool fastapi = usesPy(ev, "fastapi");
    const bool flask = usesPy(ev, "flask");
    const bool django = usesPy(ev, "django") || ev.names.count("manage.py");
    const bool express = node && usesJs(ev, "express");
    if (fastapi) addTag(p, "FastAPI");
    if (flask) addTag(p, "Flask");
    if (django) addTag(p, "Django");
    if (express) addTag(p, "Express");
    const bool backend = fastapi || flask || django || express;
    if (usesPy(ev, "sqlalchemy")) addTag(p, "SQLAlchemy");
    if (ev.names.count("Dockerfile")) addTag(p, "Docker");

    // JS frameworks only count in projects that actually have JS: a Qt app's
    // QML can mention "next" or "react" too.
    const bool jsProject = node || ev.js > 0;
    const bool react = jsProject && usesJs(ev, "react");
    const bool vue = jsProject && usesJs(ev, "vue");
    const bool vite = jsProject && (usesJs(ev, "vite") || ev.names.count("vite.config.js") || ev.names.count("vite.config.ts"));
    const bool next = node && has(ev, "\"next\":");
    if (react) addTag(p, "React");
    if (vue) addTag(p, "Vue");
    if (next) addTag(p, "Next.js");
    if (vite) addTag(p, "Vite");
    const bool web = ev.html > 0 || react || vue || next || vite;

    const bool qt = cmake && (has(ev, "find_package(qt") || has(ev, "qt6::"));
    const bool raylib = has(ev, "raylib");
    const bool sdl = has(ev, "sdl2") || has(ev, "sdl3") || has(ev, "#include <sdl");
    const bool pygame = usesPy(ev, "pygame");
    const bool arcade = usesPy(ev, "arcade");
    const bool flet = usesPy(ev, "flet");
    const bool tkinter = usesPy(ev, "tkinter") || usesPy(ev, "customtkinter");
    const bool pyqt = usesPy(ev, "pyqt5") || usesPy(ev, "pyqt6") || usesPy(ev, "pyside6");
    const bool electron = usesJs(ev, "electron");
    if (qt) addTag(p, "Qt");
    if (raylib) addTag(p, "raylib");
    if (sdl) addTag(p, "SDL");
    if (pygame) addTag(p, "pygame");
    if (flet) addTag(p, "Flet");
    if (tkinter) addTag(p, "Tkinter");
    if (pyqt) addTag(p, "PyQt");
    if (electron) addTag(p, "Electron");
    const bool game = raylib || sdl || pygame || arcade;
    const bool desktop = qt || flet || tkinter || pyqt || electron;

    const bool webApp = !ev.webAppFile.empty();
    if (webApp) addTag(p, "Telegram Mini App");

    const int sources = ev.py + ev.cpp + ev.js + ev.html + ev.other;

    // ---- category, strongest signal first
    if (nameMatches(p.name, "(копия|copy|backup|бэкап|бекап|_old\\b|\\bold_|-old\\b|\\(\\d+\\)$)")) {
        p.category = Category::Archive;
        p.reason = "Похоже на копию или бэкап";
    } else if (sources == 0) {
        p.category = Category::Study;
        p.reason = "Пустой проект";
    } else if (webApp && (tgBot || backend || web)) {
        p.category = Category::MiniApp;
        p.reason = "Telegram WebApp в " + ev.webAppFile;
    } else if (tgBot && (backend || web)) {
        p.category = Category::MiniApp;
        p.reason = std::string("Бот + ") + (backend ? "бэкенд" : "веб-страницы");
    } else if (tgBot) {
        p.category = Category::Bot;
        p.reason = "Telegram-бот (" + p.tags.back() + ")";
        for (const char* lib : { "aiogram", "pyTelegramBotAPI", "python-telegram-bot", "node-telegram-bot-api", "Telegraf", "grammY" })
            if (std::find(p.tags.begin(), p.tags.end(), lib) != p.tags.end()) {
                p.reason = std::string("Telegram-бот на ") + lib;
                break;
            }
    } else if (game) {
        p.category = Category::Game;
        p.reason = std::string("Игра: ") + (raylib ? "raylib" : sdl ? "SDL" : pygame ? "pygame" : "arcade");
    } else if (desktop) {
        p.category = Category::Desktop;
        p.reason = std::string("Приложение на ") + (qt ? "Qt" : flet ? "Flet" : tkinter ? "Tkinter" : pyqt ? "PyQt" : "Electron");
    } else if (nameMatches(p.name, "^(pythonproject\\d*|untitled\\d*|welcomescreen|test\\d*|sandbox|practice|hello.*|first.*|homework.*)$"
                           "|(^|[^a-z])\\d*(laba?|mdk)\\d*([^a-z]|$)|лаб|мдк|домашк")) {
        p.category = Category::Study;
        p.reason = "Учебный или пробный проект";
    } else if (web || backend) {
        p.category = Category::Website;
        p.reason = react ? "Сайт на React" : vue ? "Сайт на Vue" : next ? "Сайт на Next.js"
            : backend && !web ? "Веб-бэкенд" : "Сайт, " + std::to_string(ev.html) + " " + pagesWord(ev.html);
        if (ev.html == 1 && !react && !vue && !next && !backend)
            p.reason = "Одностраничный сайт";
    } else {
        p.category = Category::Script;
        p.reason = python ? "Python-скрипт" : cmake || ev.cpp > 0 ? "Консольная программа на C++" : "Код без фреймворков";
    }

    // ---- editors
    if (cmake || ev.cpp > 0)
        p.editors.push_back("CLion");
    if (python)
        p.editors.push_back("PyCharm");
    if (web || node) {
        p.editors.push_back("Visual Studio Code");
        p.editors.push_back("WebStorm");
        if (!python)
            p.editors.push_back("PyCharm");   // it opens HTML fine, and many people have only it
    }
    if (pyEnv)
        p.editors.insert(p.editors.begin(), "PyCharm");   // created in PyCharm: open it there
    else if (p.editors.empty() && ev.hasIdea)
        p.editors.push_back("CLion");
    p.editors.push_back("Visual Studio Code");
    std::vector<std::string> unique;
    for (const std::string& e : p.editors)
        if (std::find(unique.begin(), unique.end(), e) == unique.end())
            unique.push_back(e);
    p.editors = unique;
    return p;
}

} // namespace hangar
