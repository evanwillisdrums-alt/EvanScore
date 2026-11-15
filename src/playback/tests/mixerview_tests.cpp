/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gmock/gmock.h>
#include "../qml/MuseScore/Playback/mixerpanelcontextmenumodel.h"
#include "mocks/playbackconfigurationmock.h"
using namespace testing;
using namespace mu::playback;
namespace {
class MixerViews : public Test {
protected:
    std::shared_ptr<NiceMock<PlaybackConfigurationMock>> config = std::make_shared<NiceMock<PlaybackConfigurationMock>>();
    std::map<MixerSectionType,bool> sections;
    std::map<muse::audio::aux_channel_idx_t,bool> sends;
    std::unique_ptr<MixerPanelContextMenuModel> model;
    void SetUp() override {
        muse::modularity::globalIoc()->registerExport<IPlaybackConfiguration>("mix-view-test",config);
        ON_CALL(*config,isMixerSectionVisible(_)).WillByDefault([this](auto section){return sections[section];});
        ON_CALL(*config,setMixerSectionVisible(_,_)).WillByDefault([this](auto section,bool v){sections[section]=v;});
        ON_CALL(*config,isAuxSendVisible(_)).WillByDefault([this](auto index){return sends[index];});
        ON_CALL(*config,setAuxSendVisible(_,_)).WillByDefault([this](auto index,bool v){sends[index]=v;});
        model = std::make_unique<MixerPanelContextMenuModel>();
    }
    void TearDown() override {
        model.reset();
        muse::modularity::globalIoc()->unregisterIfRegistered<IPlaybackConfiguration>("mix-view-test", config);
    }
};
TEST_F(MixerViews, ViewsChangePresentationWithoutTouchingSoundOrAuxChannels) {
    EXPECT_CALL(*config,setAuxChannelVisible(_,_)).Times(0);
    for(const QString view:{"mix","sounds","effects","all"}) {
        model->selectView(view); EXPECT_EQ(model->activeView(),view);
        EXPECT_TRUE(model->volumeSectionVisible()); EXPECT_TRUE(model->muteAndSoloSectionVisible()); EXPECT_TRUE(model->titleSectionVisible());
        EXPECT_EQ(model->faderSectionVisible(),view=="mix"||view=="all");
        EXPECT_EQ(model->soundSectionVisible(),view=="sounds"||view=="all");
        EXPECT_EQ(model->audioFxSectionVisible(),view=="effects"||view=="all");
        EXPECT_EQ(model->auxSendsSectionVisible(),view=="effects"||view=="all");
    }
}
TEST_F(MixerViews, ExistingCustomVisibilityIsNotOverwrittenByConstructionOrInvalidNames) {
    sections[MixerSectionType::AudioFX]=true;
    sends[1]=true;
    const auto before=sections; const auto beforeSends=sends;
    EXPECT_EQ(model->activeView(),"custom");
    // Reads may materialize missing keys in the test double, so compare values.
    model->selectView("invalid");
    for(const auto& [section,visible]:before) EXPECT_EQ(sections[section],visible);
    for(const auto& [index,visible]:beforeSends) EXPECT_EQ(sends[index],visible);
    EXPECT_TRUE(model->audioFxSectionVisible()); EXPECT_FALSE(model->faderSectionVisible());
}
TEST_F(MixerViews, RepeatedViewSelectionDoesNotRepublishUnchangedSettingsAndManualChangesBecomeCustom) {
    model->selectView("mix");
    EXPECT_CALL(*config,setMixerSectionVisible(_,_)).Times(0);
    EXPECT_CALL(*config,setAuxSendVisible(_,_)).Times(0);
    model->selectView("mix");
    sections[MixerSectionType::Sound]=true; EXPECT_EQ(model->activeView(),"custom");
}
}
