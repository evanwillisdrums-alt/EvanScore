// Isolated CI diagnostic only. This plugin is never packaged with the app.
#include <qpa/qplatformthemeplugin.h>
#ifdef Q_OS_WIN
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#endif
#include <QCoreApplication>
#include <QEvent>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDebug>
#include <QVariant>
#include <QTimer>
#include <QPointer>
#include <QKeyEvent>
#include <QSet>

class Inspector final : public QObject {
public:
    explicit Inspector(QObject* parent) : QObject(parent) {}
    bool eventFilter(QObject* object, QEvent* event) override {
        if (done || event->type() != QEvent::UpdateRequest) return false;
        auto window = qobject_cast<QQuickWindow*>(object);
        if (!window) return false;
        auto root = findItem(window->contentItem(), "WindowContent");
        if (!root) return false;
        auto toolbar = root->findChild<QObject*>("notationToolBar");
        if (!toolbar) return false;
        done = true;
        const auto mode = qEnvironmentVariable("EVANSCORE_LAYOUT_TRIAL");
        qWarning() << "LAYOUT INSPECTOR" << mode << "window" << window->size()
                   << "page" << root->property("currentPageUri");
        dump(root);
        if (mode == "dynamics") {
            QTimer::singleShot(4000, window, [root = QPointer<QObject>(root), window = QPointer<QQuickWindow>(window)] {
                if (!root || !window) return;
                if (window->title() == "EvanScore") {
                    qWarning() << "DYNAMICS INSPECTOR empty workspace: skipped";
                    return;
                }
                bool opened = false;
                for (QObject* object : objects(window)) {
                    if (!QByteArray(object->metaObject()->className()).endsWith("AppMenuModel")) continue;
                    opened = QMetaObject::invokeMethod(object, "handleMenuItem", Qt::DirectConnection,
                        Q_ARG(QString, QStringLiteral("command://app/dock/toggle-dynamics")));
                    if (opened) break;
                }
                if (!opened) qFatal("The diagnostic could not open View > Dynamics");
                QTimer::singleShot(1000, window, [window] {
                if (!window) return;
                for (QObject* model : objects(window)) {
                    if (!QByteArray(model->metaObject()->className()).contains("DynamicsPanelModel")) continue;
                    if (!model->property("state").toMap().value("available").toBool()) {
                        qWarning() << "DYNAMICS INSPECTOR empty workspace: skipped";
                        return;
                    }
                    const auto tap = [model] {
                        for (const QVariant& entry : model->property("mappings").toList()) {
                            const auto row = entry.toMap();
                            if (row.value("dynamic").toInt() == 10) return row.value("tap").toInt();
                        }
                        return -1;
                    };
                    const int original = tap();
                    qWarning() << "DYNAMICS INSPECTOR editing ff taps from" << original << "to 10";
                    // DynamicType::FF = 10, DynamicsPlayback::Tap = 2. This is
                    // the exact native method called by the sidebar SpinBox.
                    if (!QMetaObject::invokeMethod(model, "setMapping", Qt::DirectConnection,
                            Q_ARG(int, 10), Q_ARG(int, 2), Q_ARG(int, 10))) qFatal("Could not invoke dynamics mapping edit");
                    if (tap() != 10) qFatal("The ff tap mapping did not change to 10");
                    qWarning() << "DYNAMICS INSPECTOR ff taps changed to 10; testing native undo";
                    QKeyEvent press(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier, "z");
                    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Z, Qt::ControlModifier, "z");
                    QCoreApplication::sendEvent(window, &press);
                    QCoreApplication::sendEvent(window, &release);
                    if (tap() != original) qFatal("Native undo did not restore the ff tap mapping");
                    qWarning() << "DYNAMICS INSPECTOR mapping restored after native undo";
                    return;
                }
                qFatal("The native DynamicsPanelModel was not found");
                });
            });
        }
        if (mode == "left") {
            // DockToolBarAlignment::Left == 0. Keep the left grower resizable.
            toolbar->setProperty("alignment", 0);
            toolbar->setProperty("resizable", true);
        } else if (mode == "wide") {
            window->setWidth(1600);
        }
        dump(root);
        return false;
    }
private:
    bool done = false;
    static QList<QObject*> objects(QObject* root) {
        QList<QObject*> result;
        QList<QObject*> queue { root };
        QSet<QObject*> visited;
        while (!queue.isEmpty()) {
            auto object = queue.takeLast();
            if (visited.contains(object)) continue;
            visited.insert(object);
            result.append(object);
            queue.append(object->children());
            if (auto item = qobject_cast<QQuickItem*>(object))
                for (auto child : item->childItems()) queue.append(child);
            if (auto window = qobject_cast<QQuickWindow*>(object)) queue.append(window->contentItem());
        }
        return result;
    }
    static QQuickItem* findItem(QQuickItem* item, const QString& name) {
        if (item->objectName() == name) return item;
        for (auto child : item->childItems()) {
            if (auto found = findItem(child, name)) return found;
        }
        return nullptr;
    }
    static void dump(QObject* root) {
        for (const auto name : {"mainToolBar", "notationToolBar", "playbackToolBar", "undoRedoToolBar", "noteInputBar"}) {
            auto bar = root->findChild<QObject*>(name);
            if (!bar) continue;
            qWarning() << name << "content" << bar->property("contentWidth")
                       << "width" << bar->property("width")
                       << "min" << bar->property("minimumWidth")
                       << "max" << bar->property("maximumWidth")
                       << "resizable" << bar->property("resizable")
                       << "alignment" << bar->property("alignment")
                       << "visible" << bar->property("visible");
        }
    }
};

class InspectorPlugin final : public QPlatformThemePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformThemeFactoryInterface_iid FILE "inspector.json")
public:
    QPlatformTheme* create(const QString&, const QStringList&) override {
        // EvanScore queries styleHints before constructing QApplication. Qt
        // can load the theme here while there is still no application object.
        if (QCoreApplication::instance()) install();
        else qAddPreRoutine(&install);
        // Windows native-window creation requires QWindowsTheme's singleton.
        // An explicit custom key with a null result falls back to the generic
        // theme, not the normal Windows theme. Create the real platform theme.
#ifdef Q_OS_WIN
        return QGuiApplicationPrivate::platformIntegration()->createPlatformTheme(QStringLiteral("windows"));
#else
        return nullptr;
#endif
    }
private:
    static void install() {
        auto app = QCoreApplication::instance();
        if (!app) return;
        qWarning() << "LAYOUT INSPECTOR installed";
        app->installEventFilter(new Inspector(app));
    }
};
#include "inspector.moc"
