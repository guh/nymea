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

#include <QtTest>

#include "logging/logengine.h"
#include "nymeacore.h"
#include "nymeatestbase.h"
#include "usermanager/usermanager.h"
#include "servers/mocktcpserver.h"
#include "nymeadbusservice.h"

#include "../../utils/pushbuttonagent.h"

using namespace nymeaserver;

class TestUsermanager: public NymeaTestBase
{
    Q_OBJECT
public:
    TestUsermanager(QObject* parent = nullptr);

private slots:
    void initTestCase();

    void init();

    void loginValidation_data();
    void loginValidation();

    void createUser();

    void authenticate();

    /*
    Cases for push button auth:

    Case 1: regular pushbutton
    - alice sends Users.RequestPushButtonAuth, gets "OK" back (if push button hardware is available)
    - alice pushes the hardware button and gets a notification on jsonrpc containing the token for local auth
    */
    void authenticatePushButton();

    /*
    Case 2: if we have an attacker in the network, he could try to call requestPushButtonAuth and
    hope someone would eventually press the button and give him a token. In order to prevent this,
    any previous attempt for a push button auth needs to be cancelled when a new request comes in:

    * Mallory does RequestPushButtonAuth, gets OK back
    * Alice does RequestPushButtonAuth,
    * Mallory receives a "PushButtonFailed" notification
    * Alice receives OK
    * Alice presses the hardware button
    * Alice reveices a notification with token, mallory receives nothing

    Case 3: Mallory tries to hijack it back again

    * Mallory does RequestPushButtonAuth, gets OK back
    * Alice does RequestPusButtonAuth,
    * Alice gets ok reply, Mallory gets failed notification
    * Mallory quickly does RequestPushButtonAuth again to win the fight
    * Alice gets failed notification and can instruct the user to _not_ press the button now until procedure is restarted
    */
    void authenticatePushButtonAuthInterrupt();

    void authenticatePushButtonAuthConnectionDrop();

    void createDuplicateUser();

    void getTokens();

    void removeToken();

    void unauthenticatedCallAfterTokenRemove();

    void changePassword();

    void authenticateAfterPasswordChangeOK();

    void authenticateAfterPasswordChangeFail();

    void getUserInfo();

private:
    // m_apiToken is in testBase
    QUuid m_tokenId;
};

TestUsermanager::TestUsermanager(QObject *parent): NymeaTestBase(parent)
{
    QCoreApplication::instance()->setOrganizationName("nymea-test");
}

void TestUsermanager::initTestCase()
{
    NymeaDBusService::setBusType(QDBusConnection::SessionBus);
    NymeaTestBase::initTestCase();

    QLoggingCategory::setFilterRules("*.debug=false\n"
                                     "Application.debug=true\n"
                                     "Tests.debug=true\n"
                                     "UserManager.debug=true\n"
                                     "PushButtonAgent.debug=true\n"
                                     "MockDevice.debug=true");
}

void TestUsermanager::init()
{
    UserManager *userManager = NymeaCore::instance()->userManager();
    foreach (const UserInfo &userInfo, userManager->users()) {
        qCDebug(dcTests()) << "Removing user" << userInfo.username();
        userManager->removeUser(userInfo.username());
    }
    userManager->removeUser("");

}

