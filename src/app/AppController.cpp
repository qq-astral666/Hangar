#include "AppController.h"

#include "Organizer.h"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QSettings>
#include <QtConcurrent/QtConcurrentRun>

namespace {

hangar::fs::path toPath(const QString& s)
{
    return hangar::utf8Path(s.toStdString());
}

QString fromPath(const hangar::fs::path& p)
{
    const std::u8string u = p.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(u.data()), qsizetype(u.size()));
}

const QStringList kKnownEditors {
    QStringLiteral("CLion"),
    QStringLiteral("PyCharm"),
    QStringLiteral("PyCharm CE"),
    QStringLiteral("PyCharm Community Edition"),
    QStringLiteral("WebStorm"),
    QStringLiteral("Visual Studio Code"),
    QStringLiteral("Cursor"),
    QStringLiteral("Zed"),
    QStringLiteral("Xcode"),
};

const QString kRootsKey = QStringLiteral("roots");
const QString kExtraKey = QStringLiteral("extraProjects");
const QString kOverridesKey = QStringLiteral("overrides");
const QString kOrganizeKey = QStringLiteral("organizeRoot");
const QString kHiddenKey = QStringLiteral("hidden");

// std::filesystem, not QFileInfo: the same code found the project, so the
// answer is consistent with the scan (Unicode names, symlinks...).
bool folderExists(const QString& path)
{
    std::error_code ec;
    return hangar::fs::is_directory(hangar::utf8Path(path.toStdString()), ec);
}

} // namespace

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(1200);
    connect(&m_debounce, &QTimer::timeout, this, [this] { refresh(false); });
    // Finder writes .DS_Store, IDEs touch files: only a folder appearing,
    // disappearing or being renamed is worth a rescan.
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& dir) {
        const QStringList now = listing(dir);
        if (now == m_listings.value(dir))
            return;
        m_listings.insert(dir, now);
        m_debounce.start();
    });

    connect(&m_scan, &QFutureWatcher<ScanResult>::finished, this, [this] {
        applyOverridesAndPublish(m_scan.result().projects);
        const bool wasVisible = scanning();
        m_scanning = false;
        m_scanVisible = false;
        if (wasVisible)
            emit scanningChanged();
        m_sinceScan.start();
        if (m_rescanPending) {
            m_rescanPending = false;
            refresh(false);
        }
    });

    // Coming back to the app: files may have changed in the IDE meanwhile.
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && m_sinceScan.isValid() && m_sinceScan.elapsed() > 60000)
            refresh(false);
    });
}

void AppController::start()
{
    load();
    detectEditors();
    watchRoots();
    rescan();
}

void AppController::load()
{
    QSettings s;
    if (s.contains(kRootsKey)) {
        m_roots = s.value(kRootsKey).toStringList();
    } else {
        // First launch: the usual places IDEs put projects.
        for (const char* name : { "CLionProjects", "PycharmProjects", "WebstormProjects", "IdeaProjects",
                                  "Developer", "Projects-src", "src", "code", "dev" }) {
            const QString path = QDir::home().filePath(QString::fromLatin1(name));
            if (QFileInfo(path).isDir())
                m_roots << path;
        }
    }
    m_extraProjects = s.value(kExtraKey).toStringList();
    m_overrides = s.value(kOverridesKey).toMap();
    m_hidden = s.value(kHiddenKey).toStringList();
    emit hiddenChanged();
    m_organizeRoot = s.value(kOrganizeKey, QDir::home().filePath(QStringLiteral("Projects"))).toString();
    emit rootsChanged();
    emit organizeRootChanged();
}

void AppController::saveRoots()
{
    QSettings s;
    s.setValue(kRootsKey, m_roots);
    s.setValue(kExtraKey, m_extraProjects);
    emit rootsChanged();
    watchRoots();
}

void AppController::detectEditors()
{
    QStringList installed;
    const QStringList dirs { QStringLiteral("/Applications"), QDir::home().filePath(QStringLiteral("Applications")) };
    for (const QString& editor : kKnownEditors) {
        for (const QString& dir : dirs) {
            if (QFileInfo::exists(dir + QLatin1Char('/') + editor + QStringLiteral(".app"))) {
                installed << editor;
                break;
            }
        }
    }
    // Community editions answer to the "PyCharm" suggestion.
    if (!installed.contains(QStringLiteral("PyCharm"))
        && (installed.contains(QStringLiteral("PyCharm CE")) || installed.contains(QStringLiteral("PyCharm Community Edition"))))
        installed << QStringLiteral("PyCharm");
    m_model.setInstalledEditors(installed);
}

void AppController::watchRoots()
{
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    m_listings.clear();
    for (const QString& r : m_roots) {
        if (QFileInfo(r).isDir()) {
            m_watcher.addPath(r);
            m_listings.insert(r, listing(r));
        }
    }
}

