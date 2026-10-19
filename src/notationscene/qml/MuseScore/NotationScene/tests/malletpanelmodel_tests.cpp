/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gmock/gmock.h>
#include <algorithm>
#include <QMetaProperty>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include "../malletpanelmodel.h"
#include "context/tests/mocks/globalcontextmock.h"
#include "notation/tests/mocks/notationmock.h"
#include "notation/tests/mocks/notationinteractionmock.h"
#include "notation/tests/mocks/notationselectionmock.h"
#include "notation/internal/notationundostack.h"
#include "playback/tests/mocks/playbackcontrollermock.h"
#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/instrument.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/noteval.h"
#include "engraving/dom/part.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/sticking.h"

using namespace mu::engraving;
using namespace mu::notation;
using namespace testing;

class MalletPanelWorkflow : public Test {
protected:
    muse::modularity::ContextPtr ctx = std::make_shared<muse::modularity::Context>(531);
    std::shared_ptr<NiceMock<mu::context::GlobalContextMock>> context = std::make_shared<NiceMock<mu::context::GlobalContextMock>>();
    std::shared_ptr<NiceMock<NotationMock>> notation = std::make_shared<NiceMock<NotationMock>>();
    std::shared_ptr<NiceMock<NotationInteractionMock>> interaction = std::make_shared<NiceMock<NotationInteractionMock>>();
    std::shared_ptr<NiceMock<NotationSelectionMock>> selection = std::make_shared<NiceMock<NotationSelectionMock>>();
    std::shared_ptr<NiceMock<mu::playback::PlaybackControllerMock>> playback = std::make_shared<NiceMock<mu::playback::PlaybackControllerMock>>();
    std::unique_ptr<MasterScore> score;
    std::unique_ptr<MalletPanelModel> model;
    std::vector<Note*> selected;
    std::vector<EngravingItem*> elements;
    muse::async::Notification selectionChanged;
    muse::async::Channel<muse::RectF> notationChanged;
    struct ScoreGetter : IGetScore {
        MasterScore* master=nullptr;
        Score* score() const override {return master;}
        muse::async::Notification scoreInited() const override {return {};}
    } scoreGetter;
    std::shared_ptr<NotationUndoStack> undo;
    QVariantMap oldSettings;
    Chord* first = nullptr;
    Chord* second = nullptr;
    void useWideChord() {
        const std::vector<int> pitches{48,52,55,71};
        for(size_t i=0;i<pitches.size();++i) {first->notes()[i]->setPitch(pitches[i]);first->notes()[i]->setTpcFromPitch();}
        model->refresh();
    }

    Chord* addChord(Measure* measure, Fraction tick, std::vector<int> pitches) {
        auto* segment = measure->getSegment(SegmentType::ChordRest, tick);
        auto* chord = Factory::createChord(segment);
        chord->setTrack(0); chord->setTicks(Fraction(1, 4)); chord->setDurationType(DurationType::V_QUARTER);
        segment->add(chord);
        for (int pitch : pitches) {
            auto* note = Factory::createNote(chord); note->setTrack(0); note->setPitch(pitch); note->setTpcFromPitch(); chord->add(note);
        }
        return chord;
    }
    void SetUp() override {
        QSettings prefs; prefs.beginGroup("evanscore/mallet");
        for (const auto& key : prefs.allKeys()) oldSettings.insert(key, prefs.value(key));
        prefs.remove("");
        score.reset(compat::ScoreAccess::createMasterScore(nullptr));
        score->lockUpdates(true); scoreGetter.master=score.get();
        undo=std::make_shared<NotationUndoStack>(&scoreGetter,notationChanged);
        auto* part = new Part(score.get()); score->appendPart(part);
        score->appendStaff(Factory::createStaff(part));
        part->instrument()->setId(u"marimba");
        auto* measure = Factory::createMeasure(score.get()); measure->setTick(Fraction()); measure->setTicks(Fraction(1, 1)); score->measures()->add(measure);
        first = addChord(measure, Fraction(), {60, 64, 67, 72});
        second = addChord(measure, Fraction(1, 4), {62, 65, 69, 74});
        selected = {first->notes().front()};
        ON_CALL(*context, currentNotation()).WillByDefault(Return(notation));
        ON_CALL(*playback,isPlaybackInited()).WillByDefault(Return(true));
        ON_CALL(*notation, interaction()).WillByDefault(Return(interaction));
        ON_CALL(*notation, notationChanged()).WillByDefault(Return(notationChanged));
        ON_CALL(*notation, undoStack()).WillByDefault(Return(undo));
        ON_CALL(*interaction, selection()).WillByDefault(Return(selection));
        ON_CALL(*interaction, selectionChanged()).WillByDefault(Return(selectionChanged));
        ON_CALL(*selection, notes(_)).WillByDefault([this](auto) { return selected; });
        ON_CALL(*selection, elements()).WillByDefault(ReturnRef(elements));
        ON_CALL(*selection, element()).WillByDefault([this]()->EngravingItem* { return selected.empty() ? nullptr : selected.front(); });
        muse::modularity::ioc(ctx)->registerExport<mu::context::IGlobalContext>("test", context);
        muse::modularity::ioc(ctx)->registerExport<mu::playback::IPlaybackController>("test", playback);
        model = std::make_unique<MalletPanelModel>(); model->setContext(ctx); model->setActive(true);
    }
    void TearDown() override {
        model.reset(); muse::modularity::removeIoC(ctx);
        QSettings prefs; prefs.beginGroup("evanscore/mallet"); prefs.remove("");
        for (auto it=oldSettings.begin(); it!=oldSettings.end(); ++it) prefs.setValue(it.key(), it.value());
    }
    QVariantMap state() const { return model->state(); }
    QVariantList pitches() const { return state().value("pose").toMap().value("pitches").toList(); }
};

