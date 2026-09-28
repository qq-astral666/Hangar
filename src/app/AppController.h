#pragma once

#include "ProjectModel.h"

#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Glue between the scanner/organiser (core) and the QML window: folders to
// watch, manual category overrides, "open in…" actions, background scans.
class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(ProjectModel* model READ model CONSTANT)
    Q_PROPERTY(QStringList roots READ roots NOTIFY rootsChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString organizeRoot READ organizeRoot WRITE setOrganizeRoot NOTIFY organizeRootChanged)
    Q_PROPERTY(QString organizeRootDisplay READ organizeRootDisplay NOTIFY organizeRootChanged)
    Q_PROPERTY(int hiddenCount READ hiddenCount NOTIFY hiddenChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    ProjectModel* model() { return &m_model; }
    QStringList roots() const { return m_roots; }
    // Only a scan the user asked for shows a spinner; background ones are silent.
    bool scanning() const { return m_scanning && m_scanVisible; }
    QString organizeRoot() const { return m_organizeRoot; }
    int hiddenCount() const { return int(m_hidden.size()); }
    QString organizeRootDisplay() const;
    void setOrganizeRoot(const QString& path);

    void start();

    // Dropped or picked folders: a project is added as a project, a folder
    // of projects as a root to watch.
    Q_INVOKABLE void addUrls(const QList<QUrl>& urls);
    Q_INVOKABLE void removeRoot(const QString& root);
    Q_INVOKABLE void rescan();

    Q_INVOKABLE void setCategory(const QString& path, int category);   // -1 = automatic
    Q_INVOKABLE void openIn(const QString& path, const QString& editor);
    Q_INVOKABLE void reveal(const QString& path);
    Q_INVOKABLE void openTerminal(const QString& path);
    Q_INVOKABLE void openUrl(const QString& url);
    Q_INVOKABLE void copyPath(const QString& path);
    // Moves the project folder to the Trash (recoverable from Finder).
    Q_INVOKABLE void trash(const QString& path);
    // Removes the project from Hangar only; the folder is not touched.
    Q_INVOKABLE void forget(const QString& path);
    Q_INVOKABLE void showHidden();

    Q_INVOKABLE void organize();
    Q_INVOKABLE void revealOrganized();

signals:
    void rootsChanged();
    void scanningChanged();
    void organizeRootChanged();
    void hiddenChanged();
    void toast(const QString& message);
    void findRequested();

private:
    struct ScanResult {
        std::vector<hangar::Project> projects;
    };

    void load();
    void saveRoots();
    void applyOverridesAndPublish(std::vector<hangar::Project> projects);
    void watchRoots();
    void detectEditors();
    void dropFromList(const QString& path);
    void refresh(bool visible);
    QStringList listing(const QString& dir) const;

    ProjectModel m_model;
    QStringList m_roots;
    QStringList m_extraProjects;
    QVariantMap m_overrides;   // path -> category key
    QStringList m_hidden;      // removed from the list, kept on disk
    QString m_organizeRoot;

    QFutureWatcher<ScanResult> m_scan;
    bool m_scanning = false;
    bool m_rescanPending = false;
    bool m_scanVisible = false;
    QHash<QString, QStringList> m_listings;   // root -> project-ish folder names
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QElapsedTimer m_sinceScan;
};
