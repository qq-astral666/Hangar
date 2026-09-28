#include "ProjectModel.h"

#include <QDateTime>
#include <QDir>

#include <algorithm>

namespace {

QString qstr(const std::string& s)
{
    return QString::fromStdString(s);
}

QString qpath(const hangar::fs::path& p)
{
    const std::u8string u = p.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(u.data()), qsizetype(u.size()));
}

QDateTime fromUnix(std::int64_t t)
{
    return t > 0 ? QDateTime::fromSecsSinceEpoch(t) : QDateTime();
}

} // namespace

ProjectModel::ProjectModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

namespace {

bool sameProject(const hangar::Project& a, const hangar::Project& b)
{
    return a.name == b.name && a.category == b.category && a.tags == b.tags && a.reason == b.reason
        && a.modified == b.modified && a.hasGit == b.hasGit && a.branch == b.branch
        && a.lastCommit == b.lastCommit && a.remoteUrl == b.remoteUrl && a.editors == b.editors;
}

} // namespace

bool ProjectModel::updateInPlace(std::vector<hangar::Project>& projects, const QList<bool>& overridden)
{
    if (projects.size() != m_all.size())
        return false;
    QHash<QString, int> index;   // path -> position in the new list
    for (int i = 0; i < int(projects.size()); ++i)
        index.insert(qpath(projects[std::size_t(i)].path), i);
    if (index.size() != int(m_all.size()))
        return false;
    for (const hangar::Project& p : m_all)
        if (!index.contains(qpath(p.path)))
            return false;

    // Same projects: keep the order on screen, change only what changed.
    bool categoriesChanged = false;
    std::vector<int> changed;
    for (int i = 0; i < int(m_all.size()); ++i) {
        const int j = index.value(qpath(m_all[std::size_t(i)].path));
        hangar::Project& fresh = projects[std::size_t(j)];
        const bool ov = j < overridden.size() && overridden[j];
        if (sameProject(m_all[std::size_t(i)], fresh) && m_overridden.value(i) == ov)
            continue;
        categoriesChanged |= m_all[std::size_t(i)].category != fresh.category;
        m_all[std::size_t(i)] = std::move(fresh);
        m_overridden[i] = ov;
        changed.push_back(i);
    }
    if (changed.empty())
        return true;   // nothing to do: no repaint, no scroll jump

    if (categoriesChanged && (m_category >= 0 || !m_search.isEmpty())) {
        refilter();    // a card may have to appear or disappear
    } else {
        for (int i : changed) {
            const auto it = std::find(m_visible.begin(), m_visible.end(), i);
            if (it != m_visible.end()) {
                const QModelIndex idx = this->index(int(it - m_visible.begin()));
                emit dataChanged(idx, idx);
            }
        }
    }
    if (categoriesChanged)
        emit countsChanged();
    return true;
}

void ProjectModel::setProjects(std::vector<hangar::Project> projects, QList<bool> overridden)
{
    if (!m_all.empty() && updateInPlace(projects, overridden))
        return;

    // Newest first: what you worked on lately is what you look for.
    std::vector<int> order(projects.size());
    for (std::size_t i = 0; i < order.size(); ++i)
        order[i] = int(i);
    auto stamp = [&](const hangar::Project& p) { return std::max(p.modified, p.lastCommit); };
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return stamp(projects[std::size_t(a)]) > stamp(projects[std::size_t(b)]); });

    std::vector<hangar::Project> sorted;
    QList<bool> sortedOverridden;
    sorted.reserve(projects.size());
    for (int i : order) {
        sorted.push_back(std::move(projects[std::size_t(i)]));
        sortedOverridden.push_back(i < overridden.size() && overridden[i]);
    }

    beginResetModel();
    m_all = std::move(sorted);
    m_overridden = std::move(sortedOverridden);
    m_visible.clear();
    for (int i = 0; i < int(m_all.size()); ++i)
        if (matches(m_all[std::size_t(i)]))
            m_visible.push_back(i);
    endResetModel();
    emit countsChanged();
    emit filterChanged();
}