TEST_F(MalletPanelWorkflow, OneSelectedNoteReadsTheWholeChordWithoutChangingIt) {
    EXPECT_EQ(pitches(), (QVariantList{60,64,67,72}));
    EXPECT_TRUE(state().value("supported").toBool());
    EXPECT_EQ(first->notes().size(), 4);
    EXPECT_TRUE(first->segment()->annotations().empty());
}

TEST_F(MalletPanelWorkflow, ComfortableOriginalDoesNotNeedAPermutationSuggestion) {
    EXPECT_EQ(state().value("pose").toMap().value("mallets").toList(),(QVariantList{1,2,3,4}));
    EXPECT_TRUE(model->alternativeRows().empty());EXPECT_TRUE(state().value("recommendKeep").toBool());
}
TEST_F(MalletPanelWorkflow, GenuineRevoicingKeepsPhysicalLinksAndAppliesInOneUndo) {
    useWideChord();ASSERT_FALSE(model->alternativeRows().empty());model->selectAlternative(0);
    EXPECT_EQ(pitches(),(QVariantList{48,52,67,71}));
    EXPECT_EQ(state().value("pose").toMap().value("mallets").toList(),(QVariantList{1,2,3,4}));
    model->commit();EXPECT_EQ(first->notes()[2]->pitch(),67);
    undo->undo(nullptr);model->refresh();EXPECT_EQ(pitches(),(QVariantList{48,52,55,71}));
}
TEST_F(MalletPanelWorkflow, ViewerUsesWrittenEnharmonicSpellingAndDetectsSpellingEdits) {
    first->notes().back()->setPitch(70);first->notes().back()->setTpc1(12);first->notes().back()->setTpc2(12);model->refresh();
    const auto before=state().value("sourceKey");
    auto voices=state().value("pose").toMap().value("voices").toList();EXPECT_EQ(voices.back().toMap().value("name").toString(),QString::fromUtf8("B♭4"));
    first->notes().back()->setTpc1(24);first->notes().back()->setTpc2(24);model->refresh();
    voices=state().value("pose").toMap().value("voices").toList();EXPECT_EQ(voices.back().toMap().value("name").toString(),QString::fromUtf8("A♯4"));
    EXPECT_NE(state().value("sourceKey"),before);
}
TEST_F(MalletPanelWorkflow, LocalAiIsOptionalAndClosingThePanelDisablesIt) {
    EXPECT_FALSE(model->assistantState().value("enabled").toBool());model->setAssistantEnabled(true);
    EXPECT_FALSE(model->assistantState().value("busy").toBool());model->askAssistant();
    EXPECT_TRUE(first->segment()->annotations().empty());model->setActive(false);
    EXPECT_FALSE(model->assistantState().value("enabled").toBool());
}

TEST_F(MalletPanelWorkflow, SuggestedOriginalStickingCanBeAppliedWithoutChangingTheChord) {
    EXPECT_TRUE(state().value("canCommit").toBool());
    EXPECT_EQ(state().value("selectedAlternative").toInt(), -1);
    model->commit();
    ASSERT_EQ(first->segment()->annotations().size(), 1);
    EXPECT_EQ(toSticking(first->segment()->annotations().front())->plainText().toQString(), "1 2 3 4");
    EXPECT_EQ(pitches(), (QVariantList{60,64,67,72}));
    EXPECT_FALSE(state().value("canCommit").toBool());
    undo->undo(nullptr); model->refresh();
    EXPECT_TRUE(first->segment()->annotations().empty()); EXPECT_EQ(pitches(), (QVariantList{60,64,67,72}));
    undo->redo(nullptr); model->refresh();
    EXPECT_EQ(first->segment()->annotations().size(), 1);
}

