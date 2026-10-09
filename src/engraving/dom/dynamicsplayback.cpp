/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "dynamicsplayback.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include "articulation.h"
#include "chord.h"
#include "dynamic.h"
#include "hairpin.h"
#include "note.h"
#include "score.h"
#include "masterscore.h"
#include "segment.h"
#include "staff.h"
#include "spanner.h"
#include "serialization/json.h"
#include "engraving/types/typesconv.h"
using namespace mu::engraving;

const MStyle& DynamicsPlayback::profile(const Score* score) { return score->masterScore() ? score->masterScore()->style() : score->style(); }
bool DynamicsPlayback::enabled(const Score* score) { return profile(score).styleB(Sid::evanDynamicsEnabled); }

// Exact endpoints, a continuous power curve, a normalized logistic curve, or a delayed ramp.
double DynamicsPlayback::progress(double t, int shape, double bend)
{
    t = std::clamp(t, 0.0, 1.0);
    bend = std::clamp(bend, -2.0, 2.0);
    if (t == 0.0 || t == 1.0) return t;
    if (shape == 1) {
        const double k = 4.0 * std::exp2(bend);
        auto sigmoid = [k](double x) { return 1.0 / (1.0 + std::exp(-k * (x - 0.5))); };
        return (sigmoid(t) - sigmoid(0.0)) / (sigmoid(1.0) - sigmoid(0.0));
    }
    if (shape == 2) {
        const double delay = std::clamp(0.4 + bend * 0.2, 0.0, 0.8);
        return std::clamp((t - delay) / (1.0 - delay), 0.0, 1.0);
    }
    return std::pow(t, std::exp2(bend * 2.0));
}

int DynamicsPlayback::level(const Score* score, DynamicType type, int role)
{
    int index = static_cast<int>(type);
    if (index <= 0 || index >= static_cast<int>(Dynamic::definitions().size())) index = static_cast<int>(DynamicType::MF);
    role = std::clamp(role, int(Normal), int(Unstress));
    const Sid sid = mappingStyle(role);
    struct MappingCache {
        String source;
        std::array<int, 64> values;
        bool initialized = false;
    };
    thread_local std::array<MappingCache, 9> caches;
    auto& cache = caches[role - 1];
    const String source = profile(score).styleSt(sid);
    if (!cache.initialized || cache.source != source) {
        cache.source = source;
        cache.initialized = true;
        cache.values.fill(-1);
        std::istringstream stream(source.toStdString());
        for (int& item : cache.values) {
            if (!(stream >> item)) { item = -1; break; }
        }
    }
    int value = index < static_cast<int>(cache.values.size()) ? cache.values[index] : -1;
    if (value < 0) {
        value = Dynamic::definitions()[index].velocity;
        if (role == Tap && index > static_cast<int>(DynamicType::MP)) value = Dynamic::definitions()[static_cast<int>(DynamicType::P)].velocity;
        if (role == Ghost) value = std::min(value, Dynamic::definitions()[static_cast<int>(DynamicType::PP)].velocity);
        if (role == Unstress) value = std::min(value, Dynamic::definitions()[static_cast<int>(DynamicType::P)].velocity);
        if (static_cast<DynamicType>(index) == DynamicType::N) value = 0;
    }
    return std::clamp(value, 0, 127);
}

bool DynamicsPlayback::accented(const Chord* chord)
{
    if (!chord) return false;
    return std::any_of(chord->articulations().begin(), chord->articulations().end(), [](const Articulation* a) { return a->playArticulation() && (a->isAccent() || a->isMarcato()); });
}

static const Dynamic* previousDynamic(const Segment* segment, track_idx_t track)
{
    for (const Segment* s = segment; s; s = s->prev1()) {
        for (const EngravingItem* item : s->annotations()) {
            if (!item->isDynamic()) continue;
            const auto dynamic = toDynamic(item);
            if (!dynamic->playDynamic()) continue;
            const auto assignment = dynamic->voiceAssignment();
            const Staff* staff = item->score()->staff(track2staff(track));
            const bool applies = assignment == VoiceAssignment::CURRENT_VOICE_ONLY ? item->track() == track
                : assignment == VoiceAssignment::ALL_VOICE_IN_STAFF ? item->staffIdx() == track2staff(track)
                : staff && item->part() == staff->part();
            const auto type = dynamic->dynamicType();
            const bool compound = type == DynamicType::FP || type == DynamicType::PF || type == DynamicType::SFP || type == DynamicType::SFPP;
            const bool transient = static_cast<size_t>(type) < Dynamic::definitions().size() && Dynamic::definitions()[static_cast<size_t>(type)].accent
                && !compound && type != DynamicType::N;
            if (applies && (!transient || dynamic->tick() == segment->tick())) return dynamic;
        }
    }
    return nullptr;
}

