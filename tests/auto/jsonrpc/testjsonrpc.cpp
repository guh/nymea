/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "nymeatestbase.h"
#include "../../utils/pushbuttonagent.h"
#include "nymeacore.h"
#include "version.h"
#include "servers/mocktcpserver.h"
#include "usermanager/usermanager.h"
#include "nymeadbusservice.h"

using namespace nymeaserver;

class TestJSONRPC: public NymeaTestBase
{
    Q_OBJECT

private:
    inline void verifyDeviceError(const QVariant &response, Thing::ThingError error = Thing::ThingErrorNoError) {
        verifyError(response, "deviceError", enumValueName(error));
    }
    inline void verifyRuleError(const QVariant &response, RuleEngine::RuleError error = RuleEngine::RuleErrorNoError) {
        verifyError(response, "ruleError", enumValueName(error));
    }

private slots:
    void initTestCase();
    void cleanup();

    void testHandshake();

    void testHandshakeLocale();

    void testInitialSetup();

    void testRevokeToken();

    void testBasicCall_data();
    void testBasicCall();

    void introspect();

    void enableDisableNotifications_legacy_data();
    void enableDisableNotifications_legacy();

    void ruleAddedRemovedNotifications();

    void ruleActiveChangedNotifications();

    void stateChangeEmitsNotifications();

    void pluginConfigChangeEmitsNotification();

    /*
    Cases for push button auth:

    Case 1: regular pushbutton
    - alice sends JSONRPC.RequestPushButtonAuth, gets "OK" back (if push button hardware is available)
    - alice pushes the hardware button and gets a notification on jsonrpc containing the token for local auth
    */
    void testPushButtonAuth();

    /*
    Case 2: if we have an attacker in the network, he could try to call requestPushButtonAuth and
    hope someone would eventually press the button and give him a token. In order to prevent this
    any previous attempt for a push button auth needs to be cancelled when a new request comes in:

    * Mallory does RequestPushButtonAuth, gets OK back
    * Alice does RequestPushButtonAuth,
    * Mallory receives a "PushButtonFailed" notification
    * Alice receives OK
    * alice presses the hardware button
    * Alice reveices a notification with token, mallory receives nothing

    Case 3: Mallory tries to hijack it back again

    * Mallory does RequestPushButtonAuth, gets OK back
    * Alice does RequestPusButtonAuth,
    * Alice gets ok reply, Mallory gets failed notification
    * Mallory quickly does RequestPushButtonAuth again to win the fight
    * Alice gets failed notification and can instruct the user to _not_ press the button now until procedure is restarted
    */
    void testPushButtonAuthInterrupt();

    void testPushButtonAuthConnectionDrop();

    void testInitialSetupWithPushButtonAuth();

    void testDataFragmentation_data();
    void testDataFragmentation();

    void testGarbageData();

private:
    QStringList extractRefs(const QVariant &variant);

};

QStringList TestJSONRPC::extractRefs(const QVariant &variant)
{
    if (variant.canConvert(QVariant::String)) {
        if (variant.toString().startsWith("$ref")) {
            return QStringList() << variant.toString();
        }
    }
    if (variant.canConvert(QVariant::List)) {
        QStringList refs;
        foreach (const QVariant tmp, variant.toList()) {
            refs << extractRefs(tmp);
        }
        return refs;
    }
    if (variant.canConvert(QVariant::Map)) {
        QStringList refs;
        foreach (const QVariant tmp, variant.toMap()) {
            refs << extractRefs(tmp);
        }
        return refs;
    }
    return QStringList();
}

void TestJSONRPC::initTestCase()
{
    NymeaDBusService::setBusType(QDBusConnection::SessionBus);
    NymeaTestBase::initTestCase();
    QLoggingCategory::setFilterRules("*.debug=false\n"
//                                     "JsonRpcTraffic.debug=true\n"
                                     "JsonRpc.debug=true\n"
                                     "Translations.debug=true\n"
                                     "Tests.debug=true");
}

void TestJSONRPC::cleanup()
{
    while (NymeaCore::instance()->logEngine()->jobsRunning()) {
        qApp->processEvents();
    }
}

void TestJSONRPC::testHandshake()
{
    QUuid newClientId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(newClientId);
    qApp->processEvents();

    // Check the Hello reply
    QVariantMap handShake = injectAndWait("JSONRPC.Hello", QVariantMap(), newClientId).toMap();
    QString nymeaVersionString(NYMEA_VERSION_STRING);
    QVERIFY2(handShake.value("params").toMap().value("version").toString() == nymeaVersionString, "Handshake version doesn't match nymea version.");

    // Check whether pushButtonAuth is disabled
    QCOMPARE(handShake.value("pushButtonAuthAvailable").toBool(), false);

    // Now register push button agent
    PushButtonAgent pushButtonAgent;
    pushButtonAgent.init(QDBusConnection::SessionBus);

    // And now check if it is sent again when calling JSONRPC.Hello
    handShake = injectAndWait("JSONRPC.Hello").toMap();
    QCOMPARE(handShake.value("params").toMap().value("version").toString(), nymeaVersionString);

    // Check whether pushButtonAuth is now enabled
    QCOMPARE(handShake.value("params").toMap().value("pushButtonAuthAvailable").toBool(), true);

    emit m_mockTcpServer->clientDisconnected(newClientId);


    // And now check if it is sent again when calling JSONRPC.Hello
    handShake = injectAndWait("JSONRPC.Hello").toMap();
    QCOMPARE(handShake.value("params").toMap().value("version").toString(), nymeaVersionString);
}