TEST_F(MalletPanelWorkflow, ChoosingSuggestionsDoesNotRebuildTheSuggestionList) {
    useWideChord();
    const auto before = model->alternativeRows(); ASSERT_GE(before.size(), 2);
    const auto property = model->metaObject()->property(model->metaObject()->indexOfProperty("alternatives"));
    EXPECT_NE(property.notifySignal().name(), QByteArray("stateChanged"));
    QSignalSpy listChanged(model.get(), property.notifySignal());
    const auto source = state().value("sourceKey");
    for (int i=0; i<before.size(); ++i) {
        model->selectAlternative(i);
        EXPECT_EQ(state().value("selectedAlternative").toInt(), i);
        EXPECT_EQ(state().value("pose").toMap().value("pitchText"), before[i].toMap().value("pitches"));
        EXPECT_EQ(state().value("sourceKey"), source);
    }
    EXPECT_EQ(listChanged.count(), 0);
    model->showOriginal(); EXPECT_EQ(state().value("selectedAlternative").toInt(), -1);
    EXPECT_EQ(pitches(), (QVariantList{48,52,55,71}));
}

TEST_F(MalletPanelWorkflow, SwitchingToPickingDoesNotClaimTheScoreStickingAppliesToPickedNotes) {
    auto* text=Factory::createSticking(first->segment()); text->setTrack(0); text->setPlainText(u"1 2 3 4"); first->segment()->add(text);
    model->refresh(); ASSERT_FALSE(state().value("sticking").toString().isEmpty());
    model->setPickMode(true);
    EXPECT_TRUE(state().value("sticking").toString().isEmpty());
    model->clearPicked(); model->togglePitch(61); model->togglePitch(66);
    EXPECT_EQ(pitches(), (QVariantList{61,66})); EXPECT_FALSE(state().value("canCommit").toBool());
    model->setPickMode(false); EXPECT_EQ(pitches(), (QVariantList{60,64,67,72}));
    EXPECT_EQ(state().value("sticking").toString(), "1 2 3 4");
}

TEST_F(MalletPanelWorkflow, SelectingADifferentChordBeforeTheRefreshTimerCannotPreviewOldSuggestions) {
    const auto old = state().value("sourceKey");
    selected={second->notes().front()}; selectionChanged.notify();
    model->selectAlternative(0);
    EXPECT_NE(state().value("sourceKey"), old);
    EXPECT_EQ(state().value("selectedAlternative").toInt(), -1);
    EXPECT_EQ(pitches(), (QVariantList{62,65,69,74}));
}

TEST_F(MalletPanelWorkflow, RejectedStaleCommitRefreshesTheDisplayedChord) {
    model->selectAlternative(0); const auto old=state().value("sourceKey");
    selected={second->notes().front()}; selectionChanged.notify(); model->commit();
    EXPECT_NE(state().value("sourceKey"), old);
    EXPECT_EQ(pitches(), (QVariantList{62,65,69,74}));
    EXPECT_TRUE(first->segment()->annotations().empty());
}

TEST_F(MalletPanelWorkflow, EmptyAndUnsupportedSelectionsGiveAnActionableExplanation) {
    selected.clear(); model->refresh();
    EXPECT_FALSE(state().value("canCommit").toBool()); EXPECT_FALSE(state().value("canAudition").toBool());
    EXPECT_FALSE(state().value("selectionHelp").toString().isEmpty());
    selected={first->notes().front()}; first->part()->instrument()->setId(u"piano"); model->refresh();
    EXPECT_FALSE(state().value("supported").toBool());
    EXPECT_TRUE(state().value("selectionHelp").toString().contains("keyboard percussion", Qt::CaseInsensitive));
}

TEST_F(MalletPanelWorkflow, DifficultOriginalAndSuggestionsExplainTheLimitAndTheTradeoffs) {
    model->setOption("opening", 5);
    EXPECT_FALSE(state().value("checks").toList().isEmpty());
    EXPECT_FALSE(state().value("nextStep").toString().isEmpty());
    for (const auto& value:model->alternativeRows()) {
        const auto row=value.toMap();
        EXPECT_FALSE(row.value("pros").toStringList().empty());
        EXPECT_FALSE(row.value("cons").toStringList().empty());
        EXPECT_FALSE(row.value("changes").toStringList().empty());
    }
}

TEST_F(MalletPanelWorkflow, MixedPreviewPickingSettingsAndReselectionRemainBoundedAndRepeatable) {
    for (int cycle=0;cycle<40;++cycle) {
        selected={cycle%2 ? second->notes().front() : first->notes().front()}; model->refresh();
        const auto original=pitches(); const auto source=state().value("sourceKey");
        for (int i=0;i<model->alternativeRows().size();++i) { model->selectAlternative(i); EXPECT_EQ(state().value("sourceKey"), source); }
        model->showOriginal(); EXPECT_EQ(pitches(), original);
        model->setPickMode(true); model->clearPicked(); for (int p:{60,64,67,72}) model->togglePitch(p);
        model->togglePitch(64); EXPECT_EQ(pitches(), (QVariantList{60,67,72}));
        model->setPickMode(false); EXPECT_EQ(pitches(), original); EXPECT_LE(model->alternativeRows().size(), 8);
    }
}

