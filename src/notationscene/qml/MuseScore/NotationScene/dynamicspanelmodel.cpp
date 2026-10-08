/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "dynamicspanelmodel.h"
#include <algorithm>
#include <cmath>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include "notation/styledefaultssettings.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "notation/inotationstyle.h"
#include "notation/inotationundostack.h"
#include "engraving/dom/dynamicsplayback.h"
#include "engraving/dom/dynamic.h"
#include "engraving/dom/hairpin.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/staff.h"
#include "engraving/types/typesconv.h"
using namespace mu::notation;
using namespace mu::engraving;
using namespace muse;
DynamicsPanelModel::DynamicsPanelModel(QObject* parent) : QObject(parent), Contextable(muse::iocCtxForQmlObject(this)) {}
void DynamicsPanelModel::load() {
    if (m_loaded) return;
    m_loaded = true;
    context()->currentNotationChanged().onNotify(this, [this]() { onNotationChanged(); });
    onNotationChanged();
}
void DynamicsPanelModel::onNotationChanged() {
    m_notationReceiver.async_disconnectAll();
    m_notation = context()->currentNotation();
    m_forceScore = true;
    m_notice.clear();
    if (m_notation) {
        m_notation->interaction()->selectionChanged().onNotify(&m_notationReceiver, [this]() {
            m_forceScore = false;
            if (!m_editing) emit stateChanged();
        });
        m_notation->notationChanged().onReceive(&m_notationReceiver, [this](const RectF&) { if (!m_editing) emit stateChanged(); });
    }
    emit stateChanged();
}
Score* DynamicsPanelModel::score() const { return m_notation ? m_notation->elements()->msScore() : nullptr; }
void DynamicsPanelModel::setScoreStyle(Sid id, const PropertyValue& value) { score()->masterScore()->undoChangeStyleVal(id, value); }
Dynamic* DynamicsPanelModel::dynamic() const {
    if (!m_notation || m_forceScore) return nullptr;
    auto e = m_notation->interaction()->selection()->element();
    return e && e->isDynamic() ? toDynamic(e) : nullptr;
}
Hairpin* DynamicsPanelModel::hairpin() const {
    if (!m_notation || m_forceScore) return nullptr;
    auto selection = m_notation->interaction()->selection();
    if (selection->isRange()) return nullptr;
    auto e = selection->element();
    if (e && e->isHairpin()) return toHairpin(e);
    if (e && e->isHairpinSegment()) return toHairpinSegment(e)->hairpin();
    return nullptr;
}
std::vector<Note*> DynamicsPanelModel::notes() const {
    if (!m_notation || m_forceScore || hairpin()) return {};
    return m_notation->interaction()->selection()->notes();
}
QVariantMap DynamicsPanelModel::state() const {
    auto s = score();
    if (!s) return { {"available", false}, {"scope", "score"}, {"title", tr("Open a score to edit dynamics")} };
    auto h = hairpin(); auto dyn = dynamic(); auto ns = notes();
    const bool unsupported = !m_forceScore && !h && !dyn && ns.empty() && m_notation->interaction()->selection()->element();
    const auto& st = DynamicsPlayback::profile(s);
    int shape = st.styleI(Sid::evanDynamicsCurveShape); double bend = st.styleD(Sid::evanDynamicsCurveBend);
    if (h) { if (h->dynamicsCurveShape() >= 0) shape = h->dynamicsCurveShape(); if (h->dynamicsCurveBend() >= -2.0) bend = h->dynamicsCurveBend(); }
    QVariantMap result {
        {"available", true}, {"scope", unsupported ? "none" : h ? "hairpin" : dyn ? "dynamic" : ns.empty() ? "score" : "notes"},
        {"title", unsupported ? tr("Select notes, a dynamic, or a hairpin") : h ? (h->isCrescendo() ? tr("Selected crescendo") : tr("Selected decrescendo")) : dyn ? tr("Selected dynamic marking") : ns.empty() ? tr("Full score") : tr("%n selected note(s)", nullptr, static_cast<int>(ns.size()))},
        {"notice", m_notice}, {"hasDefault", !settings()->value(SAVED_DEFAULT_DYNAMICS_PATH).toPath().empty()},
        {"enabled", st.styleB(Sid::evanDynamicsEnabled)}, {"battery", st.styleB(Sid::evanDynamicsBattery)},
        {"batteryHairpin", h && h->staff() && h->staff()->isDrumStaff(h->tick()) && st.styleB(Sid::evanDynamicsBattery)},
        {"shape", shape}, {"bend", bend}, {"startLevel", h ? DynamicsPlayback::endpoint(h, false) : 49}, {"endLevel", h ? DynamicsPlayback::endpoint(h, true) : 112},
        {"tapStartLevel", h ? DynamicsPlayback::endpoint(h, false, DynamicsPlayback::Tap) : 49},
        {"tapEndLevel", h ? DynamicsPlayback::endpoint(h, true, DynamicsPlayback::Tap) : 49},
        {"startDynamic", h ? h->dynamicsStartDynamic() : -1}, {"endDynamic", h ? h->dynamicsEndDynamic() : -1},
        {"startRole", h ? h->dynamicsStartRole() : 0}, {"endRole", h ? h->dynamicsEndRole() : 0},
        {"startVelocity", h ? h->dynamicsStartVelocity() : -1}, {"endVelocity", h ? h->dynamicsEndVelocity() : -1},
        {"isCrescendo", h && h->isCrescendo()},
        {"localCurve", h && (h->dynamicsCurveShape() >= 0 || h->dynamicsCurveBend() >= -2.0)}
    };
    if (dyn) {
        result["noteVelocity"] = dyn->preciseVelocity() >= 0 ? dyn->preciseVelocity() : 0; result["mixed"] = false;
        result["effectiveVelocity"] = dyn->preciseVelocity() >= 0 ? dyn->preciseVelocity() : DynamicsPlayback::level(s, dyn->dynamicType(), DynamicsPlayback::Normal); result["notePlay"] = dyn->playDynamic();
        result["localOverride"] = dyn->preciseVelocity() >= 0;
    }
    if (!ns.empty()) {
        const int value = ns.front()->userVelocity();
        const bool mixed = std::any_of(ns.begin(), ns.end(), [value, &ns](const Note* n) { return n->userVelocity() != value || n->getProperty(Pid::VELO_TYPE) != ns.front()->getProperty(Pid::VELO_TYPE) || n->getProperty(Pid::PLAY) != ns.front()->getProperty(Pid::PLAY); });
        result["noteVelocity"] = mixed ? -1 : value;
        result["mixed"] = mixed;
        result["notePlay"] = ns.front()->getProperty(Pid::PLAY).toBool();
        result["localOverride"] = value != 0 || !result["notePlay"].toBool();
        const QStringList categories { tr("Auto"), tr("Normal"), tr("Tap"), tr("Accent"), tr("Tenuto"), tr("Marcato"), tr("Ghost"), tr("Soft accent"), tr("Stress"), tr("Unstress") };
        result["noteCategory"] = categories[std::clamp(DynamicsPlayback::role(ns.front()), 0, 9)];
        const int base = DynamicsPlayback::velocity(ns.front(), 80);
        result["effectiveVelocity"] = !ns.front()->getProperty(Pid::PLAY).toBool() ? 0 : ns.front()->userVelocity() ? ns.front()->customizeVelocity(base) : base;
    }
    return result;
}
QVariantList DynamicsPanelModel::dynamicChoices() const {
    QVariantList result { QVariantMap { {"text", tr("Auto / notation")}, {"value", -1} } };
    for (const auto& def : Dynamic::definitions()) {
        if (def.type == DynamicType::OTHER) continue;
        result.append(QVariantMap { {"text", QString::fromStdString(TConv::toXml(def.type).ascii())}, {"value", static_cast<int>(def.type)} });
    }
    return result;
}
QVariantList DynamicsPanelModel::mappings() const {
    QVariantList result; auto s = score(); if (!s) return result;
    for (const auto& def : Dynamic::definitions()) {
        if (def.type == DynamicType::OTHER) continue;
        result.append(QVariantMap { {"name", QString::fromStdString(TConv::toXml(def.type).ascii())}, {"dynamic", static_cast<int>(def.type)},
            {"normal", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Normal)}, {"tap", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Tap)}, {"accent", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Accent)},
            {"tenuto", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Tenuto)}, {"marcato", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Marcato)}, {"ghost", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Ghost)}, {"softAccent", DynamicsPlayback::level(s, def.type, DynamicsPlayback::SoftAccent)}, {"stress", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Stress)}, {"unstress", DynamicsPlayback::level(s, def.type, DynamicsPlayback::Unstress)} });
    }
    return result;
}
void DynamicsPanelModel::edit(const std::function<void()>& change, const char* label) {
    if (!m_notation) return;
    m_editing = true;
    auto undo = m_notation->undoStack(); undo->prepareChanges(TranslatableString("undoableAction", label));
    change(); undo->commitChanges();
    m_editing = false; emit stateChanged();
}
void DynamicsPanelModel::showScore() { m_forceScore = true; emit stateChanged(); }
void DynamicsPanelModel::followSelection() { m_forceScore = false; emit stateChanged(); }
void DynamicsPanelModel::setEnabled(bool enabled) { edit([&]() { setScoreStyle(Sid::evanDynamicsEnabled, enabled); }, "Change score dynamics"); }
void DynamicsPanelModel::setBattery(bool enabled) { edit([&]() { setScoreStyle(Sid::evanDynamicsBattery, enabled); }, "Change accent and tap dynamics"); }
void DynamicsPanelModel::setMapping(int dynamic, int role, int velocity) {
    if (dynamic < 1 || dynamic >= static_cast<int>(Dynamic::definitions().size()) || role < 1 || role > DynamicsPlayback::Unstress) return;
    edit([&]() {
        std::string values;
        for (size_t i = 0; i < Dynamic::definitions().size(); ++i) values += std::to_string(i == static_cast<size_t>(dynamic) ? std::clamp(velocity, 0, 127) : DynamicsPlayback::level(score(), static_cast<DynamicType>(i), role)) + " ";
        const Sid sid = DynamicsPlayback::mappingStyle(role);
        setScoreStyle(sid, String::fromStdString(values));
        setScoreStyle(Sid::evanDynamicsEnabled, true);
    }, "Change dynamic mapping");
}
void DynamicsPanelModel::resetMappings() { edit([&]() {
    for (int role = DynamicsPlayback::Normal; role <= DynamicsPlayback::Unstress; ++role) setScoreStyle(DynamicsPlayback::mappingStyle(role), String());
}, "Reset dynamic mappings"); }
void DynamicsPanelModel::setCurve(int shape, double bend) {
    shape = std::clamp(shape, 0, 2); bend = std::clamp(bend, -2.0, 2.0);
    if (!score() || state().value("scope").toString() == "none" || dynamic() || !notes().empty()) return;
    if (DynamicsPlayback::enabled(score())) {
        if (auto h = hairpin()) {
            if (h->dynamicsCurveShape() == shape && h->dynamicsCurveBend() == bend) return;
        } else if (DynamicsPlayback::profile(score()).styleI(Sid::evanDynamicsCurveShape) == shape && DynamicsPlayback::profile(score()).styleD(Sid::evanDynamicsCurveBend) == bend) return;
    }
    edit([&]() {
        if (auto h = hairpin()) { h->undoChangeProperty(Pid::DYNAMICS_CURVE_SHAPE, shape); h->undoChangeProperty(Pid::DYNAMICS_CURVE_BEND, bend); }
        else if (notes().empty()) { setScoreStyle(Sid::evanDynamicsCurveShape, shape); setScoreStyle(Sid::evanDynamicsCurveBend, bend); }
        setScoreStyle(Sid::evanDynamicsEnabled, true);
    }, "Shape dynamics curve");
}
void DynamicsPanelModel::setEndpoint(bool end, int dynamic, int role, int velocity) {
    auto h = hairpin(); if (!h) return;
    if (dynamic < -1 || dynamic >= static_cast<int>(Dynamic::definitions().size())) return;
    edit([&]() {
        h->undoChangeProperty(end ? Pid::DYNAMICS_END_DYNAMIC : Pid::DYNAMICS_START_DYNAMIC, dynamic);
        h->undoChangeProperty(end ? Pid::DYNAMICS_END_ROLE : Pid::DYNAMICS_START_ROLE, std::clamp(role, 0, 9));
        h->undoChangeProperty(end ? Pid::DYNAMICS_END_VELOCITY : Pid::DYNAMICS_START_VELOCITY, velocity < 0 ? -1 : std::clamp(velocity, 0, 127));
        setScoreStyle(Sid::evanDynamicsEnabled, true);
    }, "Change hairpin endpoint");
}
void DynamicsPanelModel::setNoteVelocity(int velocity) {
    if (auto dyn = dynamic()) { edit([&]() { dyn->undoChangeProperty(Pid::DYNAMICS_MARK_VELOCITY, std::clamp(velocity, 0, 127)); setScoreStyle(Sid::evanDynamicsEnabled, true); }, "Change selected dynamic level"); return; }
    auto ns = notes(); if (ns.empty()) return;
    edit([&]() { for (auto n : ns) { n->undoChangeProperty(Pid::VELO_TYPE, VeloType::USER_VAL); n->undoChangeProperty(Pid::USER_VELOCITY, std::clamp(velocity, 1, 127)); n->undoChangeProperty(Pid::PLAY, velocity > 0); } }, "Change selected note dynamics");
}
void DynamicsPanelModel::setNotePlayback(bool play) { if (auto dyn = dynamic()) { edit([&]() { dyn->undoChangeProperty(Pid::PLAY, play); }, "Change dynamic playback"); return; } auto ns = notes(); if (ns.empty()) return; edit([&]() { for (auto n : ns) n->undoChangeProperty(Pid::PLAY, play); }, "Change selected note playback"); }
void DynamicsPanelModel::adjustNotes(int operation, double amount, int category) {
    auto ns = notes();
    if (ns.empty() || operation < 0 || operation > 2 || !std::isfinite(amount)) return;
    ns.erase(std::remove_if(ns.begin(), ns.end(), [category](const Note* n) { return category && DynamicsPlayback::role(n) != category; }), ns.end());
    if (ns.empty()) return;
    edit([&]() {
        for (Note* n : ns) {
            int value = DynamicsPlayback::velocity(n, 80);
            if (n->userVelocity() != 0) value = n->customizeVelocity(value);
            if (!n->getProperty(Pid::PLAY).toBool()) value = 0;
            if (operation == 2) {
                const int offset = static_cast<int>(std::lround(std::clamp(amount, -100.0, 500.0)));
                n->undoChangeProperty(Pid::VELO_TYPE, VeloType::OFFSET_VAL);
                n->undoChangeProperty(Pid::USER_VELOCITY, offset);
                n->undoChangeProperty(Pid::PLAY, offset > -100);
            } else {
                const double adjusted = operation == 0 ? value + amount : value * std::max(0.0, amount) / 100.0;
                value = static_cast<int>(std::lround(std::clamp(adjusted, 0.0, 127.0)));
                n->undoChangeProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
                n->undoChangeProperty(Pid::USER_VELOCITY, std::max(1, value));
                n->undoChangeProperty(Pid::PLAY, value > 0);
            }
        }
    }, "Adjust selected dynamics");
}
void DynamicsPanelModel::resetSelection() {
    if (auto dyn = dynamic()) { edit([&]() { dyn->undoChangeProperty(Pid::DYNAMICS_MARK_VELOCITY, -1); }, "Reset dynamic override"); return; }
    if (auto h = hairpin()) { edit([&]() {
        for (Pid id : {Pid::DYNAMICS_CURVE_SHAPE, Pid::DYNAMICS_CURVE_BEND, Pid::DYNAMICS_START_DYNAMIC, Pid::DYNAMICS_END_DYNAMIC, Pid::DYNAMICS_START_ROLE, Pid::DYNAMICS_END_ROLE, Pid::DYNAMICS_START_VELOCITY, Pid::DYNAMICS_END_VELOCITY}) h->undoChangeProperty(id, h->propertyDefault(id));
    }, "Use score hairpin defaults"); }
    else { auto ns = notes(); if (!ns.empty()) edit([&]() { for (auto n : ns) { n->undoChangeProperty(Pid::VELO_TYPE, VeloType::OFFSET_VAL); n->undoChangeProperty(Pid::USER_VELOCITY, 0); n->undoChangeProperty(Pid::PLAY, true); } }, "Use score note dynamics"); }
}