static int roleAt(const Score* score, const Segment* segment, track_idx_t track)
{
    const EngravingItem* item = segment ? segment->element(track) : nullptr;
    if (item && item->isChord()) {
        const auto& arts = toChord(item)->articulations();
        for (const Articulation* a : arts) if (a->playArticulation() && a->isMarcato()) return DynamicsPlayback::Marcato;
        for (const Articulation* a : arts) {
            if (!a->playArticulation()) continue;
            if (a->symId() == SymId::articStressAbove || a->symId() == SymId::articStressBelow) return DynamicsPlayback::Stress;
        }
        for (const Articulation* a : arts) if (a->playArticulation() && (a->isAccent() || a->symId() == SymId::articTenutoAccentAbove || a->symId() == SymId::articTenutoAccentBelow)) return DynamicsPlayback::Accent;
        for (const Articulation* a : arts) {
            if (!a->playArticulation()) continue;
            switch (a->symId()) {
            case SymId::articSoftAccentAbove: case SymId::articSoftAccentBelow:
            case SymId::articSoftAccentStaccatoAbove: case SymId::articSoftAccentStaccatoBelow:
            case SymId::articSoftAccentTenutoAbove: case SymId::articSoftAccentTenutoBelow:
            case SymId::articSoftAccentTenutoStaccatoAbove: case SymId::articSoftAccentTenutoStaccatoBelow:
                return DynamicsPlayback::SoftAccent;
            case SymId::articUnstressAbove: case SymId::articUnstressBelow: return DynamicsPlayback::Unstress;
            default: break;
            }
        }
        for (const Articulation* a : arts) if (a->playArticulation() && a->isTenuto()) return DynamicsPlayback::Tenuto;
    }
    const Staff* staff = score->staff(track2staff(track));
    return staff && DynamicsPlayback::profile(score).styleB(Sid::evanDynamicsBattery)
        && staff->isDrumStaff(segment ? segment->tick() : Fraction()) ? DynamicsPlayback::Tap : DynamicsPlayback::Normal;
}

int DynamicsPlayback::role(const Note* note)
{
    if (note->ghost()) return Ghost;
    return roleAt(note->score(), note->chord() ? note->chord()->segment() : nullptr, note->track());
}

bool DynamicsPlayback::applies(const Hairpin* h, track_idx_t track)
{
    const Staff* staff = h->score()->staff(track2staff(track));
    switch (h->voiceAssignment()) {
    case VoiceAssignment::CURRENT_VOICE_ONLY: return h->track() == track;
    case VoiceAssignment::ALL_VOICE_IN_STAFF: return h->staffIdx() == track2staff(track);
    default: return staff && h->part() == staff->part();
    }
}

const Hairpin* DynamicsPlayback::activeHairpin(const Score* score, const Fraction& tick, track_idx_t track)
{
    const auto& overlapping = score->spannerMap().findOverlapping(tick.ticks(), tick.ticks(), false);
    const Hairpin* active = nullptr;
    for (const auto& interval : overlapping) {
        const Spanner* sp = interval.value;
        if (!sp->isHairpin() || !sp->playSpanner() || sp->tick() > tick || sp->tick2() < tick) continue;
        const Hairpin* h = toHairpin(sp);
        if (applies(h, track) && (!active || h->tick() > active->tick())) active = h;
    }
    return active;
}

bool DynamicsPlayback::smoothArticulations(const Hairpin* h)
{
    return h->dynamicsSmoothArticulations() < 0 ? profile(h->score()).styleB(Sid::evanDynamicsSmoothArticulations)
        : h->dynamicsSmoothArticulations() != 0;
}

double DynamicsPlayback::hairpinValue(const Hairpin* h, double position, int noteRole)
{
    int shape = h->dynamicsCurveShape();
    double bend = h->dynamicsCurveBend();
    if (shape < 0) shape = profile(h->score()).styleI(Sid::evanDynamicsCurveShape);
    if (bend < -2.0) bend = profile(h->score()).styleD(Sid::evanDynamicsCurveBend);
    // Optional shared envelope: printed articulations do not switch velocity
    // lanes mid-ramp. Explicit note overrides are still applied by callers.
    const int lane = smoothArticulations(h) ? Auto : noteRole;
    const int from = endpoint(h, false, lane), to = endpoint(h, true, lane);
    return std::clamp(from + (to - from) * progress(position, shape, bend), 0.0, 127.0);
}

