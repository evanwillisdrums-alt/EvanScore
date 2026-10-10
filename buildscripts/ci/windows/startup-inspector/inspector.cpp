// Isolated CI diagnostic only. This plugin is never packaged with the app.
#include <qpa/qplatformthemeplugin.h>
#ifdef Q_OS_WIN
#include <qpa/qplatformintegration.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <private/qguiapplication_p.h>
#endif
#include <QCoreApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImage>
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
#include <QElapsedTimer>
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
        if (mode == "mallets") {
            QTimer::singleShot(4000, window, [window = QPointer<QQuickWindow>(window)] {
                if (window && window->title().contains("mallet-chords")) verifyMallets(window);
            });
        } else if (mode == "dynamics") {
            QTimer::singleShot(4000, window, [root = QPointer<QObject>(root), window = QPointer<QQuickWindow>(window)] {
                if (!root || !window) return;
                if (window->title() == "EvanScore") {
                    qWarning() << "DYNAMICS INSPECTOR empty workspace: skipped";
                    return;
                }
                verifyTransport(window);
                // Loaded models survive while a different sidebar tab is
                // active. Reopen through the native command so real clicks
                // target the visible Dynamics tab, including saved layouts.
                if (dockOpen(window, "dynamicsPanel")) {
                    dispatchMenu(window, "command://app/dock/toggle-dynamics");
                    QTest::qWait(100);
                }
                dispatchMenu(window, "command://app/dock/toggle-dynamics");
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
                    const auto unchangedMappings = model->property("mappings");
                    auto more = qobject_cast<QQuickItem*>(findItem(window->contentItem(), "dynamics-mapping-ff-extras"));
                    auto ghost = qobject_cast<QQuickItem*>(findItem(window->contentItem(), "dynamics-mapping-ff-ghost"));
                    auto mainTap = qobject_cast<QQuickItem*>(findItem(window->contentItem(), "dynamics-mapping-ff-tap"));
                    if (!more || !ghost || !mainTap || ghost->isVisible() || !mainTap->isVisible())
                        qFatal("Main articulations or collapsed extras have incorrect visibility");
                    for (bool expanded : {true, false}) {
                        reveal(more);
                        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                            more->mapToScene(QPointF(more->width()/2, more->height()/2)).toPoint());
                        QTest::qWait(100);
                        if (ghost->isVisible() != expanded || model->property("mappings") != unchangedMappings)
                            qFatal("Articulation disclosure failed or changed playback settings");
                    }
                    qWarning() << "DYNAMICS INSPECTOR compact articulations real clicks passed";
                    auto smooth = qobject_cast<QQuickItem*>(findItem(window->contentItem(), "dynamics-smooth-articulations"));
                    if (!smooth || !smooth->isVisible()) qFatal("Articulation smoothing switch is unavailable");
                    const auto beforeSmooth = model->property("state").toMap();
                    reveal(smooth);
                    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                        smooth->mapToScene(QPointF(10, smooth->height()/2)).toPoint());
                    QTest::qWait(150);
                    if (model->property("state").toMap().value("smoothArticulations").toBool()
                        == beforeSmooth.value("smoothArticulations").toBool())
                        qFatal("Articulation smoothing click did not update the native score");
                    dispatchMenu(window, "command://notation/undo"); QTest::qWait(250);
                    const auto afterSmooth = model->property("state").toMap();
                    if (afterSmooth.value("smoothArticulations") != beforeSmooth.value("smoothArticulations")
                        || afterSmooth.value("enabled") != beforeSmooth.value("enabled"))
                        qFatal("Articulation smoothing did not undo in one step");
                    qWarning() << "DYNAMICS INSPECTOR smoothing real click and native undo passed";
                    reveal(checkbox);
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
                                reportMemory("before repeated edits");
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
                                    if (cycle == 11) {
                                        // Reproduce the user's accumulated-edit sequence
                                        // through the real editor/button, not just the model.
                                        reveal(velocity);
                                        editor->forceActiveFocus();
                                        QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
                                        QTest::keyClick(window, Qt::Key_4); QTest::keyClick(window, Qt::Key_9);
                                        reveal(apply);
                                        QSignalSpy bulkClick(apply, SIGNAL(clicked()));
                                        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                                            apply->mapToScene(QPointF(apply->width()/2, apply->height()/2)).toPoint());
                                        QTest::qWait(40);
                                        if (bulkClick.count() != 1) qFatal("Repeated Set all taps at 49 click was not delivered");
                                        for (const auto& mapping : model->property("mappings").toList())
                                            if (mapping.toMap().value("tap").toInt() != 49)
                                                qFatal("Repeated Set all taps at 49 missed a mapping row");
                                        qWarning() << "DYNAMICS INSPECTOR repeated edits followed by real typed-49 Set all click passed";
                                    } else {
                                        QMetaObject::invokeMethod(model, "setColumnMapping", Qt::DirectConnection,
                                                                  Q_ARG(int, 2), Q_ARG(int, 31 + cycle));
                                    }
                                    QTest::qWait(40);
                                    for (int undoIndex = 0; undoIndex < 3; ++undoIndex) {
                                        dispatchMenu(window, "command://notation/undo");
                                        QTest::qWait(40);
                                    }
                                    verifyColumn();
                                    reportMemory("after edit cycle", cycle + 1);
                                }
                                qWarning() << "DYNAMICS INSPECTOR repeated mixed edits: 12 cycles, 36 edits and 36 undo passed";
                                const QSize originalSize = window->size();
                                for (const QSize size : {QSize(880, 700), QSize(1280, 900), originalSize}) {
                                    window->resize(size);
                                    QTest::qWait(100);
                                }
                                for (int cycle = 0; cycle < 2; ++cycle) {
                                    const char* commands[] = {"palettes", "properties", "percussion", "mixer", "undo-history", "navigator"};
                                    const char* panels[] = {"palettesPanel", "propertiesPanel", "percussionPanel", "mixerPanel", "undoHistoryPanel", "notationNavigatorPanel"};
                                    for (int index = 0; index < 6; ++index) {
                                        const bool wasOpen = dockOpen(window, panels[index]);
                                        const QByteArray command = QByteArray("command://app/dock/toggle-") + commands[index];
                                        dispatchMenu(window, command.constData());
                                        if (!QTest::qWaitFor([&] { return dockOpen(window, panels[index]) != wasOpen; }, 1500)) {
                                            dumpDockState(window, panels[index]);
                                            qFatal("Panel did not toggle: %s", panels[index]);
                                        }
                                        dispatchMenu(window, command.constData());
                                        if (!QTest::qWaitFor([&] { return dockOpen(window, panels[index]) == wasOpen; }, 1500)) {
                                            dumpDockState(window, panels[index]);
                                            qFatal("Panel did not restore: %s", panels[index]);
                                        }
                                        qWarning() << "DYNAMICS INSPECTOR panel toggled and restored" << panels[index] << wasOpen;
                                    }
                                }
                                if (!model) qFatal("Dynamics model vanished during general panel checks");
                                verifyColumn();
                                qWarning() << "DYNAMICS INSPECTOR general panel and resize checks passed";
                                verifyMixer(window);
                                dispatchMenu(window, "command://notation/select-all");
                                QTest::qWait(250);
                                QMetaObject::invokeMethod(model, "followSelection", Qt::DirectConnection);
                                const auto state = model->property("state").toMap();
                                if (state.value("scope").toString() != "notes" || !state.value("velocityKnown").toBool())
                                    qFatal("Selected notes did not expose a known custom velocity");
                                qWarning() << "DYNAMICS INSPECTOR selected notes" << state.value("title")
                                           << state.value("noteCategory") << state.value("effectiveVelocity");
                                verifyVelocityEnter(window, model);
                                verifyFeatureDebug(window);
                                auto noteSpin = findItem(window->contentItem(), "dynamics-note-velocity");
                                auto noteEditor = noteSpin ? qvariant_cast<QQuickItem*>(noteSpin->property("contentItem")) : nullptr;
                                if (!noteEditor) qFatal("Note velocity editor disappeared before keyboard Save");
                                noteEditor->forceActiveFocus();
                                QTest::keyClick(window, Qt::Key_S, Qt::ControlModifier);
                                QTimer::singleShot(1000, window, [] {
                                    qWarning() << "DYNAMICS INSPECTOR mapping save requested; verify the native score archive (Ctrl+S with velocity editor focused)";
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
    static void verifyVelocityEnter(QQuickWindow* window, QObject* model) {
        const auto before = model->property("state").toMap();
        auto spin = findItem(window->contentItem(), "dynamics-note-velocity");
        if (!spin || !spin->isVisible()) qFatal("Note velocity editor is unavailable");
        for (const int key : {Qt::Key_Return, Qt::Key_Enter}) {
            reveal(spin);
            auto editor = qvariant_cast<QQuickItem*>(spin->property("contentItem"));
            if (!editor) qFatal("Note velocity text editor is unavailable");
            editor->forceActiveFocus();
            QSignalSpy modified(spin, SIGNAL(valueModified()));
            QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
            QTest::keyClick(window, Qt::Key_4); QTest::keyClick(window, Qt::Key_9);
            QTest::keyClick(window, Qt::Key(key)); QTest::qWait(200);
            if (modified.count() != 1 || model->property("state").toMap().value("effectiveVelocity").toInt() != 49)
                qFatal("Enter did not commit a single native note velocity edit");
            dispatchMenu(window, "command://notation/undo"); QTest::qWait(200);
            const auto restored = model->property("state").toMap();
            if (restored.value("effectiveVelocity") != before.value("effectiveVelocity")
                || restored.value("localOverride") != before.value("localOverride"))
                qFatal("Enter triggered an extra score command or did not undo cleanly");
        }
        qWarning() << "DYNAMICS INSPECTOR Return and keypad Enter commit velocities without extra score commands passed";
    }
    static void verifyMallets(QQuickWindow* window) {
        dispatchMenu(window, "command://notation/select-all");
        dispatchMenu(window, "command://app/dock/toggle-mallet"); QTest::qWait(700);
        const auto findModel = [window]() -> QObject* {
            for (auto* obj : objects(window)) if (QByteArray(obj->metaObject()->className()).contains("MalletPanelModel")) return obj;
            return nullptr;
        };
        QPointer<QObject> model = findModel();
        if (!model || !model->property("active").toBool()) qFatal("Native mallet model did not activate");
        const auto state = [&model] { return model ? model->property("state").toMap() : QVariantMap(); };
        const auto original = state();
        if (!original.value("supported").toBool() || original.value("bars").toList().size() != 61
            || original.value("pose").toMap().value("pitches").toList().size() != 4)
            qFatal("Selected marimba chord or five-octave range did not load");
        auto panel = findItem(window->contentItem(), "MalletPanel");
        if (!panel || panel->height() > 221 || panel->height() < 100) qFatal("Mallet panel did not open compactly");
        const auto source = original.value("sourceKey");
        panel->setProperty("currentTab", 2); QTest::qWait(100);
        auto opening = findItem(panel, "mallet-player-opening");
        if (!opening) qFatal("Mallet player opening editor is unavailable");
        for (const int key : {Qt::Key_Return, Qt::Key_Enter}) {
            reveal(opening);
            auto editor = qvariant_cast<QQuickItem*>(opening->property("contentItem"));
            if (!editor) qFatal("Mallet player text editor is unavailable");
            editor->forceActiveFocus(); QSignalSpy modified(opening, SIGNAL(valueModified()));
            QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
            QTest::keyClick(window, Qt::Key_3); QTest::keyClick(window, Qt::Key_0);
            QTest::keyClick(window, Qt::Key(key)); QTest::qWait(100);
            if (modified.count() != 1 || state().value("opening").toInt() != 30 || state().value("sourceKey") != source)
                qFatal("Mallet player Enter did not commit a single setting without editing notes");
            QMetaObject::invokeMethod(model, "setOption", Q_ARG(QString, "opening"), Q_ARG(QVariant, original.value("opening")));
        }
        panel->setProperty("currentTab", 0); QTest::qWait(100);
        const auto appearance = original.value("skin");
        int octave = -1;
        for (auto value : model->property("alternatives").toList()) {
            const auto row = value.toMap();
            if (row.value("valid").toBool() && row.value("description").toString().contains("Octave")) { octave = row.value("index").toInt(); break; }
        }
        if (octave < 0) qFatal("No valid octave alternative was generated");
        QMetaObject::invokeMethod(model, "selectAlternative", Q_ARG(int, octave));
        if (state().value("sourceKey") != source || !state().value("canCommit").toBool()) qFatal("Preview changed the score or cannot commit");
        QMetaObject::invokeMethod(model, "commit"); QTest::qWait(150);
        const auto committed = state().value("sourceKey");
        if (committed == source || state().value("sticking").toString().isEmpty()) qFatal("Mallet Commit did not apply pitches and sticking");
        dispatchMenu(window, "command://notation/undo"); QTest::qWait(150);
        if (state().value("sourceKey") != source || state().value("sticking") != original.value("sticking"))
            qFatal("Mallet Commit did not undo pitches and original sticking together");
        dispatchMenu(window, "command://notation/redo"); QTest::qWait(150);
        if (state().value("sourceKey") != committed) qFatal("Mallet Commit redo failed");
        dispatchMenu(window, "command://notation/undo"); QTest::qWait(150);
        QMetaObject::invokeMethod(model, "setPickMode", Q_ARG(bool, true));
        QMetaObject::invokeMethod(model, "clearPicked");
        for (int pitch : {60, 64, 67, 71}) QMetaObject::invokeMethod(model, "togglePitch", Q_ARG(int, pitch));
        if (state().value("pose").toMap().value("pitches").toList().size() != 4 || state().value("canCommit").toBool())
            qFatal("Manual bar picking failed or was allowed to commit to the score");
        QMetaObject::invokeMethod(model, "setOption", Q_ARG(QString, "sideView"), Q_ARG(QVariant, QVariant(true)));
        QTest::qWait(100);
        QMetaObject::invokeMethod(model, "setOption", Q_ARG(QString, "sideView"), Q_ARG(QVariant, QVariant(false)));
        QMetaObject::invokeMethod(model, "setPickMode", Q_ARG(bool, false));
        if (state().value("skin") != appearance || state().value("sourceKey") != source) qFatal("Appearance or score changed during preview");
        reportMemory("before mallet visibility loop");
        for (int cycle = 0; cycle < 20; ++cycle) {
            dispatchMenu(window, "command://app/dock/toggle-mallet"); QTest::qWait(40);
            if (model && model->property("active").toBool()) qFatal("Hidden mallet model retained subscriptions");
            dispatchMenu(window, "command://app/dock/toggle-mallet"); QTest::qWait(80);
            model = findModel();
            if (!model || !model->property("active").toBool() || state().value("bars").toList().size() != 61) qFatal("Mallet panel did not reopen");
        }
        reportMemory("after mallet visibility loop");
        window->resize(1100, 760); QTest::qWait(150);
        QMetaObject::invokeMethod(model, "selectAlternative", Q_ARG(int, octave));
        QMetaObject::invokeMethod(model, "commit"); QTest::qWait(150);
        if (state().value("sourceKey") == source) qFatal("Final mallet edit did not reach the saved score");
        panel = findItem(window->contentItem(), "MalletPanel");
        if (!panel) qFatal("Mallet panel disappeared before keyboard Save");
        panel->setProperty("currentTab", 2); QTest::qWait(100);
        opening = findItem(panel, "mallet-player-opening");
        auto playerEditor = opening ? qvariant_cast<QQuickItem*>(opening->property("contentItem")) : nullptr;
        if (!playerEditor) qFatal("Mallet player editor disappeared before keyboard Save");
        playerEditor->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_S, Qt::ControlModifier); QTest::qWait(600);
        panel->setProperty("currentTab", 0); QTest::qWait(100);
        const auto capturePath = qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE");
        auto instrumentView = findItem(panel, "mallet-instrument-view");
        if (!instrumentView) qFatal("Mallet instrument view disappeared");
        qWarning() << "MALLET INSPECTOR native instrument geometry" << instrumentView->size()
                   << "panel" << panel->size()
                   << "paint bars" << instrumentView->property("scene").toMap().value("bars").toList().size();
        if (instrumentView->width() < 200 || instrumentView->height() < 60
            || instrumentView->property("scene").toMap().value("bars").toList().size() != 61)
            qFatal("Native mallet geometry hides the instrument/player or loses render inputs");
        if (!capturePath.isEmpty() && !window->grabWindow().save(capturePath))
            qFatal("Could not capture the complete native mallet window");
        qWarning() << "MALLET INSPECTOR range, preview, commit, undo, redo, pick, front and 20 reopen checks passed";
    }
    static void verifyTransport(QQuickWindow* window) {
        for (const auto name : {"mainToolBar", "playbackToolBar", "undoRedoToolBar"}) {
            auto dock = findItem(window->contentItem(), name);
            if (dock) qWarning() << "DYNAMICS INSPECTOR toolbar" << name << "height" << dock->height()
                << "min/max" << dock->property("minimumHeight") << dock->property("maximumHeight")
                << "content" << dock->property("contentHeight");
            for (auto* ancestor = dock; ancestor; ancestor = ancestor->parentItem()) {
                qWarning() << "DYNAMICS INSPECTOR toolbar ancestor" << ancestor->metaObject()->className()
                    << ancestor->objectName() << ancestor->size()
                    << ancestor->property("kddockwidgets_min_size") << ancestor->property("kddockwidgets_max_size");
            }
            for (auto* obj : objects(window)) {
                if (obj->objectName() == name) qWarning() << "DYNAMICS INSPECTOR toolbar object"
                    << obj->metaObject()->className() << obj->property("height") << obj->property("minimumHeight")
                    << obj->property("maximumHeight") << obj->property("contentHeight");
            }
            if (!dock || qAbs(dock->height() - 36.0) > 0.5)
                qFatal("Transport dock row height regression: %s", name);
        }
        auto button = findItem(window->contentItem(), "transport-time-format");
        auto value = findItem(window->contentItem(), "transport-time-value");
        QObject* model = nullptr;
        for (QObject* object : objects(window)) {
            if (QByteArray(object->metaObject()->className()).contains("PlaybackToolBarModel")) { model = object; break; }
        }
        if (!button || !value || !model || !button->isEnabled()) qFatal("Transport format toggle is unavailable");
        if (qEnvironmentVariable("EVANSCORE_EXPECT_MUSICAL_TIME") == "1" && !model->property("musicalTime").toBool())
            qFatal("Time format was not remembered across app restarts");
        const double center = button->y() + button->height()/2;
        for (const auto name : {"transport-bar-beat", "transport-tempo", "transport-meter-key"}) {
            auto field = findItem(window->contentItem(), name);
            if (!field || qAbs(field->y() + field->height()/2 - center) > 0.5) qFatal("Transport values are not horizontally aligned");
        }
        for (int click = 0; click < 2; ++click) {
            const bool before = model->property("musicalTime").toBool();
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint());
            QTest::qWait(50);
            if (model->property("musicalTime").toBool() == before) qFatal("Time format click did not switch");
            const auto expected = model->property(before ? "elapsedPosition" : "musicalPosition").toString();
            if (value->property("text").toString() != expected || expected.isEmpty()) qFatal("Time readout did not follow its format");
        }
        if (!model->property("musicalTime").toBool()) {
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint());
        }
        qWarning() << "DYNAMICS INSPECTOR transport pointer toggle, alignment and format checks passed";
    }
    static void verifyMixer(QQuickWindow* window) {
        const bool wasOpen = dockOpen(window, "mixerPanel");
        if (!wasOpen) dispatchMenu(window, "command://app/dock/toggle-mixer");
        QTest::qWait(150);
        QObject* model = nullptr;
        QObject* menu = nullptr;
        for (QObject* object : objects(window)) {
            if (object->objectName() == "mixer-panel-model") model = object;
            if (QByteArray(object->metaObject()->className()).contains("MixerPanelContextMenuModel")) menu = object;
        }
        if (!model || !menu) qFatal("Mixer models did not load");
        auto diagnostics = [model] {
            QVariantMap result;
            if (!QMetaObject::invokeMethod(model, "diagnostics", Qt::DirectConnection, Q_RETURN_ARG(QVariantMap, result)))
                qFatal("Mixer runtime diagnostics missing");
            return result;
        };
        const auto initial = diagnostics();
        auto dispatchMixer = [menu](const char* command) {
            if (!QMetaObject::invokeMethod(menu, "handleMenuItem", Qt::DirectConnection,
                                          Q_ARG(QString, QString::fromUtf8(command))))
                qFatal("Could not dispatch the native Mixer menu item: %s", command);
        };
        reportMemory("before mixer toggles");
        qint64 slowest = 0;
        for (int cycle = 0; cycle < 10; ++cycle) {
            for (const auto section : {"volume", "fader", "sound", "audio-fx"}) {
                const QByteArray command = QByteArray("command://playback/toggle-mixer-section?section=") + section;
                const QString property = QByteArray(section) == "audio-fx" ? QStringLiteral("audioFxSectionVisible") : QString::fromUtf8(section) + "SectionVisible";
                const bool before = menu->property(property.toUtf8()).toBool();
                for (int toggle = 0; toggle < 2; ++toggle) {
                    QElapsedTimer elapsed; elapsed.start();
                    dispatchMixer(command.constData());
                    const bool expected = toggle == 0 ? !before : before;
                    if (!QTest::qWaitFor([&] { return menu->property(property.toUtf8()).toBool() == expected; }, 1500))
                        qFatal("Mixer section visibility did not update");
                    QTest::qWait(20);
                    slowest = std::max(slowest, elapsed.elapsed());
                    if (QByteArray(section) == "fader" && model->property("meteringEnabled").toBool() != expected)
                        qFatal("Mixer hidden-meter subscription did not follow fader visibility");
                }
            }
            for (const auto command : {"command://playback/toggle-aux-send?auxsend-index=0", "command://playback/toggle-aux-channel?auxchannel-index=0"}) {
                dispatchMixer(command); QTest::qWait(20);
                dispatchMixer(command); QTest::qWait(20);
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            const auto now = diagnostics();
            if (now.value("ownedChannelObjects").toInt() != now.value("channels").toInt()
                || now.value("ownedAuxSendControls").toInt() != now.value("visibleAuxSendControls").toInt()
                || now.value("channels") != initial.value("channels")) qFatal("Mixer UI controls accumulated after visibility changes");
        }
        qWarning() << "DYNAMICS INSPECTOR mixer visibility and ownership checks passed" << diagnostics()
                   << "slowest section toggle ms" << slowest;
        reportMemory("after mixer toggles");
        if (!wasOpen) dispatchMenu(window, "command://app/dock/toggle-mixer");
    }
    static void verifyFeatureDebug(QQuickWindow* mainWindow) {
        dispatchMenu(mainWindow, "command://app/feature-diagnostics");
        QPointer<QQuickWindow> dialog;
        QQuickItem* report = nullptr;
        for (int attempt = 0; attempt < 50 && !report; ++attempt) {
            QTest::qWait(100);
            for (QWindow* window : QGuiApplication::topLevelWindows()) {
                auto quick = qobject_cast<QQuickWindow*>(window);
                if (!quick || quick == mainWindow) continue;
                report = findItem(quick->contentItem(), "feature-debug-report");
                if (report) { dialog = quick; break; }
            }
        }
        if (!dialog || !report) qFatal("Feature diagnostics window did not open");
        auto readReport = [report]() {
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(report->property("text").toString().toUtf8(), &error);
            if (error.error != QJsonParseError::NoError || !document.isObject())
                qFatal("Feature diagnostics report is not valid JSON");
            return document.object();
        };
        auto data = readReport();
        if (data.value("runtime").toObject().value("workingSetBytes").toDouble() <= 0)
            qFatal("Feature diagnostics omitted actual process memory");
        const auto rows = data.value("sticking").toArray();
        if (!data.value("scoreOpen").toBool() || rows.isEmpty() || data.value("truncated").toBool()
            || data.value("scope").toString() != "selected notes")
            qFatal("Feature diagnostics did not inspect the real selected score");
        for (const auto& row : rows) {
            if (row.toObject().value("dynamics").toArray().isEmpty()
                || row.toObject().value("sampleApplication").toString() != "pending VDL playback integration")
                qFatal("Feature diagnostics omitted note dynamics or misreported VDL application");
        }
        if (mainWindow->title().contains("ff-snare")) {
            if (rows.size() != 4) qFatal("Snare sticking diagnostics did not report four onsets");
            const QStringList expected { "right", "left", "right", "right" };
            for (qsizetype index = 0; index < rows.size(); ++index) {
                const auto row = rows.at(index).toObject();
                const auto strokes = row.value("strokes").toArray();
                if (row.value("recognition").toString() != "hands" || strokes.size() != 1
                    || strokes.at(0).toObject().value("hand").toString() != expected.at(index))
                    qFatal("Snare sticking diagnostics lost an explicit or repeated hand");
                const auto dynamics = row.value("dynamics").toArray().at(0).toObject();
                const bool tap = index == 1 || index == 2;
                if (!dynamics.value("velocityKnown").toBool()
                    || dynamics.value("category").toString() != (tap ? "tap" : "accent")
                    || (tap && dynamics.value("velocity").toInt() != 30))
                    qFatal("Snare diagnostics did not report the real tap/ accent mapping");
            }
        }
        auto checkbox = findItem(dialog->contentItem(), "feature-debug-full-score");
        auto refresh = findItem(dialog->contentItem(), "feature-debug-refresh");
        auto copy = findItem(dialog->contentItem(), "feature-debug-copy");
        if (!checkbox || !refresh || !copy) qFatal("Feature diagnostics controls missing");
        QTest::mouseClick(dialog, Qt::LeftButton, Qt::NoModifier,
            checkbox->mapToScene(QPointF(10, checkbox->height()/2)).toPoint());
        QTest::qWait(100);
        if (readReport().value("scope").toString() != "full score") qFatal("Feature diagnostics scope switch failed");
        QTest::mouseClick(dialog, Qt::LeftButton, Qt::NoModifier,
            refresh->mapToScene(QPointF(refresh->width()/2, refresh->height()/2)).toPoint());
        QTest::mouseClick(dialog, Qt::LeftButton, Qt::NoModifier,
            copy->mapToScene(QPointF(copy->width()/2, copy->height()/2)).toPoint());
        QTest::qWait(100);
        if (QGuiApplication::clipboard()->text() != report->property("text").toString())
            qFatal("Feature diagnostics copy report failed");
        const QString capturePath = qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE");
        if (!capturePath.isEmpty() && !dialog->grabWindow().save(capturePath))
            qFatal("Could not capture the actual feature diagnostics window");
        dialog->setWidth(680);
        dialog->setHeight(490);
        QTest::qWait(100);
        if (report->width() < 300 || report->height() < 150) qFatal("Feature diagnostics resize hid the report");
        dialog->close();
        QTest::qWait(150);
        if (dialog && dialog->isVisible()) qFatal("Feature diagnostics did not close");
        qWarning() << "DYNAMICS INSPECTOR feature diagnostics real report, scope, copy, resize and close passed";
    }
    static void reportMemory(const char* phase, int cycle = 0) {
#ifdef Q_OS_WIN
        PROCESS_MEMORY_COUNTERS_EX memory {};
        memory.cb = sizeof(memory);
        if (!K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)))
            qFatal("Could not measure native app memory");
        qWarning() << "DYNAMICS INSPECTOR memory" << phase << "cycle" << cycle
                   << "working set bytes" << quint64(memory.WorkingSetSize)
                   << "private bytes" << quint64(memory.PrivateUsage);
