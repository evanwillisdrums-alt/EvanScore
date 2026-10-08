// Isolated CI diagnostic only. This plugin is never packaged with the app.
#include <qpa/qplatformthemeplugin.h>
#include <QCoreApplication>
#include <QEvent>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDebug>
#include <QVariant>

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
        // Let Qt keep its normal Windows platform theme.
        return nullptr;
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