int DynamicsPlayback::endpoint(const Hairpin* h, bool end, int lane)
{
    const int overrideVelocity = end ? h->dynamicsEndVelocity() : h->dynamicsStartVelocity();
    const int configuredRole = end ? h->dynamicsEndRole() : h->dynamicsStartRole();
    if (overrideVelocity >= 0 && (lane == Auto || configuredRole == Auto || lane == configuredRole || configuredRole == Tap)) return std::clamp(overrideVelocity, 0, 127);
    const Segment* segment = end ? h->endSegment() : h->startSegment();
    const Dynamic* snapped = h->spannerSegments().empty() ? nullptr : end ? h->dynamicSnappedAfter() : h->dynamicSnappedBefore();
    const Dynamic* dynamic = snapped;
    if (!dynamic) dynamic = previousDynamic(segment, h->track());
    const bool inferredEnd = end && h->dynamicsEndDynamic() < 0 && h->dynamicTypeTo() == DynamicType::OTHER && !snapped
        && (!dynamic || dynamic->tick() <= h->tick());
    int type = end ? h->dynamicsEndDynamic() : h->dynamicsStartDynamic();
    const bool typeFromNotation = type < 0;
    int role = lane == Auto ? configuredRole : lane;
    // A mixed accent-to-tap destination resolves to the tap mapping for every arrival.
    if (lane != Auto && lane != Tap && configuredRole == Tap) role = Tap;
    if (!role) role = roleAt(h->score(), segment, h->track());
    if (type < 0) {
        const auto textType = end ? h->dynamicTypeTo() : h->dynamicTypeFrom();
        if (textType != DynamicType::OTHER) type = static_cast<int>(textType);
        else if (dynamic) type = static_cast<int>(dynamic->dynamicType());
        else type = static_cast<int>(end ? (h->isCrescendo() ? DynamicType::F : DynamicType::P) : DynamicType::MF);
    }
    int value = level(h->score(), static_cast<DynamicType>(type), role);
    if (typeFromNotation && dynamic) {
        if (dynamic->preciseVelocity() >= 0) value = dynamic->preciseVelocity();
        else if (role != Tap && dynamic->velocityOverride() > 0 && dynamic->velocity() != Dynamic::definitions()[static_cast<int>(dynamic->dynamicType())].velocity) value = dynamic->velocity();
    }
    int startRole = h->dynamicsStartRole();
    if (startRole == Auto) startRole = roleAt(h->score(), h->startSegment(), h->track());
    // A shared ramp ending in a different stroke category already has an
    // inferred destination (e.g. accent -> tap). Do not shift that target by
    // the default unmarked-hairpin +/-16 change.
    if (inferredEnd && (h->veloChange() != 0 || !smoothArticulations(h) || role == startRole))
        value += (h->isCrescendo() ? 1 : -1) * (h->veloChange() ? std::abs(h->veloChange()) : 16);
    return std::clamp(value, 0, 127);
}

int DynamicsPlayback::velocityAt(const Note* note, const Fraction& position, int fallback)
{
    if (!enabled(note->score())) return fallback;
    // A silent battery ghost stays silent through shared articulation curves.
    if (note->ghost() && velocity(note, fallback) == 0) return 0;
    const Hairpin* h = activeHairpin(note->score(), position, note->track());
    if (!h) h = activeHairpin(note->score(), note->tick(), note->track());
    if (h && h->ticks().ticks() > 0) {
        const double t = double((position - h->tick()).ticks()) / h->ticks().ticks();
        return static_cast<int>(std::lround(hairpinValue(h, t, role(note))));
    }
    return velocity(note, fallback);
}