#else
        Q_UNUSED(phase); Q_UNUSED(cycle);
#endif
    }
    static void dumpDockState(QObject* window, const char* name) {
        for (QObject* object : objects(window)) {
            if (object->objectName() != QString::fromUtf8(name)) continue;
            bool open = false;
            QMetaObject::invokeMethod(object, "isOpen", Qt::DirectConnection, Q_RETURN_ARG(bool, open));
            qWarning() << "DYNAMICS INSPECTOR dock state" << name << object->metaObject()->className()
                       << "open" << open << "visible" << object->property("visible");
        }
    }
    static bool dockOpen(QObject* window, const char* name) {
        // Navigator is a loader inside the score view, not a DockPanel.
        // Its native toggle changes NotationPageModel's visibility property.
        if (QByteArray(name) == "notationNavigatorPanel") {
            for (QObject* object : objects(window)) {
                if (QByteArray(object->metaObject()->className()).contains("NotationPageModel"))
                    return object->property("isNavigatorVisible").toBool();
            }
            qFatal("Could not read native navigator state");
        }
        for (QObject* object : objects(window)) {
            if (object->objectName() != QString::fromUtf8(name)
                || object->metaObject()->indexOfMethod("isOpen()") < 0) continue;
            bool open = false;
            if (QMetaObject::invokeMethod(object, "isOpen", Qt::DirectConnection, Q_RETURN_ARG(bool, open))) return open;
        }
        qFatal("Could not read native panel state: %s", name);
        return false;
    }
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