void TestJSONRPC::testHandshakeLocale()
{
    // first test if the handshake message is auto-sent upon connecting
    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    // Test withouth locale data
    QVariantMap handShake = injectAndWait("JSONRPC.Hello").toMap();
    QCOMPARE(handShake.value("params").toMap().value("locale").toString(), QString("en_US"));

    QVariantMap supportedDevices = injectAndWait("Integrations.GetThingClasses").toMap();
    bool found = false;
    foreach (const QVariant &dcMap, supportedDevices.value("params").toMap().value("thingClasses").toList()) {
        if (dcMap.toMap().value("id").toUuid() == autoMockThingClassId) {
            QCOMPARE(dcMap.toMap().value("displayName").toString(), QString("Mocked Thing (Auto created)"));
            found = true;
        }
    }
    QVERIFY(found);

    // And now with locale info
    QVariantMap params;
    params.insert("locale", "de_DE");
    handShake = injectAndWait("JSONRPC.Hello", params).toMap();
    QCOMPARE(handShake.value("params").toMap().value("locale").toString(), QString("de_DE"));

    supportedDevices = injectAndWait("Integrations.GetThingClasses").toMap();
    found = false;
    foreach (const QVariant &dcMap, supportedDevices.value("params").toMap().value("thingClasses").toList()) {
        if (dcMap.toMap().value("id").toUuid() == autoMockThingClassId) {
            QCOMPARE(dcMap.toMap().value("displayName").toString(), QString("Mock \"Thing\" (automatisch erstellt)"));
            found = true;
        }
    }
    QVERIFY(found);
}

void TestJSONRPC::testInitialSetup()
{
    foreach (const UserInfo &userInfo, NymeaCore::instance()->userManager()->users()) {
        NymeaCore::instance()->userManager()->removeUser(userInfo.username());
    }
    NymeaCore::instance()->userManager()->removeUser("");

    QVERIFY(NymeaCore::instance()->userManager()->initRequired());
    QCOMPARE(NymeaCore::instance()->userManager()->users().count(), 0);

    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));
    QVERIFY(spy.isValid());
    QSignalSpy connectedSpy(m_mockTcpServer, &MockTcpServer::clientConnected);
    QSignalSpy disconnectedSpy(m_mockTcpServer, &MockTcpServer::clientDisconnected);

    // Introspect call should work in any case
    qCDebug(dcTests()) << "Calling Introspect, expecting success";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Introspect\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    QVariantMap response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Result:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));


    // Hello call should work in any case too
    spy.clear();
    qCDebug(dcTests()) << "Calling Hello, expecting success";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Hello\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Result:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.value("params").toMap().value("initialSetupRequired").toBool(), true);

    // Any other call should fail with "unauthorized" even if we use a previously valid token
    spy.clear();
    disconnectedSpy.clear();
    qCDebug(dcTests()) << "Calling Version, expecting failure (unauthenticated)";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"token\": \"" + m_apiToken + "\", \"method\": \"JSONRPC.Version\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Result:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("unauthorized"));

    // Connection should terminate
    if (disconnectedSpy.count() == 0) disconnectedSpy.wait();
    QCOMPARE(disconnectedSpy.count(), 1);
    qCDebug(dcTests()) << "Mock client disconnected";
    connectedSpy.clear();
    emit m_mockTcpServer->clientConnected(m_clientId);
    if (connectedSpy.count() == 0) connectedSpy.wait();
    QCOMPARE(connectedSpy.count(), 1);
    qCDebug(dcTests()) << "Mock client connected";

    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);

    // Except CreateUser

    // But it should still fail when giving a an invalid username
    spy.clear();
    qCDebug(dcTests()) << "Calling CreateUser, expecting failure (bad username)";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.CreateUser\", \"params\": {\"username\": \"dummy\", \"password\": \"DummyPW1!\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling CreateUser on uninitialized instance with invalid user:" << response.value("status").toString() << response.value("params").toMap().value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(NymeaCore::instance()->userManager()->users().count(), 0);

    // or when giving a bad password
    spy.clear();
    qCDebug(dcTests()) << "Calling CreateUser, expecting failure (bad password)";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.CreateUser\", \"params\": {\"username\": \"dummy@guh.io\", \"password\": \"weak\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling CreateUser on uninitialized instance with weak password:" << response.value("status").toString() << response.value("params").toMap().value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(NymeaCore::instance()->userManager()->users().count(), 0);

    // Now lets play by the rules (with an uppercase email)
    spy.clear();
    qCDebug(dcTests()) << "Calling CreateUser, expecting success";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.CreateUser\", \"params\": {\"username\": \"Dummy@guh.io\", \"password\": \"DummyPW1!\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests) << "Calling CreateUser on uninitialized instance:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(NymeaCore::instance()->userManager()->users().count(), 1);

    // Now that we have a user, initialSetup should be false in the Hello call
    spy.clear();
    qCDebug(dcTests()) << "Calling Hello, expecting success";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Hello\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests) << "Calling Hello on initialized instance:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.value("params").toMap().value("initialSetupRequired").toBool(), false);

    // Calls should still fail, given we didn't get a new token yet
    spy.clear();
    disconnectedSpy.clear();
    qCDebug(dcTests()) << "Calling Version, expecting failure (bad token)";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"token\": \"" + m_apiToken + "\", \"method\": \"JSONRPC.Version\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests) << "Calling Version with old token:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("unauthorized"));

    // Connection should terminate
    if (disconnectedSpy.count() == 0) disconnectedSpy.wait();
    QCOMPARE(disconnectedSpy.count(), 1);
    qCDebug(dcTests()) << "Mock client disconnected";
    connectedSpy.clear();
    emit m_mockTcpServer->clientConnected(m_clientId);
    if (connectedSpy.count() == 0) connectedSpy.wait();
    QCOMPARE(connectedSpy.count(), 1);
    qCDebug(dcTests()) << "Mock client connected";

    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);


    // Now lets authenticate with a wrong user
    spy.clear();
    qCDebug(dcTests()) << "Calling Authenticate, expecting failure (bad user)";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Authenticate\", \"params\": {\"username\": \"Dummy@wrong.domain\", \"password\": \"DummyPW1!\", \"deviceName\": \"testcase\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Authenticate with wrong user:" << response.value("params").toMap().value("success").toString() << response.value("params").toMap().value("token").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.value("params").toMap().value("success").toBool(), false);
    QVERIFY(response.value("params").toMap().value("token").toByteArray().isEmpty());


    // Now lets authenticate with a wrong password
    spy.clear();
    qCDebug(dcTests()) << "Calling Authenticate, expecting failure (bad password)";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Authenticate\", \"params\": {\"username\": \"Dummy@guh.io\", \"password\": \"wrongpw\", \"deviceName\": \"testcase\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Authenticate with wrong password:" << response.value("params").toMap().value("success").toString() << response.value("params").toMap().value("token").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.value("params").toMap().value("success").toBool(), false);
    QVERIFY(response.value("params").toMap().value("token").toByteArray().isEmpty());


    // Now lets authenticate for real (but intentionally use a lowercase email here, should still work)
    spy.clear();
    qCDebug(dcTests()) << "Calling Authenticate, expecting success";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Authenticate\", \"params\": {\"username\": \"dummy@guh.io\", \"password\": \"DummyPW1!\", \"deviceName\": \"testcase\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Authenticate with valid credentials:" << response.value("params").toMap().value("success").toString() << response.value("params").toMap().value("token").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.value("params").toMap().value("success").toBool(), true);
    m_apiToken = response.value("params").toMap().value("token").toByteArray();
    QVERIFY(!m_apiToken.isEmpty());

    // Now do a Version call with the valid token and it should work
    spy.clear();
    qCDebug(dcTests()) << "Calling Version, expecting success";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"token\": \"" + m_apiToken + "\", \"method\": \"JSONRPC.Version\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Version with valid token:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));

}