int DynamicsPlayback::velocity(const Note* note, int fallback)
{
    const Score* score = note->score();
    if (!profile(score).styleB(Sid::evanDynamicsEnabled)) return fallback;
    const Segment* segment = note->chord() ? note->chord()->segment() : nullptr;
    const Dynamic* dynamic = previousDynamic(segment, note->track());
    if (note->ghost() && level(score, dynamic ? dynamic->dynamicType() : DynamicType::MF, Ghost) == 0) return 0;
    const Hairpin* active = activeHairpin(score, note->tick(), note->track());
    if (active && active->ticks().ticks() > 0) {
        const double t = double((note->tick() - active->tick()).ticks()) / active->ticks().ticks();
        return static_cast<int>(std::lround(hairpinValue(active, t, role(note))));
    }
    auto type = dynamic ? dynamic->dynamicType() : DynamicType::MF;
    bool settledCompound = false;
    if (dynamic && dynamic->tick() < note->tick()) {
        switch (type) {
        case DynamicType::FP: case DynamicType::SFP: type = DynamicType::P; settledCompound = true; break;
        case DynamicType::PF: type = DynamicType::F; settledCompound = true; break;
        case DynamicType::SFPP: type = DynamicType::PP; settledCompound = true; break;
        default: break;
        }
    }
    int result = level(score, type, role(note));
    // A local velocity on a dynamic marking remains an explicit override.
    if (dynamic && !settledCompound) {
        const int role = DynamicsPlayback::role(note);
        if (dynamic->preciseVelocity() >= 0) result = dynamic->preciseVelocity();
        // Legacy markings often contain imported playback velocities (e.g.
        // p=64). They must not supersede a custom battery tap mapping.
        else if (role != Tap && dynamic->velocityOverride() > 0 && dynamic->velocity() != Dynamic::definitions()[static_cast<int>(dynamic->dynamicType())].velocity) result = dynamic->velocity();
    }
    // A completed playback-only ramp holds its destination until the next written dynamic.
    const Hairpin* completed = nullptr;
    for (const auto& entry : score->spannerMap().map()) {
        const Spanner* sp = entry.second;
        if (!sp->isHairpin() || !sp->playSpanner() || sp->tick2() >= note->tick()) continue;
        const Hairpin* h = toHairpin(sp);
        if (applies(h, note->track()) && (!dynamic || dynamic->tick() < h->tick2())
            && (!completed || h->tick2() > completed->tick2())) completed = h;
    }
    if (completed) result = static_cast<int>(std::lround(hairpinValue(completed, 1.0, role(note))));
    return std::clamp(result, 0, 127);
}

Sid DynamicsPlayback::mappingStyle(int role)
{
    switch (role) {
    case Tap: return Sid::evanDynamicsTap;
    case Accent: return Sid::evanDynamicsAccent;
    case Tenuto: return Sid::evanDynamicsTenuto;
    case Marcato: return Sid::evanDynamicsMarcato;
    case Ghost: return Sid::evanDynamicsGhost;
    case SoftAccent: return Sid::evanDynamicsSoftAccent;
    case Stress: return Sid::evanDynamicsStress;
    case Unstress: return Sid::evanDynamicsUnstress;
    default: return Sid::evanDynamicsNormal;
    }
}

std::array<Sid, 14> DynamicsPlayback::profileStyles()
{
    return { Sid::evanDynamicsEnabled, Sid::evanDynamicsBattery, Sid::evanDynamicsCurveShape,
             Sid::evanDynamicsCurveBend, Sid::evanDynamicsNormal, Sid::evanDynamicsTap,
             Sid::evanDynamicsAccent, Sid::evanDynamicsTenuto, Sid::evanDynamicsMarcato, Sid::evanDynamicsGhost, Sid::evanDynamicsSoftAccent, Sid::evanDynamicsStress, Sid::evanDynamicsUnstress, Sid::evanDynamicsSmoothArticulations };
}

static constexpr std::array<const char*, 9> ROLE_NAMES { "normal", "tap", "accent", "tenuto", "marcato", "ghost", "softAccent", "stress", "unstress" };

MStyle DynamicsPlayback::marchingSnareDefaults(const MStyle& base)
{
    MStyle result = base;
    // Starting calibration, not a physical inches-to-MIDI conversion. Raise
    // quiet strokes above the original scale; sample libraries still need auditioning.
    const auto ordinary = [](DynamicType type, int fallback) {
        switch (type) {
        case DynamicType::PP: return 45;
        case DynamicType::P: return 60;
        case DynamicType::MP: return 72;
        case DynamicType::MF: return 84;
        case DynamicType::F: return 100;
        case DynamicType::FF: return 114;
        case DynamicType::FFF: return 126;
        default: return std::max(0, fallback);
        }
    };
    const int tap = ordinary(DynamicType::P, 0);
    const int mp = ordinary(DynamicType::MP, 0);
    for (int role = Normal; role <= Unstress; ++role) {
        std::string values;
        for (const auto& def : Dynamic::definitions()) {
            int value = ordinary(def.type, def.velocity);
            if (role == Tap || role == Unstress) value = std::min(value, tap);
            if (role == Ghost) value = 0; // Battery ghost notation means no stroke.
            if (role == Accent || role == Marcato || role == Stress || role == Tenuto || role == SoftAccent) {
                if (def.type == DynamicType::PP) value = tap; // 1-inch base, 3-inch accent/tenuto.
                if (def.type == DynamicType::P) value = tap + (mp - tap) / 3; // 4-inch accent/tenuto.
                if (role == Tenuto || role == SoftAccent) {
                    // Above mp, intermediate strokes stay one height below the accent.
                    switch (def.type) {
                    case DynamicType::MF: value = mp; break;
                    case DynamicType::F: value = ordinary(DynamicType::MF, 0); break;
                    case DynamicType::FF: value = ordinary(DynamicType::F, 0); break;
                    case DynamicType::FFF: value = ordinary(DynamicType::FF, 0); break;
                    case DynamicType::FFFF: value = ordinary(DynamicType::FFF, 0); break;
                    default: break;
                    }
                }
            }
            if (def.type == DynamicType::OTHER || def.type == DynamicType::N) value = 0;
            values += std::to_string(std::clamp(value, 0, 127)) + " ";
        }
        result.set(mappingStyle(role), String::fromStdString(values));
    }
    result.set(Sid::evanDynamicsEnabled, true);
    result.set(Sid::evanDynamicsBattery, true);
    result.set(Sid::evanDynamicsCurveShape, 0);
    result.set(Sid::evanDynamicsCurveBend, 0.0);
    result.set(Sid::evanDynamicsSmoothArticulations, false);
    return result;
}