void TestUsermanager::loginValidation_data() {
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("password");
    QTest::addColumn<UserManager::UserError>("expectedError");

    QTest::newRow("foo@bar.baz,    Bla1234*, NoError")       << "foo@bar.baz"    << "Bla1234*" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.co.uk,  Bla1234*, NoError")       << "foo@bar.co.uk"  << "Bla1234*" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.com.au, Bla1234*, NoError")       << "foo@bar.com.au" << "Bla1234*" << UserManager::UserErrorNoError;

    QTest::newRow("foo,         Bla1234*, InvalidUserId") << "foo"         << "Bla1234*" << UserManager::UserErrorInvalidUserId;
    QTest::newRow("@,           Bla1234*, InvalidUserId") << "@"           << "Bla1234*" << UserManager::UserErrorInvalidUserId;
    QTest::newRow("foo@,        Bla1234*, InvalidUserId") << "foo@"        << "Bla1234*" << UserManager::UserErrorInvalidUserId;
    QTest::newRow("foo@bar,     Bla1234*, InvalidUserId") << "foo@bar"     << "Bla1234*" << UserManager::UserErrorInvalidUserId;
    QTest::newRow("foo@bar.,    Bla1234*, InvalidUserId") << "foo@bar."    << "Bla1234*" << UserManager::UserErrorInvalidUserId;
    QTest::newRow("foo@bar.co.uk.au, Bla1234*, InvalidUserId") << "foo@bar.co.uk.au"    << "Bla1234*" << UserManager::UserErrorInvalidUserId;

    QTest::newRow("foo@bar.baz, a,        BadPassword") << "foo@bar.baz" << "a"        << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, a1,       BadPassword") << "foo@bar.baz" << "a1"       << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, a1!,      BadPassword") << "foo@bar.baz" << "a1!"      << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, aaaaaaaa, BadPassword") << "foo@bar.baz" << "aaaaaaaa" << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, aaaaaaa1, BadPassword") << "foo@bar.baz" << "aaaaaaa1" << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, aaaaaaa!, BadPassword") << "foo@bar.baz" << "aaaaaaa!" << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, aaaaaaaA, BadPassword") << "foo@bar.baz" << "aaaaaaaA" << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, aaaaaa!A, BadPassword") << "foo@bar.baz" << "aaaaaa!A" << UserManager::UserErrorBadPassword;
    QTest::newRow("foo@bar.baz, aaaaaa!1, BadPassword") << "foo@bar.baz" << "aaaaaa!1" << UserManager::UserErrorBadPassword;

    QTest::newRow("foo@bar.baz, aaaaa!1A, NoError")            << "foo@bar.baz" << "aaaaa!1A" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.baz, Bla1234*a, NoError")           << "foo@bar.baz" << "Bla1234*a" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.baz, #1-Nymea-is-awesome, NoError") << "foo@bar.baz" << "#1-Nymea-is-awesome" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.baz, Bla1234.a, NoError")           << "foo@bar.baz" << "Bla1234.a" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.baz, Bla1234\\a, NoError")          << "foo@bar.baz" << "Bla1234\\a" << UserManager::UserErrorNoError;
    QTest::newRow("foo@bar.baz, Bla1234@a, NoError")           << "foo@bar.baz" << "Bla1234@a" << UserManager::UserErrorNoError;

}

void TestUsermanager::loginValidation()
{
    QFETCH(QString, username);
    QFETCH(QString, password);
    QFETCH(UserManager::UserError, expectedError);

    UserManager *userManager = NymeaCore::instance()->userManager();
    UserManager::UserError error = userManager->createUser(username, password, Types::PermissionScopeAdmin);
    qDebug() << "Error:" << error << "Expected:" << expectedError;
    QCOMPARE(error, expectedError);

}

void TestUsermanager::createUser()
{
    QVariantMap params;
    params.insert("username", "valid@user.test");
    params.insert("password", "Bla1234*");
    QVariant response = injectAndWait("Users.CreateUser", params);

    QVERIFY2(response.toMap().value("status").toString() == "success", "Error creating user");
    QVERIFY2(response.toMap().value("params").toMap().value("error").toString() == "UserErrorNoError", "Error creating user");
}

void TestUsermanager::authenticate()
{
    m_apiToken.clear();
    createUser();

    QVariantMap params;
    params.insert("username", "valid@user.test");
    params.insert("password", "Bla1234*");
    params.insert("deviceName", "autotests");
    QVariant response = injectAndWait("Users.Authenticate", params);

    m_apiToken = response.toMap().value("params").toMap().value("token").toByteArray();

    QVERIFY2(response.toMap().value("status").toString() == "success", "Error authenticating");
    QVERIFY2(response.toMap().value("params").toMap().value("success").toString() == "true", "Error authenticating");
}