void TestJSONRPC::testRevokeToken()
{
    QSignalSpy spy(m_mockTcpServer, &MockTcpServer::outgoingData);
    QVERIFY(spy.isValid());
    QSignalSpy disconnectedSpy(m_mockTcpServer, &MockTcpServer::clientDisconnected);
    QVERIFY(disconnectedSpy.isValid());
    QSignalSpy connectedSpy(m_mockTcpServer, &MockTcpServer::clientConnected);
    QVERIFY(connectedSpy.isValid());

    // Now get all the tokens
    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 123, \"token\": \"" + m_apiToken + "\", \"method\": \"JSONRPC.Tokens\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    QVariantMap response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Getting existing Tokens" << response.value("status").toString() << response;
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QVariantList tokenList = response.value("params").toMap().value("tokenInfoList").toList();
    QCOMPARE(tokenList.count(), 1);
    QUuid oldTokenId = tokenList.first().toMap().value("id").toUuid();

    // Authenticate and create a new token
    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Authenticate\", \"params\": {\"username\": \"dummy@guh.io\", \"password\": \"DummyPW1!\", \"deviceName\": \"testcase\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Authenticate with valid credentials:" << response.value("params").toMap().value("success").toString() << response.value("params").toMap().value("token").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.value("params").toMap().value("success").toBool(), true);
    QByteArray newToken = response.value("params").toMap().value("token").toByteArray();
    QVERIFY(!newToken.isEmpty());

    // Now do a Version call with the new token and it should work
    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"token\": \"" + newToken + "\", \"method\": \"JSONRPC.Version\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Version with valid token:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));

    // Now get all the tokens using the old token
    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 123, \"token\": \"" + m_apiToken + "\", \"method\": \"JSONRPC.Tokens\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Tokens" << response.value("status").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));
    tokenList = response.value("params").toMap().value("tokenInfoList").toList();
    QCOMPARE(tokenList.count(), 2);

    // find the new token
    QUuid newTokenId;
    foreach (const QVariant &tokenInfo, tokenList) {
        if (tokenInfo.toMap().value("id").toUuid() != oldTokenId) {
            newTokenId = tokenInfo.toMap().value("id").toUuid();
            break;
        }
    }

    // Revoke the new token
    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 123, \"token\": \"" + m_apiToken + "\", \"method\": \"JSONRPC.RemoveToken\", \"params\": {\"tokenId\": \"" + newTokenId.toByteArray() + "\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling RemoveToken" << response.value("status").toString() << response;
    QCOMPARE(response.value("status").toString(), QStringLiteral("success"));

    // Do a call with the now removed token, it should be forbidden
    spy.clear();
    disconnectedSpy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"token\": \"" + newToken + "\", \"method\": \"JSONRPC.Version\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant().toMap();
    qCDebug(dcTests()) << "Calling Version with valid token:" << response.value("status").toString() << response.value("error").toString();
    QCOMPARE(response.value("status").toString(), QStringLiteral("unauthorized"));

    // And connection should drop
    if (disconnectedSpy.count() == 0) disconnectedSpy.wait();
    QCOMPARE(disconnectedSpy.count(), 1);

    // Connect again to not impact subsequent tests...
    connectedSpy.clear();
    emit m_mockTcpServer->clientConnected(m_clientId);
    if (connectedSpy.count() == 0) connectedSpy.wait();
    QCOMPARE(connectedSpy.count(), 1);
    injectAndWait("JSONRPC.Hello");
}

void TestJSONRPC::testBasicCall_data()
{
    QTest::addColumn<QByteArray>("call");
    QTest::addColumn<bool>("idValid");
    QTest::addColumn<bool>("valid");

    QTest::newRow("valid call 1") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Introspect\"}") << true << true;
    QTest::newRow("valid call 2") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Introspect\"}\n") << true << true;
    QTest::newRow("valid call 3") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Introspect\"}\n\n\n\n") << true << true;
    QTest::newRow("missing id") << QByteArray("{\"method\":\"JSONRPC.Introspect\"}\n") << false << false;
    QTest::newRow("missing method") << QByteArray("{\"id\":42}\n") << true << false;
    QTest::newRow("borked") << QByteArray("{\"id\":42, \"method\":\"JSO}\n") << false << false;
    QTest::newRow("invalid function") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Foobar\", \"token\": \"" + m_apiToken + "\"}\n") << true << false;
    QTest::newRow("invalid namespace") << QByteArray("{\"id\":42, \"method\":\"FOO.Introspect\", \"token\": \"" + m_apiToken + "\"}\n") << true << false;
    QTest::newRow("missing dot") << QByteArray("{\"id\":42, \"method\":\"JSONRPCIntrospect\", \"token\": \"" + m_apiToken + "\"}\n") << true << false;
    QTest::newRow("invalid params") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Introspect\", \"params\":{\"törööö\":\"chooo-chooo\"}}\n") << true << false;
}

void TestJSONRPC::testBasicCall()
{
    QFETCH(QByteArray, call);
    QFETCH(bool, idValid);
    QFETCH(bool, valid);

    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));
    QVERIFY(spy.isValid());

    m_mockTcpServer->injectData(m_clientId, call);

    if (spy.count() == 0) {
        spy.wait();
    }

    // Make sure we got exactly one response
    QVERIFY(spy.count() == 1);

    // Make sure the response goes to the correct clientId
    QCOMPARE(spy.first().first().toString(), m_clientId.toString());

    // Make sure the response it a valid JSON string
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.first().last().toByteArray(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);

    // Make sure the response\"s id is the same as our command
    if (idValid) {
        QCOMPARE(jsonDoc.toVariant().toMap().value("id").toInt(), 42);
    }
    if (valid) {
        QVERIFY2(jsonDoc.toVariant().toMap().value("status").toString() == "success", "Call wasn't parsed correctly by nymea.");
    }
}

