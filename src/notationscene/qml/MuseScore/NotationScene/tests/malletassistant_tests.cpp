/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include <memory>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include "../malletassistant.h"
#include "../processmemory.h"
using namespace mu::notation;
namespace {
QVariantMap context() {
    return {{"candidates",QVariantList{QVariantMap{{"candidateId","original"},{"valid",true},{"uncertain",false},{"severity",0}},
        QVariantMap{{"candidateId","alternative-0"},{"valid",false},{"uncertain",false},{"severity",2}}}}};
}
QJsonObject guidance() {
    return {{"candidateId","original"},{"benefits",QJsonArray{"Keeps the original notes and physical links."}},
        {"tradeoffs",QJsonArray{"Static geometry is uncalibrated."}},{"sourceIds",QJsonArray{"USER","MODEL"}}};
}
QByteArray envelope(const QJsonObject& value) {
    return QJsonDocument(QJsonObject{{"done",true},{"message",QJsonObject{{"role","assistant"},
        {"content",QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact))}}}}).toJson();
}
bool idle(MalletAssistant& client) {
    QElapsedTimer timer;timer.start();
    while(client.state().value("busy").toBool() && timer.elapsed()<3000)QTest::qWait(10);
    return !client.state().value("busy").toBool();
}
class LocalService {
public:
    QTcpServer server;
    QList<QByteArray> paths,bodies;
    QByteArray chat=envelope(guidance());
    bool hold=false;
    int status=200;
    LocalService() {
        server.listen(QHostAddress::LocalHost,0);
        QObject::connect(&server,&QTcpServer::newConnection,&server,[this]() {
            auto* socket=server.nextPendingConnection();
            auto bytes=std::make_shared<QByteArray>();
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[this,socket,bytes]() {
                *bytes+=socket->readAll();const auto end=bytes->indexOf("\r\n\r\n");if(end<0)return;
                int length=0;for(const auto& line:bytes->left(end).split('\n'))if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toInt();
                if(bytes->size()<end+4+length)return;
                const auto path=bytes->split(' ').value(1);paths<<path;bodies<<bytes->mid(end+4,length);bytes->clear();
                if(hold)return;
                const auto model=[](const QString& name,double size,const QString& format="gguf") {
                    return QJsonObject{{"name",name},{"size",size},{"details",QJsonObject{{"format",format}}}};
                };
                const auto payload=path=="/api/tags"?QJsonDocument(QJsonObject{{"models",QJsonArray{
                    model("qwen2.5:1.5b",1000000000),model("too-large",9000000000),model("remote:cloud",100000000),
                    model("remote-alias",500),model("non-gguf",1000000000,"other")}}}).toJson():chat;
                socket->write("HTTP/1.1 "+QByteArray::number(status)+" OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: "+QByteArray::number(payload.size())+"\r\n\r\n"+payload);
                socket->disconnectFromHost();
            });
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        });
    }
    QUrl endpoint() const {return QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort()));}
};
}
TEST(MalletAssistant, DisabledAndEnabledIdleNeverContactOrLoadAModel) {
    LocalService service;ASSERT_TRUE(service.server.isListening());MalletAssistant client(nullptr,service.endpoint());
    client.discover();client.ask(context());QTest::qWait(30);EXPECT_TRUE(service.paths.empty());
    client.setEnabled(true);QTest::qWait(30);EXPECT_TRUE(service.paths.empty());
}
TEST(MalletAssistant, DiscoveryListsOnlySmallLocalWeightsWithoutInference) {
    LocalService service;MalletAssistant client(nullptr,service.endpoint());client.setEnabled(true);client.discover();ASSERT_TRUE(idle(client));
    ASSERT_EQ(service.paths.size(),1);EXPECT_EQ(service.paths.front(),"/api/tags");
    const auto models=client.state().value("models").toList();ASSERT_EQ(models.size(),1);
    EXPECT_EQ(models.front().toMap().value("value").toString(),"qwen2.5:1.5b");
}
TEST(MalletAssistant, RequestHasBoundedContextOutputAndImmediateUnload) {
    const auto body=QJsonDocument::fromJson(MalletAssistant::requestBody("local",context())).object();
    EXPECT_EQ(body.value("keep_alive").toInt(),0);EXPECT_FALSE(body.value("stream").toBool());
    const auto options=body.value("options").toObject();EXPECT_EQ(options.value("num_ctx").toInt(),4096);
    EXPECT_EQ(options.value("num_predict").toInt(),384);EXPECT_EQ(options.value("temperature").toInt(),0);
    EXPECT_EQ(options.value("num_gpu").toInt(),0);
    const auto format=body.value("format").toObject();EXPECT_FALSE(format.value("additionalProperties").toBool(true));
    EXPECT_EQ(format.value("properties").toObject().value("candidateId").toObject().value("enum").toArray(),(QJsonArray{"none","original"}));
    EXPECT_TRUE(body.value("messages").toArray().first().toObject().value("content").toString().contains("not independently verified"));
}
TEST(MalletAssistant, ResultMustIdentifyAValidatedCandidateAndAvailableSources) {
    EXPECT_FALSE(MalletAssistant::validateResponse(envelope(guidance()),context()).contains("error"));
    for(const QString id:{"invented","alternative-0",""}) {auto value=guidance();value["candidateId"]=id;EXPECT_TRUE(MalletAssistant::validateResponse(envelope(value),context()).contains("error"));}
    auto value=guidance();value["notes"]=QJsonArray{60,64,67,72};EXPECT_TRUE(MalletAssistant::validateResponse(envelope(value),context()).contains("error"));
    value=guidance();value["sourceIds"]=QJsonArray{"Unread book, page 27"};EXPECT_TRUE(MalletAssistant::validateResponse(envelope(value),context()).contains("error"));
    value=guidance();value["tradeoffs"]=QJsonArray{};EXPECT_TRUE(MalletAssistant::validateResponse(envelope(value),context()).contains("error"));
    value=guidance();value["candidateId"]="none";EXPECT_EQ(MalletAssistant::validateResponse(envelope(value),context()).value("candidateId").toString(),"none");
    EXPECT_TRUE(MalletAssistant::validateResponse("{}",context()).contains("error"));
    EXPECT_TRUE(MalletAssistant::validateResponse(QByteArray(300000,'x'),context()).contains("error"));
}
TEST(MalletAssistant, AiCannotOverruleComfortAndFixedLinkRecommendationChecks) {
    auto original=context().value("candidates").toList().front().toMap();
    original.insert("severity",1);original.insert("keepsPhysicalLinks",true);original.insert("physicalCost",111.0);
    auto kept=original;kept.insert("candidateId","alternative-0");kept.insert("severity",0);kept.insert("physicalCost",13.0);
    auto changed=kept;changed.insert("candidateId","alternative-1");changed.insert("keepsPhysicalLinks",false);changed.insert("physicalCost",8.0);
    QVariantMap facts{{"candidates",QVariantList{original,kept,changed}},{"originalComfortable",false}};
    const auto body=QJsonDocument::fromJson(MalletAssistant::requestBody("local",facts)).object();
    EXPECT_EQ(body.value("format").toObject().value("properties").toObject().value("candidateId").toObject().value("enum").toArray(),(QJsonArray{"none","alternative-0"}));
    const auto supplied=QJsonDocument::fromJson(body.value("messages").toArray().last().toObject().value("content").toString().toUtf8()).object();
    EXPECT_EQ(supplied.value("candidates").toArray().size(),1);EXPECT_TRUE(supplied.contains("originalForComparisonOnly"));
    EXPECT_TRUE(MalletAssistant::validateResponse(envelope(guidance()),facts).contains("error"));
    auto answer=guidance();answer.insert("candidateId","alternative-0");
    EXPECT_FALSE(MalletAssistant::validateResponse(envelope(answer),facts).contains("error"));
    // Even equal warning levels cannot select a materially worse score.
    original.insert("severity",0);facts.insert("candidates",QVariantList{original,kept});
    EXPECT_TRUE(MalletAssistant::validateResponse(envelope(guidance()),facts).contains("error"));
    // A comfortable original stays the recommendation despite a lower cost.
    facts.insert("originalComfortable",true);
    EXPECT_FALSE(MalletAssistant::validateResponse(envelope(guidance()),facts).contains("error"));
    EXPECT_TRUE(MalletAssistant::validateResponse(envelope(answer),facts).contains("error"));
}
TEST(MalletAssistant, RealLocalHttpRoundTripAcceptsOnlyStructuredCheckedGuidance) {
    LocalService service;MalletAssistant client(nullptr,service.endpoint());client.setEnabled(true);client.discover();ASSERT_TRUE(idle(client));
    client.ask(context());ASSERT_TRUE(idle(client));ASSERT_EQ(service.paths.size(),2);EXPECT_EQ(service.paths.back(),"/api/chat");
    EXPECT_EQ(client.state().value("result").toMap().value("candidateId").toString(),"original");
    EXPECT_EQ(QJsonDocument::fromJson(service.bodies.back()).object().value("keep_alive").toInt(),0);
    service.chat=envelope(QJsonObject{{"candidateId","made-up"}});client.ask(context());ASSERT_TRUE(idle(client));
    EXPECT_TRUE(client.state().value("result").toMap().empty());
}
TEST(MalletAssistant, SelectionInvalidationCancelsPendingInferenceAndClearsOldGuidance) {
    LocalService service;MalletAssistant client(nullptr,service.endpoint());client.setEnabled(true);client.discover();ASSERT_TRUE(idle(client));
    service.hold=true;client.ask(context());QTest::qWait(30);ASSERT_TRUE(client.state().value("busy").toBool());
    client.invalidate();EXPECT_FALSE(client.state().value("busy").toBool());EXPECT_TRUE(client.state().value("result").toMap().empty());
    client.setEnabled(false);client.ask(context());QTest::qWait(30);EXPECT_EQ(service.paths.size(),2);
}
TEST(MalletAssistant, NetworkFailureAndOversizedResponseLeaveOrdinaryAnalysisAvailable) {
    LocalService service;MalletAssistant client(nullptr,service.endpoint());client.setEnabled(true);client.discover();ASSERT_TRUE(idle(client));
    service.status=503;client.ask(context());ASSERT_TRUE(idle(client));EXPECT_TRUE(client.state().value("result").toMap().empty());
    service.status=200;service.chat=QByteArray(300000,'x');client.ask(context());ASSERT_TRUE(idle(client));EXPECT_TRUE(client.state().value("result").toMap().empty());
}
TEST(MalletAssistant, RemoteEndpointsAreRejectedBeforeSendingAnything) {
    MalletAssistant client(nullptr,QUrl("https://example.com"));client.setEnabled(true);client.discover();
    EXPECT_FALSE(client.state().value("busy").toBool());EXPECT_TRUE(client.state().value("endpoint").toString().isEmpty());
}
TEST(MalletAssistant, MemoryDiagnosticsDistinguishSystemLoadFromProcessWorkingSet) {
    const auto memory=processMemoryDiagnostics();
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    ASSERT_TRUE(memory.contains("systemUsedPercent"));ASSERT_TRUE(memory.contains("workingSetBytes"));
    EXPECT_GT(memory.value("systemTotalBytes").toDouble(),0);
    EXPECT_GE(memory.value("systemAvailableBytes").toDouble(),0);
    EXPECT_LE(memory.value("systemAvailableBytes").toDouble(),memory.value("systemTotalBytes").toDouble());
    EXPECT_GE(memory.value("systemUsedPercent").toDouble(),0);EXPECT_LE(memory.value("systemUsedPercent").toDouble(),100);
    EXPECT_GT(memory.value("workingSetBytes").toDouble(),0);
    EXPECT_GT(memory.value("processPhysicalMemoryPercent").toDouble(),0);
    EXPECT_LE(memory.value("processPhysicalMemoryPercent").toDouble(),100);
#endif
}
