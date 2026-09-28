#pragma once

#include "Project.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <vector>

// All projects, filtered by category and search text, newest first.
class ProjectModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by AppController")

    Q_PROPERTY(int category READ category WRITE setCategory NOTIFY filterChanged)   // -1 = all
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged)
    Q_PROPERTY(QVariantList counts READ counts NOTIFY countsChanged)                 // per category
    Q_PROPERTY(int total READ total NOTIFY countsChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY filterChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        PathRole,
        DisplayPathRole,   // ~/PycharmProjects/kbzh
        CategoryRole,
        OverriddenRole,
        TagsRole,
        ReasonRole,
        ModifiedRole,      // QDateTime, invalid if unknown
        HasGitRole,
        BranchRole,
        LastCommitRole,
        RemoteUrlRole,
        EditorRole,        // best installed editor, "" if none
        EditorsRole,       // all installed suggested editors
    };

    explicit ProjectModel(QObject* parent = nullptr);

    void setProjects(std::vector<hangar::Project> projects, QList<bool> overridden);
    const std::vector<hangar::Project>& projects() const { return m_all; }
    void setInstalledEditors(QStringList editors);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int category() const { return m_category; }
    void setCategory(int category);
    QString search() const { return m_search; }
    void setSearch(const QString& search);
    QVariantList counts() const;
    int total() const { return int(m_all.size()); }

signals:
    void filterChanged();
    void countsChanged();

private:
    void refilter();
    bool updateInPlace(std::vector<hangar::Project>& projects, const QList<bool>& overridden);
    bool matches(const hangar::Project& p) const;
    QStringList installed(const hangar::Project& p) const;

    std::vector<hangar::Project> m_all;
    QList<bool> m_overridden;
    std::vector<int> m_visible;
    QStringList m_installedEditors;
    int m_category = -1;
    QString m_search;
};