void TestJSONRPC::introspect()
{
    QVariant response = injectAndWait("JSONRPC.Introspect");
    QVariantMap methods = response.toMap().value("params").toMap().value("methods").toMap();
    QVariantMap notifications = response.toMap().value("params").toMap().value("notifications").toMap();
    QVariantMap enums = response.toMap().value("params").toMap().value("enums").toMap();
    QVariantMap flags = response.toMap().value("params").toMap().value("flags").toMap();
    QVariantMap types = response.toMap().value("params").toMap().value("types").toMap();

    QVERIFY2(methods.count() > 0, "No methods in Introspect response!");
    QVERIFY2(notifications.count() > 0, "No notifications in Introspect response!");
    QVERIFY2(types.count() > 0, "No types in Introspect response!");

    // Make sure all $ref: pointers have their according type defined
    QVariantMap allItems = methods.unite(notifications).unite(types);
    foreach (const QVariant &item, allItems) {
        foreach (const QString &ref, extractRefs(item)) {
            QString typeId = ref;
            typeId.remove("$ref:");
            QVERIFY2(enums.contains(typeId) || types.contains(typeId) || flags.contains(typeId),
                     QString("Undefined ref: %1. Did you forget to add it to JsonTypes::allTypes()?").arg(ref).toLatin1().data());
            QVERIFY2(!types.value(typeId).toString().startsWith("$ref:")
                     && !flags.value(typeId).toString().startsWith("$ref:")
                     && !enums.value(typeId).toString().startsWith("$ref:"), QString("Definition for %1 must not be a reference itself").arg(ref).toLatin1().data());
        }
    }
}

void TestJSONRPC::enableDisableNotifications_legacy_data()
{
    QTest::addColumn<QString>("enabled");

    QTest::newRow("enabled") << "true";
    QTest::newRow("disabled") << "false";
}

void TestJSONRPC::enableDisableNotifications_legacy()
{
    QFETCH(QString, enabled);

    QVariantMap params;
    params.insert("enabled", enabled);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", params);

    QCOMPARE(response.toMap().value("params").toMap().value("enabled").toString(), enabled);

    qCDebug(dcTests()) << "Enabled notifications:" << response.toMap().value("params").toMap().value("namespaces").toList();

    QStringList expectedNamespaces;
    if (enabled == "true") {
        expectedNamespaces << "Actions" << "NetworkManager" << "Devices" << "Integrations" << "System" << "Rules" << "States" << "Logging" << "Tags" << "AppData" << "JSONRPC" << "Configuration" << "Events" << "Scripts" << "Users" << "Zigbee" << "ModbusRtu";
    }
    std::sort(expectedNamespaces.begin(), expectedNamespaces.end());

    QStringList actualNamespaces;
    foreach (const QVariant &ns, response.toMap().value("params").toMap().value("namespaces").toList()) {
        actualNamespaces << ns.toString();
    }
    std::sort(actualNamespaces.begin(), actualNamespaces.end());

    QCOMPARE(expectedNamespaces, actualNamespaces);
}


void TestJSONRPC::ruleAddedRemovedNotifications()
{
    enableNotifications({"Rules"});

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    // Add rule and wait for notification
    // StateDescriptor
    QVariantMap stateDescriptor;
    stateDescriptor.insert("stateTypeId", mockIntStateTypeId);
    stateDescriptor.insert("thingId", m_mockThingId);
    stateDescriptor.insert("deviceId", m_mockThingId); // DEPRECATED
    stateDescriptor.insert("operator", enumValueName(Types::ValueOperatorLess));
    stateDescriptor.insert("value", "20");
    // This is a bit odd: QUuid.toString() wraps the uuids in {}, however, the implicit cast doesn't
    // .toString(QUuid::WithoutBraces) has only been added in 5.11 so we can't use that either...
    // Only hack I can come up with right now is to convert it to a Json and back to use the implicit cast
    stateDescriptor = QJsonDocument::fromVariant(stateDescriptor).toVariant().toMap();

    QVariantMap stateEvaluator;
    stateEvaluator.insert("stateDescriptor", stateDescriptor);

    // RuleAction
    QVariantMap actionNoParams;
    actionNoParams.insert("actionTypeId", mockWithoutParamsActionTypeId);
    actionNoParams.insert("thingId", m_mockThingId);
    actionNoParams.insert("deviceId", m_mockThingId); // DEPRECATED
    // This is a bit odd: QUuid.toString() wraps the uuids in {}, however, the implicit cast doesn't
    // .toString(QUuid::WithoutBraces) has only been added in 5.11 so we can't use that either...
    // Only hack I can come up with right now is to convert it to a Json and back to use the implicit cast
    QVariantList actions = QVariantList() << QJsonDocument::fromVariant(actionNoParams).toVariant().toMap();

    // EventDescriptor
    QVariantMap eventDescriptor;
    eventDescriptor.insert("eventTypeId", mockEvent1EventTypeId);
    eventDescriptor.insert("thingId", m_mockThingId);
    eventDescriptor.insert("deviceId", m_mockThingId); // DEPRECATED
    // This is a bit odd: QUuid.toString() wraps the uuids in {}, however, the implicit cast doesn't
    // .toString(QUuid::WithoutBraces) has only been added in 5.11 so we can't use that either...
    // Only hack I can come up with right now is to convert it to a Json and back to use the implicit cast
    QVariantList eventDescriptors = QVariantList() << QJsonDocument::fromVariant(eventDescriptor).toVariant().toMap();

    QVariantMap params;
    params.insert("name", "Test Rule notifications");
    params.insert("actions", actions);
    params.insert("eventDescriptors", eventDescriptors);
    params.insert("stateEvaluator", stateEvaluator);

    QVariant response = injectAndWait("Rules.AddRule", params);
    if (clientSpy.count() == 0) clientSpy.wait();
    QVariantMap notificationRuleMap = checkNotification(clientSpy, "Rules.RuleAdded").toMap().value("params").toMap().value("rule").toMap();
    verifyRuleError(response);

    RuleId ruleId = RuleId(response.toMap().value("params").toMap().value("ruleId").toString());
    QVERIFY(!ruleId.isNull());

    QCOMPARE(notificationRuleMap.value("enabled").toBool(), true);
    QCOMPARE(notificationRuleMap.value("name").toString(), params.value("name").toString());
    QCOMPARE(notificationRuleMap.value("id").toUuid(), QUuid(ruleId));
    QVERIFY2(notificationRuleMap.value("actions").toList() == actions,
             QString("actions not matching.\nExpected: %1\nGot %2")
             .arg(QString(QJsonDocument::fromVariant(actions).toJson()))
             .arg(QString(QJsonDocument::fromVariant(notificationRuleMap.value("actions").toList()).toJson()))
             .toUtf8());
    QVERIFY2(notificationRuleMap.value("stateEvaluator").toMap().value("stateDescriptor").toMap() == stateDescriptor,
             QString("stateDescriptor not matching.\nExpected: %1\nGot %2")
             .arg(QString(QJsonDocument::fromVariant(stateDescriptor).toJson()))
             .arg(QString(QJsonDocument::fromVariant(notificationRuleMap.value("stateEvaluator").toMap().value("stateDescriptor").toMap()).toJson()))
             .toUtf8());
    QVERIFY2(notificationRuleMap.value("eventDescriptors").toList() == eventDescriptors,
             QString("eventDescriptors not matching.\nExpected: %1\nGot %2")
             .arg(QString(QJsonDocument::fromVariant(eventDescriptors).toJson()))
             .arg(QString(QJsonDocument::fromVariant(notificationRuleMap.value("eventDescriptors").toList()).toJson()))
             .toUtf8());
    QCOMPARE(notificationRuleMap.value("exitActions").toList(), QVariantList());

    // now remove the rule and check the RuleRemoved notification
    params.clear(); response.clear(); clientSpy.clear();
    params.insert("ruleId", ruleId);
    response = injectAndWait("Rules.RemoveRule", params);
    if (clientSpy.count() == 0) clientSpy.wait();
    checkNotification(clientSpy, "Devices.DeviceRemoved");
    verifyRuleError(response);

    QCOMPARE(disableNotifications(), true);
}