// Presets contain score-wide playback settings only, keeping engraving and local overrides intact.
bool DynamicsPanelModel::writePreset(const muse::io::path_t& path)
{
    if (!score()) return false;
    const auto data = DynamicsPlayback::preset(score()).toQByteArray();
    QSaveFile file(path.toQString());
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        interactive()->error(tr("Could not save dynamics preset").toStdString(), file.errorString().toStdString());
        return false;
    }
    return true;
}

void DynamicsPanelModel::readPreset(const muse::io::path_t& path)
{
    if (!score()) return;
    QFile file(path.toQString());
    MStyle candidate = DynamicsPlayback::profile(score());
    if (!file.open(QIODevice::ReadOnly)) {
        interactive()->error(tr("Could not load dynamics preset").toStdString(), file.errorString().toStdString());
        return;
    }
    if (file.size() > 1024 * 1024 || !DynamicsPlayback::readPreset(muse::ByteArray::fromQByteArray(file.readAll()), candidate)) {
        interactive()->error(tr("Could not load dynamics preset").toStdString(), tr("This is not a valid supported EvanScore dynamics preset. No settings were changed.").toStdString());
        return;
    }
    edit([&]() {
        for (Sid sid : DynamicsPlayback::profileStyles()) setScoreStyle(sid, candidate.value(sid));
    }, "Load score dynamics preset");
    m_notice = tr("Preset applied to this score. Local note and hairpin edits are preserved.");
    emit stateChanged();
}

