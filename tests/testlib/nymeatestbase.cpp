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
#include "nymeacore.h"
#include "nymeasettings.h"
#include "servers/mocktcpserver.h"
#include "usermanager/usermanager.h"

using namespace nymeaserver;

Q_LOGGING_CATEGORY(dcTests, "Tests")

#include "../plugins/mock/plugininfo.h"

NymeaTestBase::NymeaTestBase(QObject *parent) :
    QObject(parent),
    m_commandId(0)
{
    qRegisterMetaType<QNetworkReply*>();
    qsrand(QDateTime::currentMSecsSinceEpoch());
    m_mockThing1Port = 1337 + (qrand() % 10000);
    m_mockThing2Port = 7331 + (qrand() % 10000);

    // Important for settings
    QCoreApplication::instance()->setOrganizationName("nymea-test");
}

void NymeaTestBase::initTestCase()
{
    qCDebug(dcTests) << "NymeaTestBase starting.";

    // If testcase asserts cleanup won't do. Lets clear any previous test run settings leftovers
    NymeaSettings rulesSettings(NymeaSettings::SettingsRoleRules);
    rulesSettings.clear();
    NymeaSettings thingSettings(NymeaSettings::SettingsRoleThings);
    thingSettings.clear();
    NymeaSettings pluginSettings(NymeaSettings::SettingsRolePlugins);
    pluginSettings.clear();
    NymeaSettings statesSettings(NymeaSettings::SettingsRoleThingStates);
    statesSettings.clear();

    // Reset to default settings
    NymeaSettings nymeadSettings(NymeaSettings::SettingsRoleGlobal);
    nymeadSettings.clear();

    QLoggingCategory::setFilterRules("*.debug=false\nApplication.debug=true\nTests.debug=true\nMock.debug=true");

    // Start the server
    qCDebug(dcTests()) << "Setting up nymea core instance";
    NymeaCore::instance()->init();

    // Wait unitl the server is initialized
    QSignalSpy coreInitializedSpy(NymeaCore::instance(), SIGNAL(initialized()));
    QVERIFY(coreInitializedSpy.wait());
    qApp->processEvents();
    qCDebug(dcTests()) << "Nymea core instance initialized. Creating dummy user.";

    // Yes, we're intentionally mixing upper/lower case email here... username should not be case sensitive
    NymeaCore::instance()->userManager()->removeUser("dummy@guh.io");
    NymeaCore::instance()->userManager()->createUser("dummy@guh.io", "DummyPW1!", Types::PermissionScopeAdmin);
    m_apiToken = NymeaCore::instance()->userManager()->authenticate("Dummy@guh.io", "DummyPW1!", "testcase");

    if (MockTcpServer::servers().isEmpty()) {
        qCWarning(dcTests) << "no mock tcp server found";
        exit(-1);
    }

    // Add the mock
    m_mockTcpServer = MockTcpServer::servers().first();
    m_clientId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(m_clientId);

    qCDebug(dcTests()) << "Starting JSON handshake";
    QVariant response = injectAndWait("JSONRPC.Hello");

    createMock();

    response = injectAndWait("Integrations.GetThings", {});
    foreach (const QVariant &thing, response.toMap().value("params").toMap().value("things").toList()) {
        if (thing.toMap().value("thingClassId").toUuid() == autoMockThingClassId) {
            m_mockThingAutoId = ThingId(thing.toMap().value("id").toString());
        }
    }
}

void NymeaTestBase::cleanupTestCase()
{
    NymeaCore::instance()->destroy();
}

void NymeaTestBase::cleanup()
{
    // In case a test deleted the mock, lets recreate it.
    if (NymeaCore::instance()->thingManager()->findConfiguredThings(mockThingClassId).count() == 0) {
        createMock();
    }
}

