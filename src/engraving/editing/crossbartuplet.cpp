/* SPDX-License-Identifier: GPL-3.0-only */
#include "crossbartuplet.h"
#include <algorithm>
#include <memory>
#include "../dom/factory.h"
#include "../dom/measure.h"
#include "../dom/rest.h"
#include "../dom/score.h"
#include "../dom/segment.h"
#include "../dom/staff.h"
#include "../dom/tuplet.h"
#include "transaction/undostack.h"
#include "transaction/transaction.h"

using namespace mu::engraving;
namespace {
struct Plan {
    CrossBarTuplet::Preview preview;
    std::vector<Rest*> replaced;
};
Plan plan(const Rest* seed, const CrossBarTuplet::Options& options)
{
    Plan result;
    auto fail = [&result](const char16_t* error) {
        result.preview.error = error; result.preview.fragments.clear(); result.replaced.clear(); return result;
    };
    if (!seed || !seed->score() || !seed->measure() || !seed->staff() || seed->isGap()) {
        return fail(u"Choose an ordinary rest at the beginning of the new group.");
    }
    if (options.actual < 2 || options.actual > 64 || options.normal < 1 || options.normal > 64
        || !options.base.isValid() || options.base.isMeasure() || options.base.fraction() <= Fraction()) {
        return fail(u"Choose a valid base note and a ratio between 2–64 actual and 1–64 normal notes.");
    }
    if (seed->tuplet()) {
        return fail(u"Creation inside an existing tuplet is not supported by this prototype.");
    }
    // undoAddCR clones into linked voices. Do not allow a partial group while
    // the cross-bar linked-parent creation path remains unverified.
    if (seed->staff()->staffList().size() > 1) {
        return fail(u"Cross-bar creation on linked staves or parts is not yet supported.");
    }
    const Fraction ratio(options.actual, options.normal);
    result.preview.start = seed->tick();
    result.preview.end = seed->tick() + options.base.fraction() * options.normal;
    const auto end = result.preview.end;
    if (end <= seed->measure()->endTick()) {
        return fail(u"This group fits within one measure; use the ordinary tuplet command.");
    }
    if (!seed->score()->lastMeasure() || end > seed->score()->lastMeasure()->endTick()) {
        return fail(u"Add enough measures before creating this group.");
    }
    int measureCount = 0;
    for (auto* measure = seed->measure(); measure && measure->tick() < end; measure = measure->nextMeasure()) {
        if (++measureCount > 8 || measure->isMMRest()) {
            return fail(u"This prototype supports at most eight ordinary measures.");
        }
        if (seed->staff()->timeStretch(measure->tick()) != Fraction(1, 1)) {
            return fail(u"Local time stretching inside the group is not yet supported.");
        }
        for (auto* segment = measure->first(); segment; segment = segment->next()) {
            if (segment->tick() >= seed->tick() && segment->tick() < end
                && seed->staff()->timeStretch(segment->tick()) != Fraction(1, 1)) {
                return fail(u"Local time stretching inside the group is not yet supported.");
            }
        }
    }
    // Every affected tick must already be covered by ordinary rests. Existing
    // notes, lyrics, nested tuplets and invisible voice gaps are not overwritten.
    Fraction covered = seed->tick();
    for (auto* segment = seed->segment(); segment && segment->tick() < end; segment = segment->next1(SegmentType::ChordRest)) {
        auto* element = segment->element(seed->track());
        if (!element) { continue; }
        if (!element->isRest()) { return fail(u"The requested span contains notes; choose an empty rest span."); }
        auto* rest = toRest(element);
        if (rest->isGap() || rest->tuplet() || !rest->lyrics().empty() || rest->tick() != covered || rest->actualTicks() <= Fraction()) {
            return fail(u"The rest span has a gap, tuplet or attached lyrics; it cannot be replaced safely.");
        }
        result.replaced.push_back(rest);
        covered = rest->endTick();
    }
    if (covered < end || result.replaced.empty() || result.replaced.front() != seed) {
        return fail(u"The requested span is not completely covered by ordinary rests.");
    }
    Fraction cursor = seed->tick();
    for (int attack = 0; attack < options.actual; ++attack) {
        Fraction remaining = options.base.fraction() / ratio;
        while (remaining > Fraction()) {
            auto* measure = seed->score()->tick2measure(cursor);
            if (!measure) { return fail(u"A required measure is missing."); }
            const Fraction length = std::min(remaining, measure->endTick() - cursor);
            const Fraction written = length * ratio;
            const auto durations = toDurationList(written, true, 3, false);
            Fraction sum;
            for (const auto& duration : durations) { sum += duration.fraction(); }
            if (length <= Fraction() || durations.empty() || sum != written) {
                return fail(u"This ratio and barline offset need a nonrepresentable written duration. Choose a different start or base note.");
            }
            for (const auto& duration : durations) {
                result.preview.fragments.push_back({ cursor, duration, attack });
                cursor += duration.fraction() / ratio;
            }
            remaining -= length;
        }
    }
    if (cursor != end) { return fail(u"The requested group does not have exact rational timing."); }
    if (covered > end) {
        const auto tail = toDurationList(covered - end, true, 3, false);
        Fraction sum;
        for (const auto& duration : tail) {
            result.preview.fragments.push_back({ cursor, duration, -1 }); sum += duration.fraction(); cursor += duration.fraction();
        }
        if (tail.empty() || sum != covered - end) { return fail(u"The remaining ordinary rest cannot be preserved exactly."); }
    }
    return result;
}
}
CrossBarTuplet::Preview CrossBarTuplet::preview(const Rest* seed, const Options& options)
{
    return plan(seed, options).preview;
}
Tuplet* CrossBarTuplet::create(Transaction& transaction, Rest* seed, const Options& options, muse::String& error)
{
    auto prepared = plan(seed, options);
    error = prepared.preview.error;
    if (!prepared.preview.valid()) { return nullptr; }
    auto* score = seed->score();
    if (!score->undoStack()->hasActiveTransaction() || score->transactionManager()->currentTransaction() != &transaction) {
        error = u"Begin a native undo transaction before creating the group."; return nullptr;
    }
    const auto track = seed->track();
    auto group = std::unique_ptr<Tuplet>(Factory::createTuplet(seed->measure()));
    group->setTrack(track); group->setTick(prepared.preview.start);
    group->setRatio(Fraction(options.actual, options.normal)); group->setBaseLen(options.base);
    group->setTicks(prepared.preview.end - prepared.preview.start);
    std::vector<std::unique_ptr<Rest>> inserted;
    for (const auto& fragment : prepared.preview.fragments) {
        // Preserve the remainder's rest appearance outside the requested span.
        auto rest = std::unique_ptr<Rest>(fragment.attack < 0 ? Factory::copyRest(*prepared.replaced.back()) : Factory::createRest(score->dummy()));
        rest->setTrack(track); rest->setTicks(fragment.written.fraction()); rest->setDurationType(fragment.written);
        if (fragment.attack >= 0) { rest->setTuplet(group.get()); }
        inserted.push_back(std::move(rest));
    }
    for (auto* rest : prepared.replaced) { score->undoRemoveElement(rest); }
    for (size_t i = 0; i < inserted.size(); ++i) {
        const auto tick = prepared.preview.fragments[i].tick;
        score->undoAddCR(inserted[i].release(), score->tick2measure(tick), tick);
    }
    score->setLayoutAll();
    return group.release();
}