void TestJSONRPC::ruleActiveChangedNotifications()
{
    // enable notificartions
    QVariantMap params;
    params.insert("enabled", true);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", params);
    QCOMPARE(response.toMap().value("params").toMap().value("enabled").toBool(), true);

    // Add rule and wait for notification
    // StateDescriptor
    QVariantMap stateDescriptor;
    stateDescriptor.insert("stateTypeId", mockIntStateTypeId);
    stateDescriptor.insert("thingId", m_mockThingId);
    stateDescriptor.insert("deviceId", m_mockThingId); // DEPRECATED
    stateDescriptor.insert("operator", enumValueName(Types::ValueOperatorEquals));
    stateDescriptor.insert("value", "20");
    // This is a bit odd: QUuid.toString() wraps the uuids in {}, however, the implicit cast doesn't
    // .toString(QUuid::WithoutBraces) has only been added in 5.11 so we can't use that either...
    // Only hack I can come up with right now is to convert it to a Json and back to use the implicit cast
    stateDescriptor = QJsonDocument::fromVariant(stateDescriptor).toVariant().toMap();

    QVariantMap stateEvaluator;
    stateEvaluator.insert("stateDescriptor", stateDescriptor);

    // RuleAction
    QVariantMap actionNoParams;
    actionNoParams.insert("actionTypeId", mockWithoutParamsActionTypeId);
    actionNoParams.insert("thingId", m_mockThingId);
    actionNoParams.insert("deviceId", m_mockThingId); // DEPRECATED
    // This is a bit odd: QUuid.toString() wraps the uuids in {}, however, the implicit cast doesn't
    // .toString(QUuid::WithoutBraces) has only been added in 5.11 so we can't use that either...
    // Only hack I can come up with right now is to convert it to a Json and back to use the implicit cast
    QVariantList actions = QVariantList() << QJsonDocument::fromVariant(actionNoParams).toVariant().toMap();

    params.clear(); response.clear();
    params.insert("name", "Test Rule notifications");
    params.insert("actions", actions);
    params.insert("stateEvaluator", stateEvaluator);

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    response = injectAndWait("Rules.AddRule", params);
    if (clientSpy.count() == 0) clientSpy.wait();
    QVariant notificationVariant = checkNotification(clientSpy, "Rules.RuleAdded");
    verifyRuleError(response);

    QVariantMap notificationRuleMap = notificationVariant.toMap().value("params").toMap().value("rule").toMap();
    RuleId ruleId = RuleId(notificationRuleMap.value("id").toString());
    QVERIFY(!ruleId.isNull());
    QCOMPARE(notificationRuleMap.value("enabled").toBool(), true);
    QCOMPARE(notificationRuleMap.value("name").toString(), params.value("name").toString());
    QCOMPARE(notificationRuleMap.value("id").toUuid(), QUuid(ruleId));
    QCOMPARE(notificationRuleMap.value("actions").toList(), actions);
    QCOMPARE(notificationRuleMap.value("stateEvaluator").toMap().value("stateDescriptor").toMap(), stateDescriptor);
    QCOMPARE(notificationRuleMap.value("exitActions").toList(), QVariantList());

    // set the rule active
    QNetworkAccessManager nam;
    QSignalSpy spy(&nam, SIGNAL(finished(QNetworkReply*)));

    // state state to 20
    qDebug() << "setting mock int state to 20";
    QNetworkRequest request(QUrl(QString("http://localhost:%1/setstate?%2=%3").arg(m_mockThing1Port).arg(mockIntStateTypeId.toString()).arg(20)));
    QNetworkReply *reply = nam.get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    if (spy.count() == 0) spy.wait();
    notificationVariant = checkNotification(clientSpy, "Rules.RuleActiveChanged");
    verifyRuleError(response);

    QCOMPARE(notificationVariant.toMap().value("params").toMap().value("ruleId").toUuid().toString(), ruleId.toString());
    QCOMPARE(notificationVariant.toMap().value("params").toMap().value("active").toBool(), true);

    clientSpy.wait();

    spy.clear(); clientSpy.clear();

    // set the rule inactive
    qDebug() << "setting mock int state to 42";
    QNetworkRequest request2(QUrl(QString("http://localhost:%1/setstate?%2=%3").arg(m_mockThing1Port).arg(mockIntStateTypeId.toString()).arg(42)));
    QNetworkReply *reply2 = nam.get(request2);
    connect(reply2, SIGNAL(finished()), reply2, SLOT(deleteLater()));

    // Waiting for notifications:
    // Devices.StateChanged for the change we did
    // Devices.EventTriggered
    // Events.EventTriggered <-- deprecated
    // Rules.RuleActiveChanged
    // Logging.LogEntryAdded
    // Devices.StateChanged for the change done by the rule
    // Devices.EventTriggered
    // Events.EventTriggered <-- deprecated
    while (clientSpy.count() < 8) {
        clientSpy.wait();
    }

    notificationVariant = checkNotification(clientSpy, "Rules.RuleActiveChanged");
    verifyRuleError(response);

    QCOMPARE(notificationVariant.toMap().value("params").toMap().value("ruleId").toUuid().toString(), ruleId.toString());
    QCOMPARE(notificationVariant.toMap().value("params").toMap().value("active").toBool(), false);

    // now remove the rule and check the RuleRemoved notification
    params.clear(); response.clear();
    params.insert("ruleId", ruleId);
    response = injectAndWait("Rules.RemoveRule", params);

    if (clientSpy.count() == 0) clientSpy.wait();
    notificationVariant = checkNotification(clientSpy, "Rules.RuleRemoved");
    checkNotification(clientSpy, "Logging.LogDatabaseUpdated");
    verifyRuleError(response);

    QCOMPARE(notificationVariant.toMap().value("params").toMap().value("ruleId").toUuid().toString(), ruleId.toString());
}