QVariant NymeaTestBase::injectAndWait(const QString &method, const QVariantMap &params, const QUuid &clientId)
{
    QVariantMap call;
    call.insert("id", m_commandId);
    call.insert("method", method);
    call.insert("params", params);
    call.insert("token", m_apiToken);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(call);
    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    m_mockTcpServer->injectData(clientId.isNull() ? m_clientId : clientId, jsonDoc.toJson(QJsonDocument::Compact) + "\n");

    int loop = 0;

    while (loop < 5) {

        if (spy.count() == 0 || loop > 0) {
            spy.wait();
        }

        for (int i = 0; i < spy.count(); i++) {
            // Make sure the response it a valid JSON string
            QJsonParseError error;
            jsonDoc = QJsonDocument::fromJson(spy.at(i).last().toByteArray(), &error);
            if (error.error != QJsonParseError::NoError) {
                qWarning() << "JSON parser error" << error.errorString() << spy.at(i).last().toByteArray();
                return QVariant();
            }
            QVariantMap response = jsonDoc.toVariant().toMap();

            // skip notifications
            if (response.contains("notification"))
                continue;

            if (response.value("id").toInt() == m_commandId) {
                m_commandId++;
                return jsonDoc.toVariant();
            }
        }
        loop++;
    }

    m_commandId++;
    return QVariant();
}

QVariant NymeaTestBase::checkNotification(const QSignalSpy &spy, const QString &notification)
{
    //qDebug() << "Got" << spy.count() << "notifications while waiting for" << notification;
    for (int i = 0; i < spy.count(); i++) {
        // Make sure the response it a valid JSON string
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.at(i).last().toByteArray(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parser error" << error.errorString();
            return QVariant();
        }

        QVariantMap response = jsonDoc.toVariant().toMap();
        if (response.value("notification").toString() == notification) {
            return jsonDoc.toVariant();
        }
    }
    return QVariant();
}

QVariantList NymeaTestBase::checkNotifications(const QSignalSpy &spy, const QString &notification)
{
//    qWarning() << "Got" << spy.count() << "notifications while waiting for" << notification;
    QVariantList notificationList;
    for (int i = 0; i < spy.count(); i++) {
        // Make sure the response it a valid JSON string
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.at(i).last().toByteArray(), &error);
//        qCDebug(dcTests()) << "Got packet:" << qUtf8Printable(jsonDoc.toJson());
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTests()) << "JSON parser error" << error.errorString();
            return notificationList;
        }

        QVariantMap response = jsonDoc.toVariant().toMap();
        if (response.value("notification").toString() == notification) {
            notificationList.append(jsonDoc.toVariant());
        }
    }
    return notificationList;
}

QVariant NymeaTestBase::getAndWait(const QNetworkRequest &request, const int &expectedStatus)
{
    QNetworkAccessManager nam;
    connect(&nam, &QNetworkAccessManager::sslErrors, [&nam](QNetworkReply *reply, const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });
    QSignalSpy clientSpy(&nam, SIGNAL(finished(QNetworkReply*)));

    QNetworkReply *reply = nam.get(request);

    if (clientSpy.count() == 0) {
        clientSpy.wait();
    }
    qCDebug(dcTests()) << "*** finished" << reply->isFinished() << reply->error() << reply->errorString();

    if (clientSpy.count() == 0) {
        qCWarning(dcTests()) << "Got no response for get request";
        reply->deleteLater();
        return QVariant();
    }

    QByteArray data = reply->readAll();
    verifyReply(reply, data, expectedStatus);

    qCDebug(dcTests()) << "Data is:" << data;

    reply->deleteLater();

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parser error" << error.errorString();
        return QVariant();
    }

    return jsonDoc.toVariant();
}

QVariant NymeaTestBase::deleteAndWait(const QNetworkRequest &request, const int &expectedStatus)
{
    QNetworkAccessManager nam;
    connect(&nam, &QNetworkAccessManager::sslErrors, [this, &nam](QNetworkReply *reply, const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });
    QSignalSpy clientSpy(&nam, SIGNAL(finished(QNetworkReply*)));

    QNetworkReply *reply = nam.deleteResource(request);

    if (clientSpy.count() == 0) {
        clientSpy.wait();
    }

    if (clientSpy.count() == 0) {
        qWarning() << "Got no response for delete request";
        reply->deleteLater();
        return QVariant();
    }

    QByteArray data = reply->readAll();
    verifyReply(reply, data, expectedStatus);

    reply->deleteLater();

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parser error" << error.errorString();
        return QVariant();
    }

    return jsonDoc.toVariant();
}