TEST_F(MalletPanelWorkflow, EditingAPickedPreviewStartsFromTheChordTheWriterIsActuallySeeing) {
    useWideChord();
    model->setPickMode(true); model->setOption("keepBass",false); model->setOption("keepMelody",false);
    int index=-1;
    for(const auto& row:model->alternativeRows()) if(row.toMap().value("pitches")!=state().value("displayedChord")) {index=row.toMap().value("index").toInt();break;}
    ASSERT_GE(index,0);model->selectAlternative(index);
    auto displayed=pitches();ASSERT_FALSE(displayed.empty());const int removed=displayed.front().toInt();
    model->togglePitch(removed);displayed.removeFirst();std::sort(displayed.begin(),displayed.end(),[](const QVariant&a,const QVariant&b){return a.toInt()<b.toInt();});
    EXPECT_EQ(pitches(),displayed);EXPECT_EQ(state().value("selectedAlternative").toInt(),-1);
    EXPECT_FALSE(state().value("canCommit").toBool());EXPECT_TRUE(first->segment()->annotations().empty());
}

TEST_F(MalletPanelWorkflow, ManualStrikePointsRemainAttachedToTheirPitchWhenPickedNotesReorder) {
    model->setPickMode(true); model->clearPicked(); model->togglePitch(64);model->togglePitch(67);
    const auto voices=state().value("pose").toMap().value("voices").toList();ASSERT_EQ(voices.size(),2);
    model->setStrikePoint(voices.front().toMap().value("mallet").toInt(),.86);
    model->togglePitch(60);
    for(const auto& value:state().value("pose").toMap().value("voices").toList()) {
        const auto voice=value.toMap();if(voice.value("pitch").toInt()==64)EXPECT_NEAR(voice.value("fraction").toDouble(),.86,.001);
        if(voice.value("pitch").toInt()==60)EXPECT_NEAR(voice.value("fraction").toDouble(),.5,.001);
    }
}

TEST_F(MalletPanelWorkflow, AuditionUsesTheDisplayedSuggestionAndStaleSelectionCannotPlayTheOldChord) {
    useWideChord();
    model->selectAlternative(0);const auto displayed=pitches();
    EXPECT_CALL(*playback,playNotes(_,0,first->segment(),_)).WillOnce([&](const NoteValList& values,auto,auto,const auto&) {
        ASSERT_EQ(values.size(),displayed.size());for(size_t i=0;i<values.size();++i)EXPECT_EQ(values[i].pitch,displayed[int(i)].toInt());
    });
    model->audition();Mock::VerifyAndClearExpectations(playback.get());
    selected={second->notes().front()};selectionChanged.notify();
    EXPECT_CALL(*playback,playNotes(_,_,_,_)).Times(0);model->audition();
    EXPECT_EQ(pitches(),(QVariantList{62,65,69,74}));
}

TEST_F(MalletPanelWorkflow, ClosingDuringCompareCancelsTheSecondAudition) {
    useWideChord();
    model->selectAlternative(0);
    EXPECT_CALL(*playback,playNotes(_,_,_,_)).Times(1);
    model->compare();model->setActive(false);QTest::qWait(700);
    EXPECT_TRUE(model->alternativeRows().empty());EXPECT_FALSE(model->active());
}

TEST_F(MalletPanelWorkflow, AManualStrikeRemainsProtectedWhenSwitchingSuggestions) {
    const auto voices=state().value("pose").toMap().value("voices").toList();ASSERT_EQ(voices.size(),4);
    model->setStrikePoint(voices.front().toMap().value("mallet").toInt(),.86);
    for(int i=0;i<model->alternativeRows().size();++i) {
        model->selectAlternative(i);
        const auto updated=state().value("pose").toMap().value("voices").toList();ASSERT_EQ(updated.size(),4);
        EXPECT_NEAR(updated.front().toMap().value("fraction").toDouble(),.86,.001);
    }
}

TEST_F(MalletPanelWorkflow, UnavailableAudioIsExplainedWithoutDisablingChordAnalysisOrCallingThePlayer) {
    ON_CALL(*playback,isPlaybackInited()).WillByDefault(Return(false));model->refresh();
    EXPECT_FALSE(state().value("canAudition").toBool());EXPECT_TRUE(state().value("supported").toBool());
    EXPECT_FALSE(state().value("audioStatus").toString().isEmpty());EXPECT_FALSE(pitches().empty());
    EXPECT_CALL(*playback,playNotes(_,_,_,_)).Times(0);model->audition();
}