void TestJSONRPC::stateChangeEmitsNotifications()
{
    enableNotifications({"Devices", "States", "Logging", "Events"});
    bool found = false;

    // Setup connection to mock client
    QNetworkAccessManager nam;
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    // trigger state change in mock device
    int newVal = 38;
    QUuid stateTypeId("80baec19-54de-4948-ac46-31eabfaceb83");
    QNetworkRequest request(QUrl(QString("http://localhost:%1/setstate?%2=%3").arg(m_mockThing1Port).arg(stateTypeId.toString()).arg(QString::number(newVal))));
    QNetworkReply *reply = nam.get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    QSignalSpy replySpy(reply, SIGNAL(finished()));
    if (replySpy.count() == 0) replySpy.wait();

    // Make sure the notification contains all the stuff we expect
    QVariantList stateChangedVariants = checkNotifications(clientSpy, "Devices.StateChanged");
    QVERIFY2(!stateChangedVariants.isEmpty(), "Did not get Devices.StateChanged notification.");
    qDebug() << "got" << stateChangedVariants.count() << "Devices.StateChanged notifications";
    foreach (const QVariant &stateChangedVariant, stateChangedVariants) {
        if (stateChangedVariant.toMap().value("params").toMap().value("stateTypeId").toUuid() == stateTypeId) {
            found = true;
            QCOMPARE(stateChangedVariant.toMap().value("params").toMap().value("value").toInt(), newVal);
            break;
        }
    }

    QVERIFY2(found, "Could not find the correct Devices.StateChanged notification");

    // Waiting for:
    // Devices.StateChanged
    // Devices.EventTriggered
    // Events.EventTriggered <-- deprecated
    while (clientSpy.count() < 3) {
        clientSpy.wait();
    }

    // Make sure the notification contains all the stuff we expect
    QVariantList eventTriggeredVariants = checkNotifications(clientSpy, "Events.EventTriggered");
    QVERIFY2(!eventTriggeredVariants.isEmpty(), "Did not get Events.EventTriggered notification.");
    found = false;
    foreach (const QVariant &eventTriggeredVariant, eventTriggeredVariants) {
        if (eventTriggeredVariant.toMap().value("params").toMap().value("event").toMap().value("eventTypeId").toUuid() == stateTypeId) {
            found = true;
            QCOMPARE(eventTriggeredVariant.toMap().value("params").toMap().value("event").toMap().value("params").toList().first().toMap().value("value").toInt(), newVal);
            break;
        }
    }

    QVERIFY2(found, "Could not find the corresponding Events.EventTriggered notification");

    // Now turn off notifications
    QCOMPARE(disableNotifications(), true);

    // Fire the a statechange once again
    clientSpy.clear();
    newVal = 42;
    request.setUrl(QUrl(QString("http://localhost:%1/setstate?%2=%3").arg(m_mockThing1Port).arg(stateTypeId.toString()).arg(newVal)));
    reply = nam.get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    // Lets wait a max of 500ms for notifications
    clientSpy.wait(500);
    // but make sure it doesn't come
    QCOMPARE(clientSpy.count(), 0);

    // Now check that the state has indeed changed even though we didn't get a notification
    QVariantMap params;
    params.insert("thingId", m_mockThingId);
    params.insert("stateTypeId", stateTypeId);
    QVariant response = injectAndWait("Integrations.GetStateValue", params);

    QCOMPARE(response.toMap().value("params").toMap().value("value").toInt(), newVal);
}

void TestJSONRPC::pluginConfigChangeEmitsNotification()
{
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    QVariantMap params;
    params.insert("enabled", true);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", params);
    QCOMPARE(response.toMap().value("params").toMap().value("enabled").toBool(), true);

    params.clear();
    params.insert("pluginId", mockPluginId);
    QVariantList pluginParams;
    QVariantMap param1;
    param1.insert("paramTypeId",  mockPluginConfigParamIntParamTypeId);
    param1.insert("value", 42);
    pluginParams.append(param1);
    params.insert("configuration", pluginParams);

    response = injectAndWait("Integrations.SetPluginConfiguration", params);

    QVariantList notificationData = checkNotifications(clientSpy, "Integrations.PluginConfigurationChanged");
    QCOMPARE(notificationData.first().toMap().value("notification").toString() == "Integrations.PluginConfigurationChanged", true);
}

