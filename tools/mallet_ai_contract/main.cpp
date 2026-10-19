/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QTimer>
#include <cstdio>
#include "malletassistant.h"
using namespace mu::notation;
static bool wait(MalletAssistant& client,int timeout) {
    QEventLoop loop;QTimer timer;timer.setSingleShot(true);
    QObject::connect(&client,&MalletAssistant::changed,&loop,[&](){if(!client.state().value("busy").toBool())loop.quit();});
    QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
    if(client.state().value("busy").toBool()) {timer.start(timeout);loop.exec();}
    return !client.state().value("busy").toBool();
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);MalletAssistant client(nullptr,QUrl("http://127.0.0.1:11435"));
    client.setEnabled(true);client.discover();
    if(!wait(client,10000) || client.state().value("models").toList().empty()) {std::fprintf(stderr,"Local models unavailable\n");return 1;}
    client.setModel("qwen2.5:1.5b");
    const QVariantMap original{{"candidateId","original"},{"valid",true},{"uncertain",false},{"severity",0},
        {"notes","C4 E4 G4 C5"},{"physicalMallets",QVariantList{1,2,3,4}},
        {"leftOpeningCm",11.0},{"rightOpeningCm",14.0},{"leftPairTiltDegrees",2.0},{"rightPairTiltDegrees",3.0},
        {"keepsPhysicalLinks",true},{"facts","Every head is separated. Both hand openings are below 24 cm and pair tilts below 40 degrees. No crossing warning. This original chord is comfortable; keep it."}};
    client.ask({{"candidates",QVariantList{original}},{"originalComfortable",true},{"searchOrder","Keep original; no useful revoicing is needed"}});
    if(!wait(client,170000)) {std::fprintf(stderr,"Real model timed out\n");return 2;}
    const auto result=client.state().value("result").toMap();
    if(result.value("candidateId").toString()!="original") {std::fprintf(stderr,"Real model did not return checked original: %s\n",qPrintable(client.state().value("status").toString()));return 3;}
    const auto bytes=QJsonDocument::fromVariant(client.state()).toJson();
    if(argc>1) {QFile file(QString::fromLocal8Bit(argv[1]));if(!file.open(QIODevice::WriteOnly))return 4;file.write(bytes);}
    std::printf("REAL_LOCAL_AI_PASSED\n%s\n",bytes.constData());
    return 0;
}
