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
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QToolButton>
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
#include <QFontMetricsF>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQmlExpression>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <algorithm>

class Inspector final : public QObject {
public:
    explicit Inspector(QObject* parent) : QObject(parent) {}
    bool eventFilter(QObject* object, QEvent* event) override {
        if(qEnvironmentVariable("EVANSCORE_LAYOUT_TRIAL")=="save"
           && (event->type()==QEvent::Show || event->type()==QEvent::Hide)) {
            if(auto* dialog=qobject_cast<QFileDialog*>(object))
                qWarning()<<"SAVE INSPECTOR file lifecycle"<<event->type()<<dialog->windowTitle()<<"result"<<dialog->result()<<"visible"<<dialog->isVisible();
        }
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
        if (mode == "save") {
            QTimer::singleShot(4000, window, [window = QPointer<QQuickWindow>(window)] {
                if(window && window->title()!="EvanScore")verifySaveChooser(window);
            });
        } else if (mode == "keypad" || mode == "stems" || mode == "percussion") {
            QTimer::singleShot(4000, window, [window = QPointer<QQuickWindow>(window)] {
                if (window && window->title() != "EvanScore") verifyKeypad(window);
            });
        } else if (mode == "mallets") {
            QTimer::singleShot(4000, window, [window = QPointer<QQuickWindow>(window)] {
                if (window && window->title().contains("mallet-")) verifyMallets(window);
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
                                // The preceding View checks activate other sidebar tabs.
                                // Reopen Dynamics and reacquire any recreated content
                                // before sending real keyboard events to its editor.
                                if (dockOpen(window, "dynamicsPanel")) {
                                    dispatchMenu(window, "command://app/dock/toggle-dynamics");
                                    QTest::qWait(100);
                                }
                                dispatchMenu(window, "command://app/dock/toggle-dynamics");
                                QTest::qWait(250);
                                QObject* selectedModel = nullptr;
                                for (auto* object : objects(window))
                                    if (QByteArray(object->metaObject()->className()).contains("DynamicsPanelModel")) {
                                        selectedModel = object; break;
                                    }
                                if (!selectedModel) qFatal("Dynamics model did not reopen for note velocity editing");
                                dispatchMenu(window, "command://notation/select-all");
                                QTest::qWait(250);
                                QMetaObject::invokeMethod(selectedModel, "followSelection", Qt::DirectConnection);
                                const auto state = selectedModel->property("state").toMap();
                                if (state.value("scope").toString() != "notes" || !state.value("velocityKnown").toBool())
                                    qFatal("Selected notes did not expose a known custom velocity");
                                qWarning() << "DYNAMICS INSPECTOR selected notes" << state.value("title")
                                           << state.value("noteCategory") << state.value("effectiveVelocity");
                                verifyVelocityEnter(window, selectedModel);
                                verifyFeatureDebug(window);
                                verifyDynamicsLocksAndDefaults(window, selectedModel);
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
    static void verifySaveChooser(QQuickWindow* window) {
        dispatchMenu(window,"command://project/save-as");
        QObject* chooser=nullptr;
        QElapsedTimer timer;timer.start();
        while(!chooser && timer.elapsed()<5000) {
            for(auto* object:objects(window))
                if(object->objectName()=="AskSaveLocationTypeDialog")chooser=object;
            if(!chooser)QTest::qWait(20);
        }
        if(!chooser)qFatal("SAVE INSPECTOR location chooser did not open");
        auto* content=qvariant_cast<QQuickItem*>(chooser->property("contentItem"));
        if(!content || !content->window())qFatal("SAVE INSPECTOR chooser content unavailable");
        QQuickItem* computer=nullptr;
        for(auto* object:objects(content)) {
            auto* item=qobject_cast<QQuickItem*>(object);
            if(item && item->isVisible() && item->isEnabled() && item->property("text").toString()=="Save to computer")computer=item;
        }
        if(!computer)qFatal("SAVE INSPECTOR computer button unavailable");
        QPointer<QObject> guardedChooser=chooser;
        QSignalSpy results(chooser,SIGNAL(retChanged(QVariantMap)));
        QTest::mouseClick(content->window(),Qt::LeftButton,Qt::NoModifier,computer->mapToScene({computer->width()/2,computer->height()/2}).toPoint());
        QTest::qWait(1200);
        for(const auto& result:results) {
            const auto envelope=result[0].toMap();
            qWarning()<<"SAVE INSPECTOR chooser result"<<envelope<<"payload type"<<envelope.value("value").typeName();
        }
        QFileDialog* fileDialog=nullptr;
        for(auto* widget:QApplication::topLevelWidgets()) {
            if(auto* dialog=qobject_cast<QFileDialog*>(widget)) {
                qWarning()<<"SAVE INSPECTOR file dialog"<<dialog->windowTitle()<<"visible"<<dialog->isVisible()<<"files"<<dialog->selectedFiles();
                if(dialog->isVisible())fileDialog=dialog;
            }
        }
        qWarning()<<"SAVE INSPECTOR chooser after click"<<guardedChooser<<"result"<<(guardedChooser ? guardedChooser->property("ret") : QVariant());
        if(!fileDialog)qFatal("SAVE INSPECTOR computer choice closed without a visible file dialog");
        QMetaObject::invokeMethod(fileDialog,"reject",Qt::DirectConnection);
        QTest::qWait(200);
        qWarning()<<"SAVE INSPECTOR real computer chooser opened a file dialog and cancel passed";
    }
    static void verifyDynamicsLocksAndDefaults(QQuickWindow* window, QObject* model) {
        const auto originalMappings = model->property("mappings");
        const auto originalState = model->property("state").toMap();
        QMetaObject::invokeMethod(model, "makeDefault", Qt::DirectConnection);
        if (!model->property("state").toMap().value("hasDefault").toBool()
            || model->property("mappings") != originalMappings)
            qFatal("Saving the default changed score mappings or failed");
        QMetaObject::invokeMethod(model, "setNoteVelocity", Qt::DirectConnection, Q_ARG(int, 49));
        auto lock = findItem(window->contentItem(), "dynamics-selection-lock");
        if (!lock || !lock->isVisible() || !lock->isEnabled()) qFatal("Native dynamics lock is unavailable");
        reveal(lock);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, lock->mapToScene(QPointF(10,lock->height()/2)).toPoint());
        QTest::qWait(100);
        if (!model->property("state").toMap().value("locked").toBool()) qFatal("Pointer click did not lock selected notes");
        QMetaObject::invokeMethod(model, "setMapping", Qt::DirectConnection, Q_ARG(int,10), Q_ARG(int,2), Q_ARG(int,37));
        QMetaObject::invokeMethod(model, "applyDefault", Qt::DirectConnection);
        if (model->property("mappings") != originalMappings
            || model->property("state").toMap().value("effectiveVelocity").toInt() != 49)
            qFatal("Applying defaults changed locked notes or failed to restore mappings");
        dispatchMenu(window, "command://project/save"); QTest::qWait(500);
        const QDir folder(QFileInfo(qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE")).absolutePath());
        const auto files = folder.entryList({"*.mscz"}, QDir::Files);
        if (files.size() != 1 || !folder.mkpath("locks")) qFatal("Cannot locate native lock snapshot score");
        const auto snapshot = folder.filePath("locks/locked.mscz");
        if (QFile::exists(snapshot)) QFile::remove(snapshot);
        if (!QFile::copy(folder.filePath(files.front()), snapshot)) qFatal("Cannot retain native lock snapshot");
        reveal(lock);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, lock->mapToScene(QPointF(10,lock->height()/2)).toPoint());
        QTest::qWait(100);
        if (model->property("state").toMap().value("someLocked").toBool()) qFatal("Pointer click did not unlock notes");
        QMetaObject::invokeMethod(model, "applyDefault", Qt::DirectConnection);
        if (model->property("state").toMap().value("localOverride").toBool()) qFatal("Defaults left an unlocked note override");
        dispatchMenu(window, "command://notation/undo"); QTest::qWait(100);
        if (model->property("state").toMap().value("effectiveVelocity").toInt() != 49) qFatal("Default reset did not undo in one step");
        dispatchMenu(window, "command://notation/redo"); QTest::qWait(100);
        if (model->property("state").toMap().value("localOverride").toBool()) qFatal("Default reset did not redo");
        for (int i=0; i<6; ++i) { dispatchMenu(window, "command://notation/undo"); QTest::qWait(80); }
        const auto restored = model->property("state").toMap();
        if (model->property("mappings") != originalMappings || restored.value("someLocked").toBool()
            || restored.value("effectiveVelocity") != originalState.value("effectiveVelocity")
            || restored.value("localOverride") != originalState.value("localOverride"))
            qFatal("Lock/default test did not restore the source score");
        qWarning() << "DYNAMICS INSPECTOR native note lock, save default, apply default, unlock and undo checks passed";
    }
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
    static void verifyPercussion(QQuickWindow* window, QQmlEngine* engine, const std::function<QVariant(const QString&)>& evaluate) {
        const auto select = [&evaluate](int track) {
            if(!evaluate(QString("(function(){var c=curScore.newCursor(); c.track=%1; c.rewindToTick(0); return curScore.selection.select(c.element.articulations[0]);})()").arg(track)).toBool())
                qFatal("Cannot select the actual accent on track %d",track);
            QTest::qWait(150);
        };
        const auto markCounts = [&evaluate] {
            return evaluate("(function(){var c=curScore.newCursor();var n=[];for(var t=4;t<6;++t){c.track=t;c.rewindToTick(0);n.push(c.element.articulations.length);}return n.join(',');})()").toString();
        };
        QQmlComponent debugComponent(engine);
        debugComponent.setData("import MuseScore.NotationScene; FeatureDebugModel {}",QUrl());
        QScopedPointer<QObject> debug(debugComponent.create());
        if(!debug || !QMetaObject::invokeMethod(debug.data(),"load"))qFatal("Percussion diagnostics model unavailable");
        const auto diagnostic = [&debug] {
            QMetaObject::invokeMethod(debug.data(),"refresh");
            return QJsonDocument::fromJson(debug->property("report").toString().toUtf8()).object().value("sticking").toArray();
        };
        select(0);
        auto rows=diagnostic();
        if(rows.size()!=1 || rows[0].toObject().value("unpitchedPercussion").toBool()
           || rows[0].toObject().value("articulations").toArray()[0].toObject().value("effectiveAnchor").toInt()!=2)
            qFatal("Percussion default still overrides the pitched-staff accent");
        select(4); rows=diagnostic();
        if(rows.size()!=1 || !rows[0].toObject().value("unpitchedPercussion").toBool()
           || !rows[0].toObject().value("articulations").toArray()[0].toObject().value("renderedAbove").toBool())
            qFatal("Unpitched percussion did not retain its automatic above accent");
        if(dockOpen(window,"propertiesPanel")) {
            dispatchMenu(window,"command://app/dock/toggle-properties");QTest::qWait(100);
        }
        dispatchMenu(window,"command://app/dock/toggle-properties");
        QTest::qWait(200);
        QPointer<QQuickItem> panel=findItem(window->contentItem(),"ArticulationSettings");
        if(!panel || !panel->isVisible())qFatal("Actual articulation Properties panel is unavailable");
        QQuickItem* below=nullptr;
        for(auto* object:objects(panel)) {
            auto* item=qobject_cast<QQuickItem*>(object);
            if(item && item->isVisible() && item->property("text").toString()=="Below" && QByteArray(item->metaObject()->className()).contains("FlatRadioButton"))below=item;
        }
        if(!below)qFatal("Native Below articulation placement control unavailable");
        reveal(below);QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,below->mapToScene({below->width()/2,below->height()/2}).toPoint());QTest::qWait(150);
        rows=diagnostic();
        auto marking=rows[0].toObject().value("articulations").toArray()[0].toObject();
        if(marking.value("effectiveAnchor").toInt()!=1 || marking.value("renderedAbove").toBool())qFatal("Pointer Below did not override percussion placement");
        panel=findItem(window->contentItem(),"ArticulationSettings");
        if(!panel)qFatal("Articulation Properties disappeared after placement edit");
        auto direction=findItem(panel,"articulation-symbol-direction");
        if(!direction)qFatal("Symbol direction controls missing");
        QQmlExpression expression(qmlContext(direction),direction,"radioButtonGroup.itemAtIndex(1)");
        auto* down=qvariant_cast<QQuickItem*>(expression.evaluate());
        if(expression.hasError() || !down)qFatal("Symbol Down button unavailable");
        reveal(down);QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,down->mapToScene({down->width()/2,down->height()/2}).toPoint());QTest::qWait(150);
        marking=diagnostic()[0].toObject().value("articulations").toArray()[0].toObject();
        if(marking.value("symbolDirection").toInt()!=2 || marking.value("renderedAbove").toBool())qFatal("Pointer symbol direction did not apply");
        panel=findItem(window->contentItem(),"ArticulationSettings");
        if(!panel)qFatal("Articulation Properties disappeared after symbol edit");
        auto* voice2=findItem(panel,"articulation-voice-2");auto* voice3=findItem(panel,"articulation-voice-3");
        if(!voice2 || !voice2->isEnabled() || !voice3 || voice3->isEnabled()) {
            qWarning()<<"PERCUSSION INSPECTOR VOICE DIAGNOSTICS"<<QJsonDocument(QJsonObject{{"chords",diagnostic()}}).toJson(QJsonDocument::Compact)
                      <<"counts"<<markCounts()<<"voice2"<<voice2<<"enabled"<<(voice2 && voice2->isEnabled())
                      <<"voice3"<<voice3<<"enabled"<<(voice3 && voice3->isEnabled());
            if(auto* model=qvariant_cast<QObject*>(panel->property("model")))
                qWarning()<<"PERCUSSION INSPECTOR PROPERTIES VOICE MODEL"<<model->property("voiceChoices")<<model->property("currentVoice");
            qFatal("Voice targets do not match existing chords");
        }
        const auto clickVoice=[window](QQuickItem* button) {reveal(button);QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,button->mapToScene({button->width()/2,button->height()/2}).toPoint());QTest::qWait(150);};
        clickVoice(voice2);
        if(markCounts()!="0,1")qFatal("Voice button moved notes, copied an accent or left its old attachment");
        rows=diagnostic();
        if(rows[0].toObject().value("voice").toInt()!=2 || rows[0].toObject().value("articulations").toArray()[0].toObject().value("voice").toInt()!=2)
            qFatal("Native accent and owning chord voices disagree");
        dispatchMenu(window,"command://notation/undo");QTest::qWait(150);if(markCounts()!="1,0")qFatal("Accent move did not Undo in one step");
        dispatchMenu(window,"command://notation/redo");QTest::qWait(150);if(markCounts()!="0,1")qFatal("Accent move did not Redo");
        evaluate("curScore.startCmd();curScore.appendPart('marching-snare');curScore.appendPart('marching-cymbals');curScore.endCmd();");QTest::qWait(200);
        if(evaluate("curScore.nstaves").toInt()!=4)qFatal("Native template append did not create two staves");
        select(5);
        dispatchMenu(window,"command://project/save");QTest::qWait(500);
        // Leave the controls after the last real click. A normal hover tooltip
        // is a second QWindow; it must not obscure the capture or be mistaken
        // for a startup dialog by the independent desktop-window check.
        QTest::mouseMove(window,QPoint(window->width()*3/4,window->height()/2));
        QTest::qWait(500);
        const auto capture=qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE");
        if(!capture.isEmpty() && !window->grabWindow().save(capture))qFatal("Percussion Properties capture failed");
        qWarning()<<"PERCUSSION INSPECTOR pitched/percussion scope, real placement/direction/voice clicks, Undo/Redo, new templates and save passed";
    }
    static void verifyKeypad(QQuickWindow* window) {
        dispatchMenu(window, "command://notation/select-all");
        QQmlEngine* hostEngine = qmlEngine(window->contentItem());
        if (!hostEngine) hostEngine = qmlEngine(findItem(window->contentItem(), "WindowContent"));
        if (!hostEngine) qFatal("Keypad host QML engine unavailable");
        auto* engine = new QQmlEngine(window);
        engine->setImportPathList(hostEngine->importPathList());
        const auto context = hostEngine->rootContext()->contextProperty("ioc_context");
        engine->rootContext()->setContextProperty("ioc_context", context);
        engine->setProperty("ioc_context", context);
        engine->rootContext()->setContextProperty("ui", hostEngine->rootContext()->contextProperty("ui"));
        engine->setProperty("apiversion", 1);
        QString pluginPath;
        QDirIterator files(QCoreApplication::applicationDirPath() + "/..", {"EvanScoreNoteInput.qml"}, QDir::Files, QDirIterator::Subdirectories);
        if (files.hasNext()) pluginPath = files.next();
        if (pluginPath.isEmpty()) qFatal("Bundled keypad plugin unavailable");
        QQmlComponent component(engine, QUrl::fromLocalFile(pluginPath));
        if (!component.isReady()) qFatal("Actual keypad QML failed: %s", qPrintable(component.errorString()));
        auto* plugin = component.create();
        if (!plugin) qFatal("Actual keypad creation failed: %s", qPrintable(component.errorString()));
        plugin->setParent(engine);
        if (!QMetaObject::invokeMethod(plugin, "run")) qFatal("Keypad Run unavailable");
        QTest::qWait(700);
        auto* keypad = plugin->findChild<QQuickWindow*>("evanscore-keypad");
        if (!keypad || !keypad->isVisible()) qFatal("Actual keypad window unavailable");
        const auto evaluate = [plugin](const QString& code) {
            QQmlExpression expression(qmlContext(plugin), plugin, code);
            const auto value = expression.evaluate();
            if (expression.hasError()) qFatal("Keypad expression failed: %s", qPrintable(expression.error().toString()));
            return value;
        };
        if(qEnvironmentVariable("EVANSCORE_LAYOUT_TRIAL")=="percussion") {
            keypad->close();
            verifyPercussion(window, engine, evaluate);
            delete engine;
            return;
        }
        if(qEnvironmentVariable("EVANSCORE_LAYOUT_TRIAL")=="stems") {
            const auto lengths=[&evaluate] {
                return QJsonDocument::fromJson(evaluate(R"JS((function(){
                    var c=curScore.newCursor(); c.rewind(0); var lengths=[];
                    while(c.segment){if(c.element && c.element.type===elementTypes.CHORD && c.element.stem)
                        lengths.push(c.element.stem.bbox.height); c.next();}
                    return JSON.stringify(lengths);
                })())JS").toString().toUtf8()).array();
            };
            const auto original=lengths();
            if(original.size()<20) qFatal("Stem fixture did not expose ordinary and beamed stems");
            if(evaluate("curScore.style.value('evanStemLengthScale')").toDouble()!=1.0)
                qFatal("Initial stem scale was not 100 percent");
            const auto dialogEdit=[window](int phase) {
                bool operated=false;
                QTimer::singleShot(500,window,[&operated,phase] {
                    QDialog* dialog=nullptr;
                    for(QWidget* w:QApplication::topLevelWidgets())
                        if(w->isVisible() && w->findChild<QSpinBox*>("stemLengthScale")) dialog=qobject_cast<QDialog*>(w);
                    if(!dialog) qFatal("Native Styles dialog/stem control unavailable");
                    dialog->setProperty("currentPageCode","notes"); QTest::qWait(100);
                    auto* spin=dialog->findChild<QSpinBox*>("stemLengthScale");
                    auto* shorten=dialog->findChild<QGroupBox*>("shortenStem");
                    auto* buttons=dialog->findChild<QDialogButtonBox*>("buttonBox");
                    if(!spin->isVisible() || !spin->isEnabled() || !shorten || !buttons)
                        qFatal("Stem scale is hidden, disabled or inside conditional shortening");
                    if(phase==0) {
                        if(spin->value()!=100) qFatal("Styles did not show 100 percent");
                        if(shorten->isChecked()) QTest::mouseClick(shorten,Qt::LeftButton,Qt::NoModifier,QPoint(12,10));
                        if(shorten->isChecked() || !spin->isEnabled()) qFatal("Global scale depends on Shorten stems");
                        spin->setFocus(); QTest::keyClick(spin,Qt::Key_A,Qt::ControlModifier); QTest::keyClicks(spin,"80"); QTest::keyClick(spin,Qt::Key_Tab);
                        QTest::qWait(150);
                        const auto capture=qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE");
                        if(!capture.isEmpty() && !dialog->grab().save(capture+".style.png")) qFatal("Style capture failed");
                    } else {
                        if(spin->value()!=80 || shorten->isChecked()) qFatal("Native Styles lost the saved global stem settings");
                        if(phase==1) {
                            auto* reset=dialog->findChild<QToolButton*>("resetStemLengthScale");
                            QTest::mouseClick(reset,Qt::LeftButton); QTest::qWait(50);
                            if(spin->value()!=100) qFatal("Stem scale reset did not restore 100 percent");
                        } else {
                            auto* makeDefault=dialog->findChild<QPushButton*>("makeDefaultStyleButton");
                            if(!makeDefault) qFatal("Make Default Style unavailable");
                            QTest::mouseClick(makeDefault,Qt::LeftButton); QTest::qWait(100);
                            if(!makeDefault->text().contains("Saved")) qFatal("Could not save default stem style");
                        }
                    }
                    operated=true;
                    QTest::mouseClick(buttons->button(phase==1 ? QDialogButtonBox::Cancel : QDialogButtonBox::Ok),Qt::LeftButton);
                });
                dispatchMenu(window,"command://notation/open-edit-style");
                for(int step=0;step<40 && !operated;++step) QTest::qWait(50);
                QTest::qWait(200);
                if(!operated) qFatal("Native Styles edit did not run");
            };
            dialogEdit(0);
            const auto shorter=lengths();
            if(shorter.size()!=original.size()) qFatal("Stem style changed notation count");
            for(int i=0;i<original.size();++i)
                if(!(shorter[i].toDouble()>0 && shorter[i].toDouble()<original[i].toDouble()))
                    qFatal("Global scale did not shorten stem %d: %f -> %f",i,original[i].toDouble(),shorter[i].toDouble());
            dialogEdit(1);
            if(lengths()!=shorter) qFatal("Cancel did not restore the 80-percent stems");
            evaluate("cmd('command://notation/undo')"); QTest::qWait(200);
            if(lengths()!=original) qFatal("Global stem style did not undo in one step");
            evaluate("cmd('command://notation/redo')"); QTest::qWait(200);
            if(lengths()!=shorter) qFatal("Global stem style did not redo");
            dialogEdit(2);
            QFile styleIdentity(QFileInfo(qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE")).absolutePath()+"/default-style.json");
            if(!styleIdentity.open(QIODevice::WriteOnly)) qFatal("Cannot retain saved default style path");
            styleIdentity.write(QJsonDocument(QJsonObject{{"path",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                +"/"+QCoreApplication::applicationName()+"/Styles/EvanScore-Default.mss"}}).toJson());
            styleIdentity.close();
            dispatchMenu(window,"command://project/save"); QTest::qWait(500);
            qWarning() << "STEMS INSPECTOR real Styles pointer/typing, independent shortening, all stems, cancel, reset, undo, redo, default and save passed";
            keypad->close(); return;
        }
        QFile keypadSource(pluginPath); if(!keypadSource.open(QIODevice::ReadOnly)) qFatal("Cannot read tested keypad source");
        const auto keypadText=keypadSource.readAll();
        qWarning() << "KEYPAD INSPECTOR source identity" << pluginPath << plugin->property("version")
                   << "Tie transaction present" << keypadText.contains("Toggle tie");
        QFile keypadIdentity(QFileInfo(qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE")).absolutePath()+"/keypad-source.json");
        if(!keypadIdentity.open(QIODevice::WriteOnly)) qFatal("Cannot retain keypad source identity");
        keypadIdentity.write(QJsonDocument(QJsonObject{{"path",pluginPath},{"version",plugin->property("version").toString()},
            {"tieTransaction",keypadText.contains("Toggle tie")},{"requestedSource",qEnvironmentVariable("EVANSCORE_KEYPAD_SOURCE")}}).toJson());
        keypadIdentity.close();
        if(qEnvironmentVariable("EVANSCORE_KEYPAD_SOURCE")=="checkout" && plugin->property("version").toString()!="1.6")
            qFatal("Keypad overlay did not load the expected plugin version");
        qWarning() << "KEYPAD INSPECTOR actual selection" << evaluate("selectedNotes().length")
                   << "chords" << evaluate("selectedChords().length")
                   << "single element enum" << evaluate("elementTypes.TREMOLO_SINGLECHORD")
                   << "buzz enum" << evaluate("tremoloTypes.BUZZ_ROLL");
        auto* buzz = findItem(keypad->contentItem(), "keypad-key-buzz");
        if (!buzz) qFatal("Buzz button missing");
        QTest::mouseClick(keypad, Qt::LeftButton, Qt::NoModifier, buzz->mapToScene(QPointF(buzz->width()/2, buzz->height()/2)).toPoint());
        QTest::qWait(200);
        qWarning() << "KEYPAD INSPECTOR buzz click" << plugin->property("activeTremolo")
                   << "notice" << plugin->property("notice")
                   << "native chords" << evaluate("JSON.stringify(selectedChords().map(function(c) { return {track:c.track, tremolo: c.tremoloSingleChord ? c.tremoloSingleChord.tremoloType : null}; }))");
        if (plugin->property("activeTremolo").toString() != "BUZZ_ROLL") qFatal("Actual buzz button did not add buzz rolls");
        qWarning() << "KEYPAD INSPECTOR buzz real click passed";
        const auto click = [keypad](const QString& name) {
            auto* key = findItem(keypad->contentItem(), name);
            if (!key || !key->isVisible() || !key->isEnabled()) qFatal("Keypad key is not clickable: %s", qPrintable(name));
            QTest::mouseClick(keypad, Qt::LeftButton, Qt::NoModifier,
                key->mapToScene(QPointF(key->width()/2, key->height()/2)).toPoint());
            QTest::qWait(100);
        };
        const auto choose = [&evaluate, &click](const QString& id) {
            // Exercise customization's actual catalog/tool bindings. The
            // operation still comes from a real pointer click on the key.
            evaluate("layoutOverrides = {notes:{r0c0:'" + id + "'}}; activePage=0");
            QTest::qWait(80);
            click("keypad-key-" + id);
        };
        const auto selectTick = [&evaluate](int tick) {
            if (!evaluate(QString("(function(){var c=curScore.newCursor();c.rewindToTick(%1);return curScore.selection.select(c.element.type === elementTypes.CHORD ? c.element.notes[0] : c.element);})()").arg(tick)).toBool())
                qFatal("Native keypad selection failed at tick %d", tick);
            evaluate("syncSelection()"); QTest::qWait(50);
        };
        const QString snapshotCode = R"JS((function(){
            var c=curScore.newCursor(); c.rewind(0); var rows=[];
            for(var count=0;c.segment && count<100;++count){
                var e=c.element;
                if(e){
                    var row={tick:c.fraction.ticks,type:e.type,duration:e.duration.numerator+'/'+e.duration.denominator,beam:e.beamMode,stem:e.stemDirection};
                    if(e.type===elementTypes.CHORD){
                        row.notes=Array.from(e.notes).map(function(n){return {pitch:n.pitch,tpc:n.tpc,head:n.headType,dots:n.dots.length,tie:!!n.tieForward,accidental:n.accidental ? n.accidental.accidentalType : null};});
                        row.articulations=Array.from(e.articulations).map(function(a){return a.symbol;});
                        row.grace=Array.from(e.graceNotes).map(function(g){return {type:g.noteType,duration:g.duration.numerator+'/'+g.duration.denominator,tremolo:g.tremoloSingleChord ? g.tremoloSingleChord.tremoloType : null};});
                        row.tremolo=e.tremoloSingleChord ? e.tremoloSingleChord.tremoloType : null;
                    }
                    row.fermata=Array.from(c.segment.annotations).filter(function(a){return a.type===elementTypes.FERMATA;}).map(function(a){return a.track;});
                    rows.push(row);
                }
                c.next();
            }
            return JSON.stringify({rows:rows,spanners:Array.from(curScore.spanners).map(function(s){return s.type;})});
        })())JS";
        const auto snapshot = [&evaluate, &snapshotCode] { return evaluate(snapshotCode).toString(); };
        click("keypad-undo"); QTest::qWait(100);
        const auto baseline = snapshot();
        for (const auto id : {"diddle", "tremolo2", "roll", "tremolo4", "buzz"}) {
            dispatchMenu(window, "command://notation/select-all");
            choose(id);
            const QString type = evaluate(QString("catalogById['%1'].tremoloName").arg(id)).toString();
            if (!evaluate("selectedChords().length>0 && selectedChords().every(function(c){return c.tremoloSingleChord && c.tremoloSingleChord.tremoloType===tremoloTypes['"+type+"'];})").toBool())
                qFatal("Native roll type did not apply: %s", id);
            click("keypad-undo"); if (snapshot()!=baseline) qFatal("Roll did not undo in one step: %s", id);
            click("keypad-redo");
            choose(id);
            if (!evaluate("selectedChords().every(function(c){return !c.tremoloSingleChord;})").toBool()) qFatal("Matching roll did not toggle off: %s", id);
            click("keypad-undo"); click("keypad-undo");
            if(snapshot()!=baseline) qFatal("Roll toggle cycle changed other notation: %s",id);
            qWarning() << "KEYPAD INSPECTOR actual add, undo, redo, toggle" << id << "passed";
        }
        dispatchMenu(window, "command://notation/select-all"); choose("buzz");
        if (!evaluate("curScore.selection.select(selectedChords()[0].tremoloSingleChord)").toBool()) qFatal("Native roll selection failed");
        evaluate("syncSelection()");
        if(evaluate("selectedChords().length").toInt()!=1) qFatal("Selecting a roll loses its owning chord in the keypad");
        choose("roll");
        if(!evaluate("selectedChords()[0].tremoloSingleChord.tremoloType===tremoloTypes.R32").toBool()) qFatal("Selected buzz could not change to three-slash roll");
        click("keypad-undo"); click("keypad-undo");
        if(snapshot()!=baseline) qFatal("Selected roll edit did not undo cleanly");
        qWarning() << "KEYPAD INSPECTOR selected roll resolves owning chord passed";
        // User workflows differ from Select All: single notes, disjoint notes,
        // partial ranges, and ranges containing rests must all retain their scope.
        for (const auto selection : {"single", "disjoint", "range", "rests"}) {
            selectTick(1920);
            if (QByteArray(selection)=="disjoint")
                evaluate("(function(){var c=curScore.newCursor();c.rewindToTick(2400);if(!curScore.selection.select(c.element.notes[0],true))throw Error('disjoint selection');})()");
            else if (QByteArray(selection)=="range" || QByteArray(selection)=="rests") {
                const int end = QByteArray(selection)=="range" ? 2640 : 7680;
                const auto rangePosition = [&evaluate] {
                    return evaluate("JSON.stringify({range:curScore.selection.isRange,start:curScore.selection.startSegment ? curScore.selection.startSegment.fraction.ticks : null,end:curScore.selection.endSegment ? curScore.selection.endSegment.fraction.ticks : null})").toString();
                };
                if(evaluate("curScore.selection.isRange || selectedChords().length!==1 || selectedChords()[0].fraction.ticks!==1920").toBool())
                    qFatal("Keypad range setup did not start from the requested single note");
                // Use the same native range-extension command as Shift+Right.
                // selectRange's plugin wrapper currently leaves a stale element
                // list until a later update, which is a separate API regression.
                for(int step=0;step<16;++step) {
                    const auto before=rangePosition();
                    evaluate("cmd('select-next-chord')");
                    // Legacy action dispatch can complete on a later event-loop
                    // turn. Do not enqueue another extension while observing
                    // the old endpoint, which can overshoot the intended range.
                    QElapsedTimer deadline;deadline.start();
                    while(rangePosition()==before && deadline.elapsed()<2000)QTest::qWait(10);
                    if(rangePosition()==before)qFatal("Keypad native range extension did not complete");
                    if(evaluate(QString("curScore.selection.isRange && (!curScore.selection.endSegment || curScore.selection.endSegment.fraction.ticks >= %1)").arg(end)).toBool()) break;
                }
                if(!evaluate(QString("curScore.selection.isRange && curScore.selection.startSegment.fraction.ticks===1920 && curScore.selection.endSegment.fraction.ticks===%1").arg(end)).toBool()) {
                    qWarning()<<"KEYPAD INSPECTOR RANGE DIAGNOSTICS"<<evaluate("JSON.stringify({range:curScore.selection.isRange,start:curScore.selection.startSegment ? curScore.selection.startSegment.fraction.ticks : null,end:curScore.selection.endSegment ? curScore.selection.endSegment.fraction.ticks : null,elements:Array.from(curScore.selection.elements).map(function(e){return {type:e.type,track:e.track};}),chords:selectedChords().length})").toString();
                    qFatal("Keypad native partial range selection failed");
                }
            }
            const int count=evaluate("selectedChords().length").toInt();
            const int expected=QByteArray(selection)=="single" ? 1 : QByteArray(selection)=="disjoint" ? 2 : QByteArray(selection)=="range" ? 3 : 9;
            if(count!=expected) qFatal("Keypad selected scope %s expected %d chords, got %d",selection,expected,count);
            for(const auto id : {"buzz", "diddle", "tremolo2", "roll", "tremolo4"}) {
                choose(id);
                if(!evaluate("selectedChords().every(function(c){return !!c.tremoloSingleChord;})").toBool())
                    qFatal("Keypad roll skipped selected notes in %s",selection);
                const auto changed=QJsonDocument::fromJson(snapshot().toUtf8()).object().value("rows").toArray();
                int rolled=0; for(const auto& row:changed) if(row.toObject().contains("tremolo") && !row.toObject().value("tremolo").isNull()) ++rolled;
                if(rolled!=expected) qFatal("Keypad roll escaped its selected scope in %s: %d",selection,rolled);
                click("keypad-undo"); if(snapshot()!=baseline) qFatal("Keypad partial roll Undo failed in %s",selection);
                click("keypad-redo"); choose(id); click("keypad-undo"); click("keypad-undo");
                if(snapshot()!=baseline) qFatal("Keypad partial roll toggle cycle failed in %s",selection);
            }
            qWarning() << "KEYPAD INSPECTOR scoped real roll clicks" << selection << "passed";
        }
        selectTick(1920); choose("flam");
        if(!evaluate("(function(){var c=curScore.newCursor();c.rewindToTick(1920);return curScore.selection.select(c.element.graceNotes[0].notes[0]);})()").toBool())
            qFatal("Keypad grace note selection failed");
        const auto graceBaseline=snapshot();
        choose("buzz");
        if(!evaluate("selectedChords().length===1 && selectedChords()[0].tremoloSingleChord && !selectedChords()[0].parent.tremoloSingleChord").toBool())
            qFatal("Keypad grace roll did not stay on selected grace chord");
        click("keypad-undo"); if(snapshot()!=graceBaseline) qFatal("Grace roll Undo failed");
        click("keypad-undo"); if(snapshot()!=baseline) qFatal("Grace roll cycle changed other notation");
        qWarning() << "KEYPAD INSPECTOR single, disjoint, partial range, rests and grace rolls passed";
        if(qEnvironmentVariableIsSet("EVANSCORE_KEYPAD_ROLLS_ONLY")) {
            dispatchMenu(window,"command://notation/select-all"); choose("buzz");
            dispatchMenu(window,"command://project/save"); QTest::qWait(500);
            qWarning() << "KEYPAD INSPECTOR snare existing-note rolls, scoped undo and save passed";
            keypad->close(); return;
        }
        for(const auto id : {"whole","half","quarter","eighth","sixteenth","thirtysecond","sixtyfourth","breve","dot","dot2","dot3","rest",
                             "natural","sharp","flat","flat2","sharp2","flip","pitchup","pitchdown","octaveup","octavedown",
                             "accent","tenuto","marcato","staccato","staccatissimo","accenttenuto","accentstaccato","softaccent","stress","unstress",
                             "flam","appoggiatura","grace4","grace16","grace32","grace8after","grace16after","grace32after","openheads","delete",
                             "beamstart","beamjoin","beamnone","beambreak8","beambreak16"}) {
            const bool beam=QByteArray(id).startsWith("beam");
            // Secondary beams require sixteenths; tertiary beams require
            // thirty-seconds. Native eighth notes correctly reject both.
            selectTick(QByteArray(id)=="beambreak8" ? 7800 : QByteArray(id)=="beambreak16" ? 8220
                       : beam ? 2160 : QByteArray(id)=="half" ? 960 : 0);
            choose(id);
            if(snapshot()==baseline) qFatal("Actual keypad button had no score effect: %s",id);
            click("keypad-undo");
            if(snapshot()!=baseline) qFatal("Actual keypad button did not undo once: %s",id);
            qWarning() << "KEYPAD INSPECTOR actual notation and one-step undo" << id << "passed";
        }
        selectTick(2160); choose("beamnone"); choose("beamauto");
        if(evaluate("selectedChords()[0].beamMode").toInt()!=0) qFatal("Automatic beaming did not restore Auto");
        click("keypad-undo"); click("keypad-undo");
        selectTick(2160); choose("fermata");
        if(!evaluate("selectedChords()[0].parent.annotations.length>0 && Array.from(selectedChords()[0].parent.annotations).some(function(a){return a.type===elementTypes.FERMATA;})").toBool())
            qFatal("Fermata landed on the wrong segment");
        click("keypad-undo"); if(snapshot()!=baseline) qFatal("Fermata did not undo cleanly");
        QQmlComponent historyComponent(engine);
        historyComponent.setData("import MuseScore.NotationScene; UndoHistoryModel {}", QUrl());
        QScopedPointer<QObject> historyObject(historyComponent.create());
        auto* history=qobject_cast<QAbstractItemModel*>(historyObject.data());
        if(!history) qFatal("Keypad native history model unavailable");
        const auto historyState=[history] {
            const int index=history->property("currentIndex").toInt();
            return QString("index=%1 rows=%2 action=%3").arg(index).arg(history->rowCount()).arg(history->data(history->index(index,0),Qt::DisplayRole).toString());
        };
        selectTick(0);
        qWarning() << "KEYPAD INSPECTOR before tie" << historyState();
        choose("tie");
        qWarning() << "KEYPAD INSPECTOR after tie" << historyState();
        if(!evaluate("selectedNotes()[0].tieForward !== null").toBool()) qFatal("Tie button did not create a tie");
        auto* undoButton=findItem(keypad->contentItem(),"keypad-undo");
        QSignalSpy undoClick(undoButton,SIGNAL(clicked()));
        click("keypad-undo");
        qWarning() << "KEYPAD INSPECTOR after tie undo" << historyState() << "pointer clicks" << undoClick.count();
        if(snapshot()!=baseline) {
            qWarning().noquote() << "KEYPAD INSPECTOR TIE BASELINE" << baseline;
            qWarning().noquote() << "KEYPAD INSPECTOR TIE AFTER UNDO" << snapshot();
            evaluate("cmd('command://notation/undo')"); QTest::qWait(100);
            qWarning() << "KEYPAD INSPECTOR after diagnostic direct undo" << historyState() << "matches baseline" << (snapshot()==baseline);
            qFatal("Tie did not undo cleanly");
        }
        if(qEnvironmentVariable("EVANSCORE_KEYPAD_SOURCE")=="bundled" && plugin->property("version").toString()=="1.6") {
            selectTick(0);
            QSignalSpy historyChanged(history,SIGNAL(currentIndexChanged()));
            // Bypass the keypad transaction to verify the native app fix too.
            evaluate("cmd('tie')"); QTest::qWait(100);
            if(historyChanged.count()==0) qFatal("Native Tie did not publish its completed undo state");
            click("keypad-undo"); if(snapshot()!=baseline) qFatal("Native Tie command still ignores Undo");
            click("keypad-redo");
            if(snapshot()==baseline) qFatal("Native Tie command did not Redo");
            click("keypad-undo"); if(snapshot()!=baseline) qFatal("Native Tie command did not restore baseline");
            qWarning() << "KEYPAD INSPECTOR native tie history notification, undo and redo passed";
        }
        selectTick(0); choose("slur");
        if(evaluate("curScore.spanners.length").toInt()!=1) qFatal("Slur button did not create a spanner");
        // Cancel slur editing before invoking native Undo.
        choose("escape"); click("keypad-undo"); if(snapshot()!=baseline) qFatal("Slur did not undo cleanly");
        qWarning() << "KEYPAD INSPECTOR beam Auto, correct-segment fermata, tie and slur passed";
        evaluate("layoutOverrides={}; activePage=0"); QTest::qWait(100);
        for(int tab=0;tab<5;++tab) {
            click(QString("keypad-tab-%1").arg(tab));
            keypad->resize(208, keypad->minimumHeight()); QTest::qWait(100);
            if(keypad->width()!=208 || !plugin->property("compact").toBool()) qFatal("Keypad refused compact resizing");
            auto* grid=findItem(keypad->contentItem(),"keypad-grid");
            for(auto* button:grid->childItems()) {
                if(!button->objectName().startsWith("keypad-key-") || !button->isVisible()) continue;
                if(button->width()<35 || button->height()<24 || button->y()<0 || button->x()<0
                    || button->x()+button->width()>grid->width()+.5 || button->y()+button->height()>grid->height()+.5)
                    qFatal("Compact keypad hides a button: %s",qPrintable(button->objectName()));
            }
        }
        click("keypad-tab-0"); keypad->resize(208,keypad->minimumHeight()); QTest::qWait(100);
        const auto capturePath=qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE");
        if(!capturePath.isEmpty() && !keypad->grabWindow().save(capturePath)) qFatal("Keypad capture failed");
        dispatchMenu(window,"command://notation/select-all"); choose("buzz");
        selectTick(2160); choose("fermata");
        dispatchMenu(window,"command://project/save"); QTest::qWait(500);
        qWarning() << "KEYPAD INSPECTOR all notation buttons, undo and compact tabs passed";
        keypad->close();
        delete engine;
    }
    static void verifyMallets(QQuickWindow* window) {
        dispatchMenu(window, "command://notation/select-all");
        if(!dockOpen(window,"malletPanel")) dispatchMenu(window, "command://app/dock/toggle-mallet");
        QTest::qWait(700);
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
        QPointer<QQuickItem> panel = findItem(window->contentItem(), "MalletPanel");
        if (!panel || panel->height() > 221 || panel->height() < 100) qFatal("Mallet panel did not open compactly");
        const auto source = original.value("sourceKey");
        if (original.value("sticking").toString().isEmpty()) {
            auto apply=findItem(panel,"mallet-commit");
            if(!apply || !apply->isEnabled())qFatal("The original suggested sticking cannot be applied");
            QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,apply->mapToScene({apply->width()/2,apply->height()/2}).toPoint());QTest::qWait(120);
            if(state().value("sticking").toString().isEmpty() || state().value("pose").toMap().value("pitches")!=original.value("pose").toMap().value("pitches"))qFatal("Applying original sticking changed notes or did nothing");
            dispatchMenu(window,"command://notation/undo");QTest::qWait(120);
            if(state().value("sourceKey")!=source)qFatal("Original sticking did not undo in one step");
            dispatchMenu(window,"command://notation/redo");QTest::qWait(120);
            if(state().value("sticking").toString().isEmpty())qFatal("Original sticking did not redo");
            dispatchMenu(window,"command://notation/undo");QTest::qWait(120);
        }
        verifyMalletSuggestionClicks(window,panel,model);
        bool reassigned=false;
        for(auto value:model->property("alternatives").toList()) {
            const auto row=value.toMap();
            if(!row.value("keepsSticking").toBool()) reassigned=true;
            if(reassigned && row.value("keepsSticking").toBool()) qFatal("Written-link alternatives came after reassignment");
            if(row.value("pros").toStringList().empty() || row.value("cons").toStringList().empty()) qFatal("Alternative lacks concrete pros/cons");
        }
        auto edge=findItem(panel,"mallet-strike-edge");
        auto center=findItem(panel,"mallet-strike-center");
        if(!edge || !center || !edge->isVisible()) qFatal("Manual strike controls unavailable");
        QSignalSpy edgeClick(edge,SIGNAL(clicked(QVariant)));
        if (!edgeClick.isValid()) qFatal("Native mallet button click signal is unavailable");
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,edge->mapToScene({edge->width()/2,edge->height()/2}).toPoint());QTest::qWait(100);
        auto voices=state().value("pose").toMap().value("voices").toList();
        qWarning() << "MALLET INSPECTOR edge click" << edgeClick.count() << "selected" << panel->property("selectedMallet") << "voices" << voices << "notice" << state().value("notice");
        if(edgeClick.count()!=1 || voices.empty() || voices.front().toMap().value("fraction").toDouble()<.85 || state().value("sourceKey")!=source) qFatal("Real edge click failed or changed score");
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,center->mapToScene({center->width()/2,center->height()/2}).toPoint());QTest::qWait(100);
        QMetaObject::invokeMethod(model,"resetStrikePoints");
        panel->setProperty("currentTab", 2); QTest::qWait(100);
        auto aiToggle=findItem(panel,"mallet-local-ai-enabled");
        if(!aiToggle || model->property("assistant").toMap().value("enabled").toBool())qFatal("Local AI must start disabled");
        reveal(aiToggle);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,aiToggle->mapToScene({10,aiToggle->height()/2}).toPoint());QTest::qWait(100);
        auto aiState=model->property("assistant").toMap();
        if(!aiState.value("enabled").toBool())qFatal("Pointer click on the optional AI checkbox did not enable it");
        if(aiState.value("busy").toBool())qFatal("Enabling optional AI must not start inference or discovery");
        auto aiSetup=findItem(panel,"mallet-local-ai-setup");
        if(!aiSetup || !aiSetup->isEnabled())qFatal("Explicit local AI setup action is unavailable");
        QFile setupData(":/evanscore/ai/mallet-ai-setup.ps1");
        if(!setupData.open(QIODevice::ReadOnly) || !setupData.readAll().contains("e359637575302f288d76c0007c408a4be9c7376d59c4f28684d63b232a44b427"))
            qFatal("The complete app does not expose its verified embedded local AI setup script");
        auto aiAsk=findItem(panel,"mallet-local-ai-ask");
        if(!aiAsk || aiAsk->isEnabled())qFatal("Local AI must not infer without an installed selected model");
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,aiToggle->mapToScene({10,aiToggle->height()/2}).toPoint());QTest::qWait(100);
        if(model->property("assistant").toMap().value("enabled").toBool())qFatal("Optional AI toggle did not turn off");
        QMetaObject::invokeMethod(model,"setOption",Q_ARG(QString,"inwardPreference"),Q_ARG(QVariant,QVariant(0)));
        if(state().value("inwardPreference").toInt()!=0)qFatal("Neutral rotation preference was not retained");
        QMetaObject::invokeMethod(model,"setOption",Q_ARG(QString,"inwardPreference"),Q_ARG(QVariant,QVariant(15)));
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
        QMetaObject::invokeMethod(model,"setOption",Q_ARG(QString,"keepBass"),Q_ARG(QVariant,QVariant(false)));
        QMetaObject::invokeMethod(model,"setOption",Q_ARG(QString,"keepMelody"),Q_ARG(QVariant,QVariant(false)));
        int octave = -1;
        for (auto value : model->property("alternatives").toList()) {
            const auto row = value.toMap();
            if (row.value("valid").toBool() && row.value("pitches").toString() != original.value("pose").toMap().value("pitchText").toString()) { octave = row.value("index").toInt(); break; }
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
        const auto picked=state().value("pose").toMap();
        const auto ids=picked.value("mallets").toList();
        if(ids.size()!=4 || ids != QVariantList{1,2,3,4}) qFatal("Ordinary C chord did not retain sensible physical head order");
        QMetaObject::invokeMethod(model,"setStrikePoint",Q_ARG(int,1),Q_ARG(double,.86));
        if(state().value("pose").toMap().value("pitches")!=picked.value("pitches") || state().value("pose").toMap().value("mallets")!=picked.value("mallets")) qFatal("Manual strike point changed pitches or identity");
        QMetaObject::invokeMethod(model,"resetStrikePoints");
        auto focusView=findItem(panel,"mallet-instrument-view");
        if(!focusView || !focusView->property("focusPlacement").toBool()) qFatal("Mallet detail focus is unavailable");
        focusView->setProperty("focusPlacement",false);QTest::qWait(100);focusView->setProperty("focusPlacement",true);
        QMetaObject::invokeMethod(model, "setPickMode", Q_ARG(bool, false));
        if (state().value("skin") != appearance || state().value("sourceKey") != source) qFatal("Appearance or score changed during preview");
        QMetaObject::invokeMethod(model,"setOption",Q_ARG(QString,"keepBass"),Q_ARG(QVariant,QVariant(false)));
        QMetaObject::invokeMethod(model,"setOption",Q_ARG(QString,"keepMelody"),Q_ARG(QVariant,QVariant(false)));
        reportMemory("before mallet visibility loop");
        for (int cycle = 0; cycle < 20; ++cycle) {
            qWarning() << "MALLET INSPECTOR before close" << cycle << "open" << dockOpen(window,"malletPanel")
                       << "visible" << (panel && panel->isVisible()) << "active" << model->property("active");
            dispatchMenu(window, "command://app/dock/toggle-mallet"); QTest::qWait(100);
            qWarning() << "MALLET INSPECTOR after close" << cycle << "open" << dockOpen(window,"malletPanel")
                       << "visible" << (panel && panel->isVisible()) << "active" << (model ? model->property("active") : QVariant());
            if(dockOpen(window,"malletPanel")) qFatal("Native mallet dock did not close");
            if (model && model->property("active").toBool()) qFatal("Hidden mallet model retained subscriptions");
            dispatchMenu(window, "command://app/dock/toggle-mallet"); QTest::qWait(80);
            model = findModel();
            panel = findItem(window->contentItem(), "MalletPanel");
            if (!model || !panel || !model->property("active").toBool() || state().value("bars").toList().size() != 61) qFatal("Mallet panel did not reopen");
        }
        reportMemory("after mallet visibility loop");
        window->resize(1100, 760); QTest::qWait(150);
        QMetaObject::invokeMethod(model, "selectAlternative", Q_ARG(int, octave));
        const auto finalPreview=state().value("pose").toMap();
        const QJsonObject expected{{"sourceKey",source.toString()}, {"pitches",QJsonArray::fromVariantList(finalPreview.value("pitches").toList())},
            {"mallets",QJsonArray::fromVariantList(finalPreview.value("mallets").toList())},{"reverseNumbering",state().value("reverseNumbering").toBool()}};
        QFile expectedFile(QFileInfo(qEnvironmentVariable("EVANSCORE_FEATURE_DEBUG_CAPTURE")).absolutePath()+"/expected-mallet.json");
        if(!expectedFile.open(QIODevice::WriteOnly) || expectedFile.write(QJsonDocument(expected).toJson())<0)qFatal("Cannot retain the exact native mallet preview");
        expectedFile.close();
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
        if (instrumentView->width() < 200 || instrumentView->height() < 80 || panel->height() < 200
            || instrumentView->mapToItem(panel, QPointF(0, instrumentView->height())).y() > panel->height() + .5
            || instrumentView->property("scene").toMap().value("bars").toList().size() != 61)
            qFatal("Native mallet geometry hides the instrument/player or loses render inputs");
        for (const auto name : {"mallet-score-selection", "mallet-pick-bars", "mallet-commit", "Diagnostics", "Alternatives", "Player"}) {
            auto button = findItem(panel, name);
            if (!button) qFatal("Mallet caption button is unavailable: %s", name);
            const auto font = button->property("textFont").value<QFont>();
            const double captionWidth = QFontMetricsF(font).horizontalAdvance(button->property("text").toString());
            qWarning() << "MALLET INSPECTOR native caption" << name << "width" << button->width()
                       << "text width" << captionWidth;
            if (button->width() + .5 < captionWidth + 2 * button->property("margins").toDouble())
                qFatal("Native mallet caption is clipped: %s", name);
            for(QObject* child:objects(button)) {
                auto* label=qobject_cast<QQuickItem*>(child);
                if(!label || !label->isVisible() || label->property("text")!=button->property("text")
                    || !label->property("truncated").isValid()) continue;
                qWarning() << "MALLET INSPECTOR rendered caption" << name << "width" << label->width()
                           << "implicit" << label->implicitWidth() << "truncated" << label->property("truncated");
                if(label->property("truncated").toBool()) qFatal("Rendered mallet caption is truncated: %s",name);
            }
        }
        if (!capturePath.isEmpty() && !window->grabWindow().save(capturePath))
            qFatal("Could not capture the complete native mallet window");
        qWarning() << "MALLET INSPECTOR range, written links, preview, commit, undo, redo, pick, edge, focus and 20 reopen checks passed";
    }
    static void verifyMalletSuggestionClicks(QQuickWindow* window,QQuickItem* panel,QObject* model) {
        panel->setProperty("currentTab",1);QTest::qWait(80);
        auto explore=findItem(panel,"mallet-explore-alternatives");
        if(explore && explore->isVisible() && !panel->property("showOtherOptions").toBool()) {
            reveal(explore);QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,explore->mapToScene({explore->width()/2,explore->height()/2}).toPoint());QTest::qWait(80);
        }
        const auto alternatives=model->property("alternatives").toList();
        if(alternatives.empty())qFatal("Native fixture has no suggestion cards to test");
        auto scroll=findItem(panel,"mallet-information-scroll");if(!scroll)qFatal("Native suggestion scroll view is unavailable");
        const auto source=model->property("state").toMap().value("sourceKey");
        QSignalSpy rowsChanged(model,SIGNAL(alternativesChanged()));if(!rowsChanged.isValid())qFatal("Stable suggestion-list notification is unavailable");
        for(int i=0;i<alternatives.size();++i) {
            QPointer<QQuickItem> card=findItem(panel,QString("mallet-alternative-card-%1").arg(i));
            if(!card)qFatal("Native suggestion card is missing");
            const double top=card->mapToItem(scroll,{0,0}).y()+scroll->property("contentY").toDouble();
            scroll->setProperty("contentY",std::clamp(top,0.,std::max(0.,scroll->property("contentHeight").toDouble()-scroll->height())));QTest::qWait(50);
            // Click the descriptive body, not the small pitch-name button.
            QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,card->mapToScene({card->width()/2,45}).toPoint());QTest::qWait(60);
            const auto selected=model->property("state").toMap();
            if(!card || selected.value("selectedAlternative").toInt()!=i || selected.value("pose").toMap().value("pitchText")!=alternatives[i].toMap().value("pitches") || selected.value("sourceKey")!=source)
                qFatal("Clicking a suggestion card failed, rebuilt the card, or changed the source score");
            if(rowsChanged.count()!=0)qFatal("Selecting a suggestion rebuilt the list instead of retaining its cards");
        }
        auto original=findItem(panel,"mallet-original");if(!original)qFatal("Native Original button is missing");
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,original->mapToScene({original->width()/2,original->height()/2}).toPoint());QTest::qWait(80);
        if(model->property("state").toMap().value("selectedAlternative").toInt()!=-1)qFatal("Original button failed to restore the written chord");
        panel->setProperty("currentTab",0);QTest::qWait(80);
        qWarning()<<"MALLET INSPECTOR every suggestion real body click, stable cards, preview and original passed"<<alternatives.size();
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
        const auto runtime=data.value("runtime").toObject();
        const auto systemPercent=runtime.value("systemUsedPercent").toDouble(-1);
        const auto processPercent=runtime.value("processPhysicalMemoryPercent").toDouble(-1);
        auto memoryLabel=findItem(dialog->contentItem(),"feature-debug-memory");
        if(systemPercent<0 || systemPercent>100 || processPercent<=0 || processPercent>100
            || !memoryLabel || !memoryLabel->property("text").toString().contains("%"))
            qFatal("Feature diagnostics omitted labeled system/process memory percentages");
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