void TestJSONRPC::testPushButtonAuth()
{
    PushButtonAgent pushButtonAgent;
    pushButtonAgent.init(QDBusConnection::SessionBus);

    QVariantMap params;
    params.insert("deviceName", "pbtestdevice");
    QVariant response = injectAndWait("JSONRPC.RequestPushButtonAuth", params);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId = response.toMap().value("params").toMap().value("transactionId").toInt();

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    pushButtonAgent.sendButtonPressed();

    if (clientSpy.count() == 0) clientSpy.wait();
    QVariantMap rsp = checkNotification(clientSpy, "JSONRPC.PushButtonAuthFinished").toMap();

    QCOMPARE(rsp.value("params").toMap().value("transactionId").toInt(), transactionId);
    QVERIFY2(!rsp.value("params").toMap().value("token").toByteArray().isEmpty(), "Token not in push button auth notification");
}



void TestJSONRPC::testPushButtonAuthInterrupt()
{
    enableNotifications({});
    PushButtonAgent pushButtonAgent;
    pushButtonAgent.init(QDBusConnection::SessionBus);

    // m_clientId is registered in gutTestbase already, just using it here to improve readability of the test
    QUuid aliceId = m_clientId;

    // Create a new clientId for mallory and connect it to the server
    QUuid malloryId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(malloryId);
    QSignalSpy responseSpy(m_mockTcpServer, &MockTcpServer::outgoingData);
    m_mockTcpServer->injectData(malloryId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (responseSpy.count() == 0) responseSpy.wait();

    // Snoop in on everything the TCP server sends to its clients.
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    // request push button auth for client 1 (alice) and check for OK reply
    QVariantMap params;
    params.insert("deviceName", "alice");
    QVariant response = injectAndWait("JSONRPC.RequestPushButtonAuth", params, aliceId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId1 = response.toMap().value("params").toMap().value("transactionId").toInt();


    // Request push button auth for client 2 (mallory)
    clientSpy.clear();
    params.clear();
    params.insert("deviceName", "mallory");
    response = injectAndWait("JSONRPC.RequestPushButtonAuth", params, malloryId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId2 = response.toMap().value("params").toMap().value("transactionId").toInt();

    // Both clients should receive something. Wait for it
    if (clientSpy.count() < 2) {
        clientSpy.wait();
    }

    // spy.at(0) should be the failed notification for alice
    // spy.at(1) shpuld be the OK reply for mallory


    // alice should have received a failed notification. She knows something's wrong.
    QVariantMap notification = QJsonDocument::fromJson(clientSpy.first().at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(clientSpy.first().first().toUuid(), aliceId);
    QCOMPARE(notification.value("notification").toString(), QLatin1String("JSONRPC.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId1);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), false);

    // Mallory instead should have received an OK
    QVariantMap reply = QJsonDocument::fromJson(clientSpy.at(1).at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(clientSpy.at(1).first().toUuid(), malloryId);
    QCOMPARE(reply.value("params").toMap().value("success").toBool(), true);


    // Alice tries once more
    clientSpy.clear();
    params.clear();
    params.insert("deviceName", "alice");
    response = injectAndWait("JSONRPC.RequestPushButtonAuth", params, aliceId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId3 = response.toMap().value("params").toMap().value("transactionId").toInt();

    // Both clients should receive something. Wait for it
    if (clientSpy.count() < 2) {
        clientSpy.wait();
    }

    // spy.at(0) should be the failed notification for mallory
    // spy.at(1) shpuld be the OK reply for alice

    // mallory should have received a failed notification. She knows something's wrong.
    notification = QJsonDocument::fromJson(clientSpy.first().at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(clientSpy.first().first().toUuid(), malloryId);
    QCOMPARE(notification.value("notification").toString(), QLatin1String("JSONRPC.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId2);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), false);

    // Alice instead should have received an OK
    reply = QJsonDocument::fromJson(clientSpy.at(1).at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(clientSpy.at(1).first().toUuid(), aliceId);
    QCOMPARE(reply.value("params").toMap().value("success").toBool(), true);

    clientSpy.clear();

    // do the button press
    pushButtonAgent.sendButtonPressed();

    // Wait for things to happen
    if (clientSpy.count() == 0) {
        clientSpy.wait();
    }

    // There should have been only exactly one message sent, the token for alice
    // Mallory should not have received anything
    QCOMPARE(clientSpy.count(), 1);
    notification = QJsonDocument::fromJson(clientSpy.first().at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(clientSpy.first().first().toUuid(), aliceId);
    QCOMPARE(notification.value("notification").toString(), QLatin1String("JSONRPC.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId3);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), true);
    QVERIFY2(!notification.value("params").toMap().value("token").toByteArray().isEmpty(), "Token is empty while it shouldn't be");
}

void TestJSONRPC::testPushButtonAuthConnectionDrop()
{
    PushButtonAgent pushButtonAgent;
    pushButtonAgent.init(QDBusConnection::SessionBus);

    // Snoop in on everything the TCP server sends to its clients.
    QSignalSpy clientSpy(m_mockTcpServer, &MockTcpServer::outgoingData);

    // Create a new clientId for alice and connect it to the server
    QUuid aliceId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(aliceId);
    m_mockTcpServer->injectData(aliceId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (clientSpy.count() == 0) clientSpy.wait();

    // request push button auth for client 1 (alice) and check for OK reply
    QVariantMap params;
    params.insert("deviceName", "alice");
    QVariant response = injectAndWait("JSONRPC.RequestPushButtonAuth", params, aliceId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);

    // Disconnect alice
    m_mockTcpServer->clientDisconnected(aliceId);

    // Now try with bob
    // Create a new clientId for bob and connect it to the server
    QUuid bobId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(bobId);
    clientSpy.clear();
    m_mockTcpServer->injectData(bobId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (clientSpy.count() == 0) clientSpy.wait();

    // request push button auth for client 2 (bob) and check for OK reply
    params.clear();
    params.insert("deviceName", "bob");
    response = injectAndWait("JSONRPC.RequestPushButtonAuth", params, bobId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId = response.toMap().value("params").toMap().value("transactionId").toInt();

    clientSpy.clear();

    pushButtonAgent.sendButtonPressed();

    // Wait for things to happen
    if (clientSpy.count() == 0) {
        clientSpy.wait();
    }

    // There should have been only exactly one message sent, the token for bob
    QCOMPARE(clientSpy.count(), 1);
    QVariantMap notification = QJsonDocument::fromJson(clientSpy.first().at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(clientSpy.first().first().toUuid(), bobId);
    QCOMPARE(notification.value("notification").toString(), QLatin1String("JSONRPC.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), true);
    QVERIFY2(!notification.value("params").toMap().value("token").toByteArray().isEmpty(), "Token is empty while it shouldn't be");

}

void TestJSONRPC::testInitialSetupWithPushButtonAuth()
{
    foreach (const UserInfo &userInfo, NymeaCore::instance()->userManager()->users()) {
        NymeaCore::instance()->userManager()->removeUser(userInfo.username());
    }
    NymeaCore::instance()->userManager()->removeUser("");
    QVERIFY(NymeaCore::instance()->userManager()->initRequired());

    QSignalSpy spy(m_mockTcpServer, &MockTcpServer::outgoingData);
    QVERIFY(spy.isValid());

    PushButtonAgent pushButtonAgent;
    pushButtonAgent.init(QDBusConnection::SessionBus);

    // Hello call should work in any case, telling us initial setup is required
    spy.clear();
    qCDebug(dcTests()) << "Calling Hello on uninitialized instance";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Hello\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    QVariant response = jsonDoc.toVariant();
    qCDebug(dcTests()) << "Result:" << response.toMap().value("status").toString() << response.toMap().value("error").toString();
    QCOMPARE(response.toMap().value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.toMap().value("params").toMap().value("initialSetupRequired").toBool(), true);

    // request push button auth for alice and check for OK reply
    QUuid aliceId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(aliceId);
    spy.clear();
    m_mockTcpServer->injectData(aliceId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (spy.count() == 0) spy.wait();

    QVariantMap params;
    params.insert("deviceName", "alice");
    response = injectAndWait("JSONRPC.RequestPushButtonAuth", params, aliceId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId = response.toMap().value("params").toMap().value("transactionId").toInt();

    spy.clear();
    pushButtonAgent.sendButtonPressed();

    // Wait for things to happen
    if (spy.count() == 0) {
        spy.wait();
    }

    // There should have been only exactly one message sent, the token for alice
    QCOMPARE(spy.count(), 1);
    QVariantMap notification = QJsonDocument::fromJson(spy.first().at(1).toByteArray()).toVariant().toMap();
    QCOMPARE(spy.first().first().toUuid(), aliceId);
    QCOMPARE(notification.value("notification").toString(), QLatin1String("JSONRPC.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), true);
    QVERIFY2(!notification.value("params").toMap().value("token").toByteArray().isEmpty(), "Token is empty while it shouldn't be");

    // initialSetupRequired should be false in Hello call now
    spy.clear();
    qCDebug(dcTests()) << "Calling Hello on uninitialized instance";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.Hello\"}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant();
    qCDebug(dcTests()) << "Result:" << response.toMap().value("status").toString() << response.toMap().value("error").toString();
    QCOMPARE(response.toMap().value("status").toString(), QStringLiteral("success"));
    QCOMPARE(response.toMap().value("params").toMap().value("initialSetupRequired").toBool(), false);


    // CreateUser without a token should fail now even though there are 0 users in the DB
    spy.clear();
    QSignalSpy disconnectedSpy(m_mockTcpServer, &MockTcpServer::clientDisconnected);
    qCDebug(dcTests()) << "Calling CreateUser on uninitialized instance";
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 555, \"method\": \"JSONRPC.CreateUser\", \"params\": {\"username\": \"Dummy@guh.io\", \"password\": \"DummyPW1!\"}}\n");
    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY(spy.count() == 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    response = jsonDoc.toVariant();
    qCDebug(dcTests()) << "Result:" << response.toMap().value("status").toString() << response.toMap().value("error").toString();
    QCOMPARE(response.toMap().value("status").toString(), QStringLiteral("unauthorized"));
    QCOMPARE(NymeaCore::instance()->userManager()->users().count(), 0);

    // Connection should drop
    if (disconnectedSpy.isEmpty()) disconnectedSpy.wait();
    QCOMPARE(disconnectedSpy.count(), 1);

    // Reconnect to not impact subsequent tests
    m_mockTcpServer->clientConnected(m_clientId);
    spy.clear();
    m_mockTcpServer->injectData(m_clientId, "{\"id\": 0, \"method\": \"JSONRPC.Hello\"}");
    if (spy.isEmpty()) spy.wait();
}

void TestJSONRPC::testDataFragmentation_data()
{
    QTest::addColumn<QList<QByteArray> >("packets");

    QList<QByteArray> packets;

    packets.append("{\"id\": 555, \"method\": \"JSONRPC.Hello\"}\n");
    QTest::newRow("1 packet") << packets;

    packets.clear();
    packets.append("{\"id\": 555, \"m");
    packets.append("ethod\": \"JSONRPC.Hello\"}\n");
    QTest::newRow("2 packets") << packets;

    packets.clear();
    packets.append("{\"id\": 555, \"m");
    packets.append("ethod\": \"JSONRP");
    packets.append("C.Hello\"}\n");
    QTest::newRow("3 packets") << packets;

    packets.clear();
    packets.append("{\"id\": 555, \"method\": \"JSONRPC.Hello\"}\n{\"id\": 5556, \"metho");
    QTest::newRow("next packet start appended") << packets;
}

void TestJSONRPC::testDataFragmentation()
{
    QJsonDocument jsonDoc;
    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    QFETCH(QList<QByteArray>, packets);

    foreach (const QByteArray &packet, packets) {
        spy.clear();
        m_mockTcpServer->injectData(m_clientId, packet);
    }
    if (spy.count() == 0) {
        spy.wait();
    }
    QCOMPARE(spy.count(), 1);
    jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    QCOMPARE(jsonDoc.toVariant().toMap().value("status").toString(), QStringLiteral("success"));
}

void TestJSONRPC::testGarbageData()
{
    QSignalSpy spy(m_mockTcpServer, &MockTcpServer::connectionTerminated);

    QByteArray data;
    for (int i = 0; i < 1024 * 1024; i++) {
        data.append("a");
    }
    for (int i = 0; i < 11 && spy.count() == 0; i ++) {
        m_mockTcpServer->injectData(m_clientId, data);
    }
    QCOMPARE(spy.count(), 1);
}

#include "testjsonrpc.moc"

QTEST_MAIN(TestJSONRPC)
