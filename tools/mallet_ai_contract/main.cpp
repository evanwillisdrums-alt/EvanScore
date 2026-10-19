/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <cstdio>
#include "malletassistant.h"
#include "malletplacement.h"
using namespace mu::notation;
using namespace mu::notation::mallet;
static QVariantMap facts(const Pose& pose,const QString& id,bool fixed=true) {
    QVariantList notes,ids,warnings;
    for(const auto& name:pose.names)notes<<QString::fromStdString(name);
    for(int mallet:pose.mallets)ids<<mallet;
    for(const auto& issue:pose.issues)warnings<<QString::fromStdString(issue.message);
    return {{"candidateId",id},{"valid",pose.valid},{"uncertain",pose.uncertain},{"severity",pose.severity},
        {"notesInSourceVoiceOrder",notes},{"physicalMallets",ids},{"warnings",warnings},
        {"leftOpeningCm",pose.openings[0]},{"rightOpeningCm",pose.openings[1]},
        {"leftPairTiltDegrees",pose.rotations[0]},{"rightPairTiltDegrees",pose.rotations[1]},
        {"keepsPhysicalLinks",fixed},{"physicalCost",pose.cost},{"musicalPreferenceCost",pose.musicalCost}};
}
static bool wait(MalletAssistant& client,int timeout) {
    QEventLoop loop;QTimer timer;timer.setSingleShot(true);
    QObject::connect(&client,&MalletAssistant::changed,&loop,[&](){
        QTimer::singleShot(0,&loop,[&](){if(!client.state().value("busy").toBool())loop.quit();});
    });
    QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
    if(client.state().value("busy").toBool()) {timer.start(timeout);loop.exec();}
    return !client.state().value("busy").toBool();
}
static bool unloaded() {
    QNetworkAccessManager network;
    for(int n=0;n<40;++n) {
        auto* reply=network.get(QNetworkRequest(QUrl("http://127.0.0.1:11435/api/ps")));
        QEventLoop loop;QTimer timeout;timeout.setSingleShot(true);
        QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
        QObject::connect(&timeout,&QTimer::timeout,&loop,&QEventLoop::quit);
        timeout.start(3000);loop.exec();
        const auto data=QJsonDocument::fromJson(reply->readAll());
        const bool empty=reply->error()==QNetworkReply::NoError && data.object().value("models").isArray()
            && data.object().value("models").toArray().empty();
        reply->deleteLater();if(empty)return true;
        QEventLoop delay;QTimer::singleShot(250,&delay,&QEventLoop::quit);delay.exec();
    }
    return false;
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    const bool setup=app.arguments().contains("--setup");
    MalletAssistant client(nullptr,QUrl(setup?"http://127.0.0.1:11434":"http://127.0.0.1:11435"));
    QObject::connect(&client,&MalletAssistant::changed,&app,[&]() {
        std::printf("AI_STATUS %s\n",qPrintable(client.state().value("status").toString()));std::fflush(stdout);
    });
    client.setEnabled(true);
    if(setup) {
        client.setup();
        if(!wait(client,31*60*1000) || !client.state().value("managedReady").toBool()) {
            std::fprintf(stderr,"Actual native setup failed: %s\n",qPrintable(client.state().value("backendLog").toString()));return 5;
        }
    } else client.discover();
    if(!wait(client,10000) || client.state().value("models").toList().empty()) {std::fprintf(stderr,"Local models unavailable\n");return 1;}
    client.setModel("qwen2.5:1.5b");
    const auto keys=keyboard(36,96);const Player player;
    const auto comfortable=solve(keys,{60,64,67,72},player).front();
    if(!comfortable.valid || comfortable.severity!=0)return 7;
    const auto original=facts(comfortable,"original");
    client.ask({{"candidates",QVariantList{original}},{"originalComfortable",true},
        {"openingLimitCm",24},{"tiltLimitDegrees",40},{"contextLimits","Static estimated geometry; continuous motion and human calibration are unproven"},
        {"searchOrder","Keep original; no useful revoicing is needed"}});
    if(!wait(client,170000)) {std::fprintf(stderr,"Real model timed out\n");return 2;}
    const auto result=client.state().value("result").toMap();
    if(result.value("candidateId").toString()!="original") {std::fprintf(stderr,"Real model did not return checked original: %s\n",qPrintable(client.state().value("status").toString()));return 3;}
    if(!unloaded()) {std::fprintf(stderr,"Model did not unload after the native request\n");return 6;}
    QVariantList responses{client.state()};
    // Restart the app-owned service using its saved setup, then ask about an
    // actual solver-generated difficult chord and checked fixed-link revoicing.
    if(setup) {client.setEnabled(false);client.setEnabled(true);client.discover();if(!wait(client,25000))return 8;}
    const std::vector<int> wide{48,52,55,71};const auto wideOriginal=solve(keys,wide,player).front();
    const auto suggestions=alternatives(keys,wide,player);
    if(suggestions.empty() || suggestions.front().mallets!=wideOriginal.mallets || suggestions.front().severity>=2)return 9;
    const auto revised=facts(suggestions.front(),"alternative-0");
    client.ask({{"candidates",QVariantList{facts(wideOriginal,"original"),revised}},{"originalComfortable",false},
        {"openingLimitCm",24},{"tiltLimitDegrees",40},{"contextLimits","Static estimated geometry; continuous motion and human calibration are unproven"},
        {"searchOrder","Prefer the checked, comfortable octave revoicing with fixed links over the original difficult opening"}});
    if(!wait(client,170000) || client.state().value("result").toMap().value("candidateId").toString()!="alternative-0") {
        std::fprintf(stderr,"Actual difficult-chord inference failed: %s\n",QJsonDocument::fromVariant(client.state()).toJson().constData());return 10;
    }
    if(!unloaded())return 11;
    responses<<client.state();
    auto report=client.state();report.insert("responses",responses);
    const auto bytes=QJsonDocument::fromVariant(report).toJson();
    if(argc>1) {QFile file(QString::fromLocal8Bit(argv[argc-1]));if(!file.open(QIODevice::WriteOnly))return 4;file.write(bytes);}
    std::printf("REAL_LOCAL_AI_PASSED\n%s\n",bytes.constData());
    return 0;
}
