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
#include <QMouseEvent>
#include <QTest>
#include <QSignalSpy>
#include <QSet>
#include <algorithm>

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
                    if (QByteArray(object->metaObject()->className()).contains("DynamicsPanelModel")) {
                        opened = true;
                        break;
                    }
                }
                for (QObject* object : objects(window)) {
                    if (opened) break;
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
                    auto checkbox = qobject_cast<QQuickItem*>(findItem(window->contentItem(), "dynamics-enabled"));
                    if (!checkbox || !checkbox->isVisible() || !checkbox->isEnabled())
                        qFatal("The custom dynamics switch is not clickable");
                    const bool originallyEnabled = model->property("state").toMap().value("enabled").toBool();
                    // Send real window pointer events. Invoking clicked()
                    // alone would miss an overlay intercepting the control.
                    const QPointF point = checkbox->mapToScene(QPointF(10, checkbox->height() / 2));
                    qWarning() << "DYNAMICS INSPECTOR switch position" << point << "size" << checkbox->size();
                    QQuickItem* hit = window->contentItem();
                    for (int depth = 0; hit && depth < 18; ++depth) {
                        qWarning() << "DYNAMICS INSPECTOR pointer path" << hit->objectName() << hit->metaObject()->className();
                        hit = hit->childAt(hit->mapFromScene(point).x(), hit->mapFromScene(point).y());
                    }
                    QSignalSpy clicked(checkbox, SIGNAL(clicked()));
                    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point.toPoint());
                    QTest::qWait(100);
                    qWarning() << "DYNAMICS INSPECTOR click signal count" << clicked.count() << "checked" << checkbox->property("checked");
                    if (model->property("state").toMap().value("enabled").toBool() == originallyEnabled)
                        qFatal("Clicking the custom dynamics switch did not update the score");
                    qWarning() << "DYNAMICS INSPECTOR actual switch click changed score state";
                    qWarning() << "DYNAMICS INSPECTOR editing ff taps from" << original << "to 10";
                    QObject* spin = nullptr;
                    for (QObject* object : objects(window)) {
                        if (object->objectName() == "dynamics-mapping-ff-tap") { spin = object; break; }
                    }
                    if (!spin || !spin->setProperty("value", 10)
                        || !QMetaObject::invokeMethod(spin, "valueModified", Qt::DirectConnection))
                        qFatal("Could not edit the ff tap sidebar SpinBox");
                    if (tap() != 10) qFatal("The ff tap mapping did not change to 10");
                    qWarning() << "DYNAMICS INSPECTOR ff taps changed to 10; testing native undo";
                    bool undoRequested = false;
                    for (QObject* menu : objects(window)) {
                        if (!QByteArray(menu->metaObject()->className()).endsWith("AppMenuModel")) continue;
                        undoRequested = QMetaObject::invokeMethod(menu, "handleMenuItem", Qt::DirectConnection,
                            Q_ARG(QString, QStringLiteral("command://notation/undo")));
                        if (undoRequested) break;
                    }
                    if (!undoRequested) qFatal("The diagnostic could not request Edit > Undo");
                    // Command dispatch settles asynchronously; verify after
                    // it completes rather than aborting inside its request.
                    QTimer::singleShot(500, window, [model = QPointer<QObject>(model), window, original, originallyEnabled] {
                        if (!model) qFatal("Dynamics panel disappeared after undo");
                        for (const QVariant& entry : model->property("mappings").toList()) {
                            const auto row = entry.toMap();
                            if (row.value("dynamic").toInt() != 10) continue;
                            if (row.value("tap").toInt() != original) qFatal("Native undo did not restore the ff tap mapping");
                            qWarning() << "DYNAMICS INSPECTOR mapping restored after native undo";
                            for (QObject* menu : objects(window)) {
                                if (!QByteArray(menu->metaObject()->className()).endsWith("AppMenuModel")) continue;
                                QMetaObject::invokeMethod(menu, "handleMenuItem", Qt::DirectConnection,
                                    Q_ARG(QString, QStringLiteral("command://notation/undo")));
                                break;
                            }
                            QTimer::singleShot(500, window, [model, window, originallyEnabled] {
                                if (!model || model->property("state").toMap().value("enabled").toBool() != originallyEnabled)
                                    qFatal("Native undo did not restore the custom dynamics switch");
                                qWarning() << "DYNAMICS INSPECTOR switch restored after native undo";
                                const QVariantList before = model->property("mappings").toList();
                                auto velocity = findItem(window->contentItem(), "dynamics-column-velocity");
                                auto apply = findItem(window->contentItem(), "dynamics-column-apply");
                                if (!velocity || !apply) qFatal("Bulk tap controls are unavailable");
                                reveal(velocity);
                                auto editor = qvariant_cast<QQuickItem*>(velocity->property("contentItem"));
                                if (!editor) qFatal("Bulk velocity editor is unavailable");
                                editor->forceActiveFocus();
                                QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
                                QTest::keyClick(window, Qt::Key_3);
                                QTest::keyClick(window, Qt::Key_0);
                                // Click while the edit still has focus, matching typing 30
                                // then pressing Set all without an intermediate Enter.
                                reveal(apply);
                                QSignalSpy applied(apply, SIGNAL(clicked()));
                                QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                                                  apply->mapToScene(QPointF(apply->width()/2, apply->height()/2)).toPoint());
                                QTest::qWait(250);
                                qWarning() << "DYNAMICS INSPECTOR Set all real click" << applied.count()
                                           << "editor value" << velocity->property("value");
                                if (applied.count() != 1) qFatal("Set all pointer click was not delivered");
                                auto verifyColumn = [model, before] {
                                    const auto after = model->property("mappings").toList();
                                    if (after.size() != before.size()) qFatal("Bulk mapping changed the dynamic rows");
                                    for (qsizetype i = 0; i < after.size(); ++i) {
                                        auto expected = before[i].toMap(); expected["tap"] = 30;
                                        if (after[i].toMap() != expected) qFatal("Bulk tap edit changed another articulation or missed a row");
                                    }
                                };
                                verifyColumn();
                                dispatchMenu(window, "command://notation/undo");
                                QTest::qWait(250);
                                if (model->property("mappings").toList() != before)
                                    qFatal("Bulk tap edit did not undo in one step");
                                dispatchMenu(window, "command://notation/redo");
                                QTest::qWait(250);
                                verifyColumn();
                                qWarning() << "DYNAMICS INSPECTOR bulk taps, other columns, undo and redo passed";
                                // Exercise accumulated edit state: change two separate
                                // articulation mappings, batch taps, then undo all three.
                                for (int cycle = 0; cycle < 12; ++cycle) {
                                    const auto rows = model->property("mappings").toList();
                                    for (int editIndex = 0; editIndex < 2; ++editIndex) {
                                        const auto row = rows[editIndex].toMap();
                                        const int role = editIndex == 0 ? 3 : 4;
                                        const char* key = editIndex == 0 ? "accent" : "tenuto";
                                        QMetaObject::invokeMethod(model, "setMapping", Qt::DirectConnection,
                                            Q_ARG(int, row.value("dynamic").toInt()), Q_ARG(int, role),
                                            Q_ARG(int, (row.value(key).toInt() + 1) % 128));
                                        QTest::qWait(40);
                                    }
                                    QMetaObject::invokeMethod(model, "setColumnMapping", Qt::DirectConnection,
                                                              Q_ARG(int, 2), Q_ARG(int, 31 + cycle));
                                    QTest::qWait(40);
                                    for (int undoIndex = 0; undoIndex < 3; ++undoIndex) {
                                        dispatchMenu(window, "command://notation/undo");
                                        QTest::qWait(40);
                                    }
                                    verifyColumn();
                                }
                                qWarning() << "DYNAMICS INSPECTOR repeated mixed edits: 12 cycles, 36 edits and 36 undo passed";
                                const QSize originalSize = window->size();
                                for (const QSize size : {QSize(880, 700), QSize(1280, 900), originalSize}) {
                                    window->resize(size);
                                    QTest::qWait(100);
                                }
                                for (int cycle = 0; cycle < 2; ++cycle) {
                                    for (const char* dock : {"palettes", "properties", "percussion", "mixer", "undo-history", "navigator"}) {
                                        const QByteArray command = QByteArray("command://app/dock/toggle-") + dock;
                                        dispatchMenu(window, command.constData()); QTest::qWait(80);
                                        dispatchMenu(window, command.constData()); QTest::qWait(80);
                                    }
                                }
                                if (!model) qFatal("Dynamics model vanished during general panel checks");
                                verifyColumn();
                                qWarning() << "DYNAMICS INSPECTOR general panel and resize checks passed";
                                dispatchMenu(window, "command://notation/select-all");
                                QTest::qWait(250);
                                QMetaObject::invokeMethod(model, "followSelection", Qt::DirectConnection);
                                const auto state = model->property("state").toMap();
                                if (state.value("scope").toString() != "notes" || !state.value("velocityKnown").toBool())
                                    qFatal("Selected notes did not expose a known custom velocity");
                                qWarning() << "DYNAMICS INSPECTOR selected notes" << state.value("title")
                                           << state.value("noteCategory") << state.value("effectiveVelocity");
                                dispatchMenu(window, "command://project/save");
                                QTimer::singleShot(1000, window, [] {
                                    qWarning() << "DYNAMICS INSPECTOR mapping save requested; verify the native score archive";
                                });
                            });
                            return;
                        }
                        qFatal("The ff tap row disappeared after undo");
                    });
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
    static void reveal(QQuickItem* item) {
        for (auto ancestor = item->parentItem(); ancestor; ancestor = ancestor->parentItem()) {
            if (ancestor->metaObject()->indexOfProperty("contentY") < 0) continue;
            const double target = item->mapToItem(ancestor, QPointF(0, item->height()/2)).y();
            const double current = ancestor->property("contentY").toDouble();
            const double maximum = std::max(0.0, ancestor->property("contentHeight").toDouble() - ancestor->height());
            ancestor->setProperty("contentY", std::clamp(current + target - ancestor->height()/2, 0.0, maximum));
            QTest::qWait(100);
            break;
        }
    }
    static void dispatchMenu(QObject* window, const char* command) {
        for (QObject* menu : objects(window)) {
            if (!QByteArray(menu->metaObject()->className()).endsWith("AppMenuModel")) continue;
            if (QMetaObject::invokeMethod(menu, "handleMenuItem", Qt::DirectConnection,
                    Q_ARG(QString, QString::fromUtf8(command)))) return;
        }
        qFatal("Could not dispatch the native menu command: %s", command);
    }
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