QVariant NymeaTestBase::postAndWait(const QNetworkRequest &request, const QVariant &params, const int &expectedStatus)
{
    QNetworkAccessManager nam;
    connect(&nam, &QNetworkAccessManager::sslErrors, [this, &nam](QNetworkReply *reply, const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });
    QSignalSpy clientSpy(&nam, SIGNAL(finished(QNetworkReply*)));

    QByteArray payload = QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = nam.post(request, payload);

    if (clientSpy.count() == 0) {
        clientSpy.wait();
    }

    if (clientSpy.count() == 0) {
        qWarning() << "Got no response for post request";
        reply->deleteLater();
        return QVariant();
    }



    QByteArray data = reply->readAll();
    verifyReply(reply, data, expectedStatus);

    reply->deleteLater();

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parser error" << error.errorString() << qUtf8Printable(data);
        return QVariant();
    }

    return jsonDoc.toVariant();
}


QVariant NymeaTestBase::putAndWait(const QNetworkRequest &request, const QVariant &params, const int &expectedStatus)
{
    QNetworkAccessManager nam;
    connect(&nam, &QNetworkAccessManager::sslErrors, [this, &nam](QNetworkReply *reply, const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });
    QSignalSpy clientSpy(&nam, SIGNAL(finished(QNetworkReply*)));

    QByteArray payload = QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = nam.put(request, payload);

    if (clientSpy.count() == 0) {
        clientSpy.wait();
    }

    if (clientSpy.count() == 0) {
        qWarning() << "Got no response for put request";
        reply->deleteLater();
        return QVariant();
    }

    QByteArray data = reply->readAll();
    verifyReply(reply, data, expectedStatus);

    reply->deleteLater();

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parser error" << error.errorString();
        return QVariant();
    }

    return jsonDoc.toVariant();
}

void NymeaTestBase::verifyReply(QNetworkReply *reply, const QByteArray &data, const int &expectedStatus)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QCOMPARE(statusCode, expectedStatus);

    Q_UNUSED(data)
//    if (!data.isEmpty()) {
//        QJsonParseError error;
//        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
//        QCOMPARE(error.error, QJsonParseError::NoError);
//        Q_UNUSED(jsonDoc);
//    }
}

void NymeaTestBase::enableNotifications(const QStringList &namespaces)
{
    QVariantList variantList;
    foreach (const QString &ns, namespaces) {
        variantList << ns;
    }
    std::sort(variantList.begin(), variantList.end());
    QVariantMap notificationParams;
    notificationParams.insert("namespaces", variantList);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", notificationParams);
    QVariantList resultList = response.toMap().value("params").toMap().value("namespaces").toList();
    std::sort(resultList.begin(), resultList.end());
    QCOMPARE(resultList, variantList);
}

bool NymeaTestBase::disableNotifications()
{
    QVariantMap notificationParams;
    notificationParams.insert("enabled", false);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", notificationParams);
    if (response.toMap().value("params").toMap().value("enabled").toBool() != false) {
        return false;
    }
    qDebug() << "Notifications disabled.";
    return true;
}

void NymeaTestBase::waitForDBSync()
{
    while (NymeaCore::instance()->logEngine()->jobsRunning()) {
        qApp->processEvents();
    }
}

void NymeaTestBase::restartServer()
{
    // Destroy and recreate the core instance...
    qCDebug(dcTests()) << "Tearing down server instance";
    NymeaCore::instance()->destroy();
    qCDebug(dcTests()) << "Restarting server instance";
    NymeaCore::instance()->init();
    QSignalSpy coreSpy(NymeaCore::instance(), SIGNAL(initialized()));
    coreSpy.wait();
    m_mockTcpServer = MockTcpServer::servers().first();
    m_mockTcpServer->clientConnected(m_clientId);

    injectAndWait("JSONRPC.Hello");
}

void NymeaTestBase::clearLoggingDatabase()
{
    NymeaCore::instance()->logEngine()->clearDatabase();
}

void NymeaTestBase::createMock()
{
    QVariantMap params;
    params.insert("name", "Test Mock");
    params.insert("thingClassId", mockThingClassId.toString());

    QVariantList thingParams;
    QVariantMap httpPortParam;
    httpPortParam.insert("paramTypeId", mockThingHttpportParamTypeId.toString());
    httpPortParam.insert("value", m_mockThing1Port);
    thingParams.append(httpPortParam);
    params.insert("thingParams", thingParams);

    QVariant response = injectAndWait("Integrations.AddThing", params);

    verifyError(response, "thingError", "ThingErrorNoError");

    m_mockThingId = ThingId(response.toMap().value("params").toMap().value("thingId").toString());
    QVERIFY2(!m_mockThingId.isNull(), "Newly created mock thing id must not be null.");
}