void TestUsermanager::authenticatePushButton()
{
    PushButtonAgent pushButtonAgent;
    pushButtonAgent.init(QDBusConnection::SessionBus);

    QVariantMap params;
    params.insert("deviceName", "pbtestdevice");
    QVariant response = injectAndWait("Users.RequestPushButtonAuth", params);
    qCDebug(dcTests()) << "Pushbutton auth response:" << qUtf8Printable(QJsonDocument::fromVariant(response).toJson(QJsonDocument::Indented));
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId = response.toMap().value("params").toMap().value("transactionId").toInt();

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    pushButtonAgent.sendButtonPressed();

    if (clientSpy.count() == 0) clientSpy.wait();
    QVariantMap rsp = checkNotification(clientSpy, "Users.PushButtonAuthFinished").toMap();

    for (int i = 0; i < clientSpy.count(); i++) {
        qCDebug(dcTests()) << "Notification:" << clientSpy.at(i);
    }

    QCOMPARE(rsp.value("params").toMap().value("transactionId").toInt(), transactionId);
    QVERIFY2(!rsp.value("params").toMap().value("token").toByteArray().isEmpty(), "Token not in push button auth notification");
}

void TestUsermanager::authenticatePushButtonAuthInterrupt()
{
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
    QVariant response = injectAndWait("Users.RequestPushButtonAuth", params, aliceId);
    QCOMPARE(response.toMap().value("params").toMap().value("success").toBool(), true);
    int transactionId1 = response.toMap().value("params").toMap().value("transactionId").toInt();


    // Request push button auth for client 2 (mallory)
    clientSpy.clear();
    params.clear();
    params.insert("deviceName", "mallory");
    response = injectAndWait("Users.RequestPushButtonAuth", params, malloryId);
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
    QCOMPARE(notification.value("notification").toString(), QLatin1String("Users.PushButtonAuthFinished"));
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
    response = injectAndWait("Users.RequestPushButtonAuth", params, aliceId);
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
    QCOMPARE(notification.value("notification").toString(), QLatin1String("Users.PushButtonAuthFinished"));
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
    QCOMPARE(notification.value("notification").toString(), QLatin1String("Users.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId3);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), true);
    QVERIFY2(!notification.value("params").toMap().value("token").toByteArray().isEmpty(), "Token is empty while it shouldn't be");
}

void TestUsermanager::authenticatePushButtonAuthConnectionDrop()
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
    QVariant response = injectAndWait("Users.RequestPushButtonAuth", params, aliceId);
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
    response = injectAndWait("Users.RequestPushButtonAuth", params, bobId);
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
    QCOMPARE(notification.value("notification").toString(), QLatin1String("Users.PushButtonAuthFinished"));
    QCOMPARE(notification.value("params").toMap().value("transactionId").toInt(), transactionId);
    QCOMPARE(notification.value("params").toMap().value("success").toBool(), true);
    QVERIFY2(!notification.value("params").toMap().value("token").toByteArray().isEmpty(), "Token is empty while it shouldn't be");

}

void TestUsermanager::createDuplicateUser()
{
    authenticate();

    QVariantMap params;
    params.insert("username", "valid@user.test");
    params.insert("password", "Bla1234*");
    QVariant response = injectAndWait("Users.CreateUser", params);

    QVERIFY2(response.toMap().value("status").toString() == "success", "Unexpected error code creating duplicate user");
    QVERIFY2(response.toMap().value("params").toMap().value("error").toString() == "UserErrorDuplicateUserId", "Unexpected error creating duplicate user");
}

void TestUsermanager::getTokens()
{
    authenticate();

    QVariant response = injectAndWait("Users.GetTokens");
    QVERIFY2(response.toMap().value("status").toString() == "success", "Unexpected error code creating duplicate user");
    QCOMPARE(response.toMap().value("params").toMap().value("error").toString(), QString("UserErrorNoError"));

    QVariantList tokenInfoList = response.toMap().value("params").toMap().value("tokenInfoList").toList();
    QCOMPARE(tokenInfoList.count(), 1);

    m_tokenId = tokenInfoList.first().toMap().value("id").toUuid();
    QVERIFY2(!m_tokenId.isNull(), "Token ID should not be null");
    QCOMPARE(tokenInfoList.first().toMap().value("username").toString(), QString("valid@user.test"));
    QCOMPARE(tokenInfoList.first().toMap().value("deviceName").toString(), QString("autotests"));
}

void TestUsermanager::removeToken()
{
    getTokens();

    QVariantMap params;
    params.insert("tokenId", m_tokenId);
    QVariant response = injectAndWait("Users.RemoveToken", params);
    QCOMPARE(response.toMap().value("status").toString(), QString("success"));
    QCOMPARE(response.toMap().value("params").toMap().value("error").toString(), QString("UserErrorNoError"));
}

void TestUsermanager::changePassword()
{
    authenticate();

    QVariantMap params;
    params.insert("newPassword", "Blubb123");
    QVariant response = injectAndWait("Users.ChangePassword", params);
    QCOMPARE(response.toMap().value("status").toString(), QString("success"));
    QCOMPARE(response.toMap().value("params").toMap().value("error").toString(), QString("UserErrorNoError"));
}

void TestUsermanager::authenticateAfterPasswordChangeOK()
{
    changePassword();

    QVariantMap params;
    params.insert("username", "valid@user.test");
    params.insert("password", "Blubb123"); // New password, should be ok
    params.insert("deviceName", "autotests");
    QVariant response = injectAndWait("Users.Authenticate", params);

    m_apiToken = response.toMap().value("params").toMap().value("token").toByteArray();
    QVERIFY2(!m_apiToken.isEmpty(), "Token should not be empty");
    QVERIFY2(response.toMap().value("status").toString() == "success", "Error authenticating");
    QVERIFY2(response.toMap().value("params").toMap().value("success").toString() == "true", "Error authenticating");
}

void TestUsermanager::authenticateAfterPasswordChangeFail()
{
    changePassword();

    QVariantMap params;
    params.insert("username", "valid@user.test");
    params.insert("password", "Bla1234*"); // Original password, should not be ok
    params.insert("deviceName", "autotests");
    QVariant response = injectAndWait("Users.Authenticate", params);

    m_apiToken = response.toMap().value("params").toMap().value("token").toByteArray();
    QVERIFY2(m_apiToken.isEmpty(), "Token should be empty");
    QVERIFY2(response.toMap().value("status").toString() == "success", "Error authenticating");
    QCOMPARE(response.toMap().value("params").toMap().value("success").toString(), QString("false"));
}

void TestUsermanager::getUserInfo()
{
    authenticate();

    QVariant response = injectAndWait("Users.GetUserInfo");

    QCOMPARE(response.toMap().value("status").toString(), QString("success"));

    QVariantMap userInfoMap = response.toMap().value("params").toMap().value("userInfo").toMap();


    QCOMPARE(userInfoMap.value("username").toString(), QString("valid@user.test"));

}

void TestUsermanager::unauthenticatedCallAfterTokenRemove()
{
    removeToken();

    QSignalSpy spy(m_mockTcpServer, &MockTcpServer::connectionTerminated);

    QVariant response = injectAndWait("Users.GetTokens");
    QCOMPARE(response.toMap().value("status").toString(), QString("unauthorized"));

    if (spy.count() == 0) {
        spy.wait();
    }
    QVERIFY2(spy.count() == 1, "Connection should be terminated!");

    // need to restart as our connection dies
    restartServer();
}

#include "testusermanager.moc"
QTEST_MAIN(TestUsermanager)