muse::ByteArray DynamicsPlayback::preset(const Score* score)
{
    muse::JsonObject root;
    root["format"] = "EvanScoreDynamics";
    root["version"] = 1;
    root["enabled"] = profile(score).styleB(Sid::evanDynamicsEnabled);
    root["battery"] = profile(score).styleB(Sid::evanDynamicsBattery);
    root["smoothArticulations"] = profile(score).styleB(Sid::evanDynamicsSmoothArticulations);
    root["curveShape"] = profile(score).styleI(Sid::evanDynamicsCurveShape);
    root["curveBend"] = profile(score).styleD(Sid::evanDynamicsCurveBend);
    muse::JsonObject mappings;
    for (int role = Normal; role <= Unstress; ++role) {
        muse::JsonObject levels;
        for (const auto& def : Dynamic::definitions()) {
            if (def.type != DynamicType::OTHER) levels[std::string(TConv::toXml(def.type).ascii())] = level(score, def.type, role);
        }
        mappings[ROLE_NAMES[role - 1]] = levels;
    }
    root["mappings"] = mappings;
    return muse::JsonDocument(root).toJson();
}

bool DynamicsPlayback::readPreset(const muse::ByteArray& data, MStyle& target)
{
    std::string error;
    const auto document = muse::JsonDocument::fromJson(data, &error);
    if (!error.empty() || !document.isObject()) return false;
    const auto root = document.rootObject();
    const auto version = root.value("version");
    const auto shape = root.value("curveShape");
    const auto bend = root.value("curveBend");
    if (root.value("format").toStdString() != "EvanScoreDynamics" || !version.isNumber() || version.toDouble() != 1.0
        || !root.value("enabled").isBool() || !root.value("battery").isBool()
        || !shape.isNumber() || shape.toDouble() < 0 || shape.toDouble() > 2 || shape.toDouble() != shape.toInt()
        || !bend.isNumber() || !std::isfinite(bend.toDouble()) || bend.toDouble() < -2.0 || bend.toDouble() > 2.0
        || !root.value("mappings").isObject()
        || (root.contains("smoothArticulations") && !root.value("smoothArticulations").isBool())) return false;
    MStyle candidate = target;
    const auto mappings = root.value("mappings").toObject();
    for (int role = Normal; role <= Unstress; ++role) {
        const auto lane = mappings.value(ROLE_NAMES[role - 1]);
        if (!lane.isObject()) return false;
        const auto levels = lane.toObject();
        if (levels.size() != Dynamic::definitions().size() - 1) return false;
        std::string values = "0 ";
        for (const auto& def : Dynamic::definitions()) {
            if (def.type == DynamicType::OTHER) continue;
            const auto value = levels.value(std::string(TConv::toXml(def.type).ascii()));
            if (!value.isNumber() || !std::isfinite(value.toDouble()) || value.toDouble() < 0 || value.toDouble() > 127
                || value.toDouble() != value.toInt()) return false;
            values += std::to_string(value.toInt()) + " ";
        }
        candidate.set(mappingStyle(role), muse::String::fromStdString(values));
    }
    candidate.set(Sid::evanDynamicsEnabled, root.value("enabled").toBool());
    candidate.set(Sid::evanDynamicsBattery, root.value("battery").toBool());
    candidate.set(Sid::evanDynamicsSmoothArticulations, root.value("smoothArticulations").toBool());
    candidate.set(Sid::evanDynamicsCurveShape, shape.toInt());
    candidate.set(Sid::evanDynamicsCurveBend, bend.toDouble());
    target = candidate;
    return true;
}