void ProjectModel::setInstalledEditors(QStringList editors)
{
    m_installedEditors = std::move(editors);
    if (!m_visible.empty())
        emit dataChanged(index(0), index(int(m_visible.size()) - 1), { EditorRole, EditorsRole });
}

int ProjectModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_visible.size());
}

QStringList ProjectModel::installed(const hangar::Project& p) const
{
    QStringList out;
    for (const std::string& e : p.editors) {
        const QString name = qstr(e);
        if (m_installedEditors.contains(name))
            out << name;
    }
    return out;
}

QVariant ProjectModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= int(m_visible.size()))
        return {};
    const int i = m_visible[std::size_t(index.row())];
    const hangar::Project& p = m_all[std::size_t(i)];
    switch (role) {
    case Qt::DisplayRole:
    case NameRole: return qstr(p.name);
    case PathRole: return qpath(p.path);
    case DisplayPathRole: {
        QString path = qpath(p.path);
        const QString home = QDir::homePath();
        if (path.startsWith(home))
            path = QStringLiteral("~") + path.mid(home.size());
        return path;
    }
    case CategoryRole: return int(p.category);
    case OverriddenRole: return i < m_overridden.size() && m_overridden[i];
    case TagsRole: {
        QStringList tags;
        for (const std::string& t : p.tags)
            tags << qstr(t);
        return tags;
    }
    case ReasonRole: return qstr(p.reason);
    case ModifiedRole: return fromUnix(std::max(p.modified, p.lastCommit));
    case HasGitRole: return p.hasGit;
    case BranchRole: return qstr(p.branch);
    case LastCommitRole: return fromUnix(p.lastCommit);
    case RemoteUrlRole: return qstr(p.remoteUrl);
    case EditorRole: {
        const QStringList list = installed(p);
        return list.isEmpty() ? QString() : list.first();
    }
    case EditorsRole: return installed(p);
    default: return {};
    }
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { PathRole, "path" },
        { DisplayPathRole, "displayPath" },
        { CategoryRole, "category" },
        { OverriddenRole, "overridden" },
        { TagsRole, "tags" },
        { ReasonRole, "reason" },
        { ModifiedRole, "modified" },
        { HasGitRole, "hasGit" },
        { BranchRole, "branch" },
        { LastCommitRole, "lastCommit" },
        { RemoteUrlRole, "remoteUrl" },
        { EditorRole, "editor" },
        { EditorsRole, "editors" },
    };
}

void ProjectModel::setCategory(int category)
{
    if (category == m_category)
        return;
    m_category = category;
    refilter();
}

void ProjectModel::setSearch(const QString& search)
{
    if (search == m_search)
        return;
    m_search = search;
    refilter();
}

bool ProjectModel::matches(const hangar::Project& p) const
{
    if (m_category >= 0 && int(p.category) != m_category)
        return false;
    const QString needle = m_search.trimmed();
    if (needle.isEmpty())
        return true;
    if (qstr(p.name).contains(needle, Qt::CaseInsensitive) || qstr(p.reason).contains(needle, Qt::CaseInsensitive))
        return true;
    for (const std::string& t : p.tags)
        if (qstr(t).contains(needle, Qt::CaseInsensitive))
            return true;
    return false;
}

void ProjectModel::refilter()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < int(m_all.size()); ++i)
        if (matches(m_all[std::size_t(i)]))
            m_visible.push_back(i);
    endResetModel();
    emit filterChanged();
}

QVariantList ProjectModel::counts() const
{
    QVariantList out;
    for (int c = 0; c < hangar::kCategoryCount; ++c)
        out << int(std::count_if(m_all.begin(), m_all.end(), [c](const hangar::Project& p) { return int(p.category) == c; }));
    return out;
}
