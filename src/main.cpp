#include "AppController.h"

#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>

namespace {

// App-wide ⌘R / ⌘F. QML Shortcut matches the typed character, so with the
// Russian layout ⌘R arrives as ⌘К and never fires. Here the physical key is
// checked too (macOS virtual key codes: R = 15, F = 3).
class HotkeyFilter : public QObject {
public:
    explicit HotkeyFilter(AppController* controller)
        : m_controller(controller)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() != QEvent::KeyPress && event->type() != QEvent::ShortcutOverride)
            return QObject::eventFilter(watched, event);
        auto* key = static_cast<QKeyEvent*>(event);
        // Qt maps ⌘ to ControlModifier on macOS.
        const Qt::KeyboardModifiers mods = key->modifiers() & ~Qt::KeypadModifier;
        if (mods != Qt::ControlModifier)
            return QObject::eventFilter(watched, event);

        const quint32 vk = key->nativeVirtualKey();
        const int k = key->key();
        const bool isR = k == Qt::Key_R || k == 0x41A /* К */ || vk == 15;
        const bool isF = k == Qt::Key_F || k == 0x410 /* А */ || vk == 3;
        if (!isR && !isF)
            return QObject::eventFilter(watched, event);

        if (event->type() == QEvent::ShortcutOverride) {
            event->accept();   // "we take this key", the KeyPress follows
            return true;
        }
        if (!key->isAutoRepeat()) {
            if (isR)
                m_controller->rescan();
            else
                emit m_controller->findRequested();
        }
        return true;
    }

private:
    AppController* m_controller;
};

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Hangar"));
    QGuiApplication::setOrganizationName(QStringLiteral("Hangar"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("hangar.app"));
    QGuiApplication::setApplicationVersion(QStringLiteral(PROJECT_VERSION_STRING));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QLockFile instanceLock(dataDir + QStringLiteral("/instance.lock"));
    if (!instanceLock.tryLock(100)) {
        qWarning() << "Hangar is already running";
        return 0;
    }

    AppController controller;
    HotkeyFilter hotkeys(&controller);
    app.installEventFilter(&hotkeys);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({ { QStringLiteral("controller"), QVariant::fromValue(&controller) } });
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Hangar", "Main");

    controller.start();
    return app.exec();
}