void DynamicsPanelModel::savePreset()
{
    if (!score()) return;
    auto path = interactive()->selectSavingFileSync(tr("Save dynamics preset").toStdString(),
                globalConfiguration()->userDataPath() + "/EvanScore-Dynamics.evands",
                { "EvanScore dynamics preset (*.evands)" });
    if (path.empty()) return;
    if (!path.toQString().endsWith(".evands", Qt::CaseInsensitive)) path = path + ".evands";
    if (writePreset(path)) { m_notice = tr("Dynamics preset saved."); emit stateChanged(); }
}

void DynamicsPanelModel::loadPreset()
{
    if (!score()) return;
    const auto path = interactive()->selectOpeningFileSync(tr("Load dynamics preset").toStdString(),
                      globalConfiguration()->userDataPath(), { "EvanScore dynamics preset (*.evands)", "JSON files (*.json)" });
    if (!path.empty()) readPreset(path);
}

void DynamicsPanelModel::makeDefault()
{
    if (!score()) return;
    const auto directory = globalConfiguration()->userAppDataPath() + "/Dynamics";
    if (!QDir().mkpath(directory.toQString())) {
        interactive()->error(tr("Could not save default dynamics").toStdString(), tr("The dynamics settings folder could not be created.").toStdString());
        return;
    }
    const auto path = directory + "/EvanScore-Default.evands";
    if (!writePreset(path)) return;
    settings()->setSharedValue(SAVED_DEFAULT_DYNAMICS_PATH, muse::Val(path.toStdString()));
    m_notice = tr("Default saved for new scores, including after restarting EvanScore.");
    emit stateChanged();
}

void DynamicsPanelModel::applyDefault()
{
    const auto path = settings()->value(SAVED_DEFAULT_DYNAMICS_PATH).toPath();
    if (!path.empty()) readPreset(path);
}