QStringList AppController::listing(const QString& dir) const
{
    QStringList names = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    names.removeIf([](const QString& n) { return n.startsWith(QLatin1Char('.')); });
    return names;
}

void AppController::rescan()
{
    refresh(true);
}

void AppController::refresh(bool visible)
{
    if (m_scanning) {
        if (visible && !m_scanVisible) {
            m_scanVisible = true;
            emit scanningChanged();
        }
        m_rescanPending = true;
        return;
    }
    m_scanning = true;
    m_scanVisible = visible;
    if (visible)
        emit scanningChanged();

    std::vector<hangar::fs::path> roots, extra;
    for (const QString& r : m_roots)
        roots.push_back(toPath(r));
    for (const QString& e : m_extraProjects)
        extra.push_back(toPath(e));

    m_scan.setFuture(QtConcurrent::run([roots, extra] {
        ScanResult result;
        for (const hangar::fs::path& p : hangar::findProjects(roots, extra))
            result.projects.push_back(hangar::inspect(p));
        return result;
    }));
}

void AppController::applyOverridesAndPublish(std::vector<hangar::Project> projects)
{
    std::erase_if(projects, [this](const hangar::Project& p) { return m_hidden.contains(fromPath(p.path)); });
    QList<bool> overridden;
    for (hangar::Project& p : projects) {
        const QString key = m_overrides.value(fromPath(p.path)).toString();
        hangar::Category c;
        const bool has = !key.isEmpty() && hangar::categoryFromKey(key.toStdString(), &c);
        if (has)
            p.category = c;
        overridden << has;
    }
    m_model.setProjects(std::move(projects), overridden);
}

void AppController::addUrls(const QList<QUrl>& urls)
{
    int roots = 0, projects = 0;
    for (const QUrl& url : urls) {
        const QString path = QDir::cleanPath(url.isLocalFile() ? url.toLocalFile() : url.toString());
        if (!QFileInfo(path).isDir())
            continue;
        const hangar::fs::path p = toPath(path);
        // Dropping a hidden project brings it back.
        if (m_hidden.removeAll(path) > 0) {
            QSettings().setValue(kHiddenKey, m_hidden);
            emit hiddenChanged();
            ++projects;
        }
        // A folder with projects inside is a root; a project on its own is an extra project.
        bool hasProjectChildren = false;
        std::error_code ec;
        for (hangar::fs::directory_iterator it(p, ec), end; it != end && !ec && !hasProjectChildren; it.increment(ec))
            hasProjectChildren = hangar::looksLikeProject(it->path());
        if (hasProjectChildren) {
            if (!m_roots.contains(path)) {
                m_roots << path;
                ++roots;
            }
        } else if (hangar::looksLikeProject(p)) {
            if (!m_extraProjects.contains(path)) {
                m_extraProjects << path;
                ++projects;
            }
        }
    }
    if (roots + projects == 0) {
        emit toast(tr("Там не нашлось проектов"));
        return;
    }
    saveRoots();
    rescan();
    if (roots && projects)
        emit toast(tr("Добавлено: папок %1, проектов %2").arg(roots).arg(projects));
    else if (roots)
        emit toast(roots == 1 ? tr("Папка добавлена") : tr("Добавлено папок: %1").arg(roots));
    else
        emit toast(projects == 1 ? tr("Проект добавлен") : tr("Добавлено проектов: %1").arg(projects));
}

void AppController::removeRoot(const QString& root)
{
    if (m_roots.removeAll(root) + m_extraProjects.removeAll(root) == 0)
        return;
    saveRoots();
    rescan();
}

void AppController::setCategory(const QString& path, int category)
{
    if (category < 0 || category >= hangar::kCategoryCount)
        m_overrides.remove(path);
    else
        m_overrides.insert(path, QString::fromLatin1(hangar::categoryKey(hangar::Category(category))));
    QSettings().setValue(kOverridesKey, m_overrides);

    // Re-publish without rescanning the disk.
    std::vector<hangar::Project> projects = m_model.projects();
    for (hangar::Project& p : projects)
        if (fromPath(p.path) == path && category < 0)
            p = hangar::inspect(p.path);   // back to the automatic guess
    applyOverridesAndPublish(std::move(projects));
}

void AppController::openIn(const QString& path, const QString& editor)
{
    QString app = editor;
    if (app == QLatin1String("PyCharm")) {
        // Whichever PyCharm is installed.
        for (const QString& candidate : { QStringLiteral("PyCharm"), QStringLiteral("PyCharm CE"),
                                          QStringLiteral("PyCharm Community Edition") }) {
            if (QFileInfo::exists(QStringLiteral("/Applications/") + candidate + QStringLiteral(".app"))
                || QFileInfo::exists(QDir::home().filePath(QStringLiteral("Applications/") + candidate + QStringLiteral(".app")))) {
                app = candidate;
                break;
            }
        }
    }
    if (app.isEmpty()) {
        reveal(path);
        return;
    }
    if (!QProcess::startDetached(QStringLiteral("/usr/bin/open"), { QStringLiteral("-a"), app, path }))
        emit toast(tr("Не удалось открыть %1").arg(app));
}

