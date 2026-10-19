/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include <QQuickWindow>
#include <QPainter>
#include <QLineF>
#include <QSignalSpy>
#include <QShortcut>
#include <QTest>
#include <limits>
#include "../malletsceneview.h"
#include "../malletplacement.h"

using namespace mu::notation;
using namespace mu::notation::mallet;
namespace {
QVariantMap sceneData(const std::vector<int>& pitches, bool pick) {
    const auto keys=keyboard(36,96); Player player;player.optimizeStrikes=false;
    const auto pose=solve(keys,pitches,player).front();
    QVariantList bars,heads,notes,ids,wrists,anchors,shoulders;
    for(const auto& b:keys.bars)bars<<QVariantMap{{"pitch",b.pitch},{"x",b.x},{"y",b.y},{"width",b.width},{"length",b.length},{"accidental",b.accidental}};
    for(int i=0;i<4;++i)heads<<QVariantMap{{"id",i+1},{"active",pose.active[i]},{"x",pose.targets[i].x},{"y",pose.targets[i].y}};
    for(int p:pitches)notes<<p;for(int id:pose.mallets)ids<<id;
    for(const auto& w:pose.wrists)wrists<<QVariantMap{{"x",w.x},{"y",w.y}};
    for(const auto& a:pose.anchors)anchors<<QVariantMap{{"x",a.x},{"y",a.y}};
    for(const auto& s:pose.shoulders)shoulders<<QVariantMap{{"x",s.x},{"y",s.y}};
    return {{"bars",bars},{"keyboardWidth",keys.width},{"keyboardFront",keys.front},{"headDiameter",player.head},{"low",36},{"high",96},{"pickMode",pick},
        {"skin","#d99d78"},{"hair","#382b27"},{"pose",QVariantMap{{"pitches",notes},{"mallets",ids},{"heads",heads},{"wrists",wrists},{"anchors",anchors},{"shoulders",shoulders},{"body",QVariantMap{{"x",pose.body.x},{"y",pose.body.y}}}}}};
}
}
class MalletSceneInteraction : public ::testing::Test {
protected:
    QQuickWindow window;
    MalletSceneView view {window.contentItem()};
    void SetUp() override {
        window.resize(700,300);view.setSize({700,300});view.setFocusPlacement(false);
        view.setScene(sceneData({60,64,67,72},true));window.show();QTest::qWait(60);
    }
    void click(int pitch,bool overview=false) {QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,view.barPoint(pitch,overview).toPoint());}
};
TEST_F(MalletSceneInteraction, ClickingAnOccupiedHeadInPickingModeRemovesThatPitch) {
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked);click(60);
    ASSERT_EQ(clicks.count(),1);EXPECT_EQ(clicks.front().front().toInt(),60);
}
TEST_F(MalletSceneInteraction, NearbyMalletHeadsDoNotStealTheNextBarClickInACompactPanel) {
    view.setSize({700,124});QTest::qWait(50);
    ASSERT_LT(QLineF(view.barPoint(60),view.barPoint(62)).length(),9);
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked);click(62);
    ASSERT_EQ(clicks.count(),1);EXPECT_EQ(clicks.front().front().toInt(),62);
}
TEST_F(MalletSceneInteraction, CompactFocusEnlargesActiveBarsAndKeepsClicksAndOverviewAccurate) {
    view.setSize({700,124});QTest::qWait(50);
    const double full=QLineF(view.barPoint(60),view.barPoint(64)).length();
    view.setFocusPlacement(true);view.fit();QTest::qWait(50);
    EXPECT_GT(QLineF(view.barPoint(60),view.barPoint(64)).length(),full*1.4);
    const auto capture=qEnvironmentVariable("EVANSCORE_MALLET_COMPACT_CAPTURE");
    if(!capture.isEmpty()) {QImage image(view.size().toSize(),QImage::Format_ARGB32);QPainter painter(&image);view.paint(&painter);painter.end();EXPECT_TRUE(image.save(capture));}
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked);
    click(60);ASSERT_EQ(clicks.count(),1);EXPECT_EQ(clicks.back().front().toInt(),60);
    QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,QPoint(680,100));
    EXPECT_EQ(clicks.count(),1); // The player context inset is not a bar.
    view.setZoom(3.5);QTest::qWait(50);
    const auto hiddenHead=view.barPoint(72);
    ASSERT_GE(hiddenHead.x(),610);ASSERT_LE(hiddenHead.x(),700);
    ASSERT_GE(hiddenHead.y(),20);ASSERT_LE(hiddenHead.y(),124);
    QSignalSpy selected(&view,&MalletSceneView::malletSelected);
    QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,hiddenHead.toPoint());
    EXPECT_EQ(clicks.count(),1);EXPECT_EQ(selected.count(),0);
    view.setZoom(1);QTest::qWait(50);
    for(int pitch=36;pitch<=96;++pitch) {click(pitch,true);ASSERT_EQ(clicks.count(),pitch-34);EXPECT_EQ(clicks.back().front().toInt(),pitch);}
    const auto stable=view.barPoint(64);
    view.setScene(sceneData({60,64},true));QTest::qWait(50);
    EXPECT_NEAR(view.barPoint(64).x(),stable.x(),.01);EXPECT_NEAR(view.barPoint(64).y(),stable.y(),.01);
}
TEST_F(MalletSceneInteraction, DraggingAnOccupiedBarDoesNotRemoveItsNote) {
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked),drags(&view,&MalletSceneView::strikePointDragged);
    const auto start=view.barPoint(60).toPoint();
    QTest::mousePress(&window,Qt::LeftButton,Qt::NoModifier,start);
    QTest::mouseMove(&window,start+QPoint(0,20));
    QTest::mouseRelease(&window,Qt::LeftButton,Qt::NoModifier,start+QPoint(0,20));
    EXPECT_EQ(clicks.count(),0);ASSERT_GT(drags.count(),0);
}
TEST_F(MalletSceneInteraction, PickingNotesKeepsTheNextClickTargetStillUntilFitIsRequested) {
    view.setFocusPlacement(true);view.fit();QTest::qWait(50);const auto before=view.barPoint(64);
    view.setScene(sceneData({36,43},true));QTest::qWait(50);
    EXPECT_NEAR(view.barPoint(64).x(),before.x(),.01);EXPECT_NEAR(view.barPoint(64).y(),before.y(),.01);
    view.fit();QTest::qWait(50);EXPECT_GT(QLineF(view.barPoint(64),before).length(),5);
}
TEST_F(MalletSceneInteraction, OverviewCanPickEveryNaturalAndAccidentalAcrossTheFullRange) {
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked);
    for(int pitch=36;pitch<=96;++pitch) {click(pitch,true);ASSERT_EQ(clicks.count(),pitch-35);EXPECT_EQ(clicks.back().front().toInt(),pitch);}
}
TEST_F(MalletSceneInteraction, ScoreModeSelectsTheMalletWithoutTogglingTheChord) {
    view.setScene(sceneData({60,64,67,72},false));QTest::qWait(40);
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked),selected(&view,&MalletSceneView::malletSelected);
    click(60);EXPECT_EQ(clicks.count(),0);ASSERT_GT(selected.count(),0);EXPECT_EQ(selected.back().front().toInt(),1);
}
TEST_F(MalletSceneInteraction, DoubleClickRestoresTheCenterStrikeWithoutEditingMusic) {
    view.setScene(sceneData({60,64,67,72},false));QTest::qWait(40);
    QSignalSpy points(&view,&MalletSceneView::strikePointDragged),clicks(&view,&MalletSceneView::pitchClicked);
    QTest::mouseDClick(&window,Qt::LeftButton,Qt::NoModifier,view.barPoint(60).toPoint());
    ASSERT_EQ(points.count(),1);EXPECT_DOUBLE_EQ(points.front()[1].toDouble(),.5);EXPECT_EQ(clicks.count(),0);
}
TEST_F(MalletSceneInteraction, NonFiniteZoomCannotCorruptTheView) {
    view.setZoom(std::numeric_limits<double>::quiet_NaN());EXPECT_DOUBLE_EQ(view.zoom(),1);
    view.setZoom(20);EXPECT_DOUBLE_EQ(view.zoom(),4);view.setZoom(-10);EXPECT_DOUBLE_EQ(view.zoom(),1);
}

TEST_F(MalletSceneInteraction, ARepeatedPickClickIsNotSwallowedAsADoubleClick) {
    QSignalSpy clicks(&view,&MalletSceneView::pitchClicked);
    QTest::mouseDClick(&window,Qt::LeftButton,Qt::NoModifier,view.barPoint(60).toPoint());
    ASSERT_EQ(clicks.count(),2);EXPECT_EQ(clicks.back().front().toInt(),60);
}