void AppController::reveal(const QString& path)
{
    QProcess::startDetached(QStringLiteral("/usr/bin/open"), { QStringLiteral("-R"), path });
}

void AppController::openTerminal(const QString& path)
{
    QProcess::startDetached(QStringLiteral("/usr/bin/open"), { QStringLiteral("-a"), QStringLiteral("Terminal"), path });
}

void AppController::openUrl(const QString& url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void AppController::copyPath(const QString& path)
{
    QGuiApplication::clipboard()->setText(path);
    emit toast(tr("Путь скопирован"));
}

void AppController::trash(const QString& path)
{
    const QString name = QFileInfo(path).fileName();
    if (!folderExists(path)) {
        // Already gone (deleted in Finder, moved...): just clean up the list.
        dropFromList(path);
        emit toast(tr("Папки «%1» уже не было, убрал из списка").arg(name));
        return;
    }
    if (!QFile::moveToTrash(path)) {
        emit toast(tr("Не удалось переместить «%1» в Корзину").arg(name));
        return;
    }
    dropFromList(path);
    emit toast(tr("«%1» в Корзине. Вернуть можно из Finder").arg(name));
}

void AppController::forget(const QString& path)
{
    const QString name = QFileInfo(path).fileName();
    const bool exists = folderExists(path);
    // An explicitly added project is simply un-added; one found in a watched
    // folder would come back on the next scan, so it's remembered as hidden.
    if (exists && !m_extraProjects.contains(path) && !m_hidden.contains(path)) {
        m_hidden << path;
        QSettings().setValue(kHiddenKey, m_hidden);
        emit hiddenChanged();
    }
    dropFromList(path);
    emit toast(exists ? tr("«%1» убран из списка, папка на месте").arg(name)
                      : tr("«%1» убран из списка").arg(name));
}

void AppController::showHidden()
{
    if (m_hidden.isEmpty())
        return;
    const int n = int(m_hidden.size());
    m_hidden.clear();
    QSettings().setValue(kHiddenKey, m_hidden);
    emit hiddenChanged();
    refresh(false);
    emit toast(tr("Вернул в список: %1").arg(n));
}

void AppController::dropFromList(const QString& path)
{
    const bool wasRoot = m_roots.removeAll(path) > 0;
    const bool wasExtra = m_extraProjects.removeAll(path) > 0;
    if (wasRoot || wasExtra)
        saveRoots();
    if (m_overrides.remove(path) > 0)
        QSettings().setValue(kOverridesKey, m_overrides);
    hangar::removeLinksTo(toPath(m_organizeRoot), toPath(path));

    // Gone from the list right away; a rescan confirms.
    std::vector<hangar::Project> projects = m_model.projects();
    std::erase_if(projects, [&](const hangar::Project& p) { return fromPath(p.path) == path; });
    applyOverridesAndPublish(std::move(projects));
    refresh(false);
}

QString AppController::organizeRootDisplay() const
{
    QString path = m_organizeRoot;
    const QString home = QDir::homePath();
    if (path.startsWith(home))
        path = QStringLiteral("~") + path.mid(home.size());
    return path;
}

void AppController::setOrganizeRoot(const QString& path)
{
    QString clean = QDir::cleanPath(path.startsWith(QLatin1String("file:")) ? QUrl(path).toLocalFile() : path);
    if (clean == m_organizeRoot || clean.isEmpty())
        return;
    // The organised tree must not be one of the watched roots (links would
    // be scanned as projects again, harmless but confusing).
    m_organizeRoot = clean;
    QSettings().setValue(kOrganizeKey, clean);
    emit organizeRootChanged();
}

void AppController::organize()
{
    const hangar::OrganizeResult r = hangar::organize(m_model.projects(), toPath(m_organizeRoot));
    if (!r.errors.empty()) {
        qWarning() << "Hangar: organize errors:";
        for (const std::string& e : r.errors)
            qWarning() << "  " << QString::fromStdString(e);
    }
    QString message;
    if (r.created == 0 && r.removed == 0)
        message = tr("Всё уже разложено");
    else
        message = tr("Готово: новых ярлыков %1, убрано старых %2").arg(r.created).arg(r.removed);
    if (!r.errors.empty())
        message += tr(" · ошибок: %1").arg(r.errors.size());
    emit toast(message);
}

void AppController::revealOrganized()
{
    QDir().mkpath(m_organizeRoot);
    QProcess::startDetached(QStringLiteral("/usr/bin/open"), { m_organizeRoot });
}
