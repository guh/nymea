/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 **
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
    m_mockDevice1Port = 1337 + (qrand() % 10000);
    m_mockDevice2Port = 7331 + (qrand() % 10000);

    // Important for settings
    QCoreApplication::instance()->setOrganizationName("nymea-test");
}

void NymeaTestBase::initTestCase()
{
    qDebug() << "NymeaTestBase starting.";

    // If testcase asserts cleanup won't do. Lets clear any previous test run settings leftovers
    qDebug() << "Reset test settings";
    NymeaSettings rulesSettings(NymeaSettings::SettingsRoleRules);
    rulesSettings.clear();
    NymeaSettings deviceSettings(NymeaSettings::SettingsRoleDevices);
    deviceSettings.clear();
    NymeaSettings pluginSettings(NymeaSettings::SettingsRolePlugins);
    pluginSettings.clear();
    NymeaSettings statesSettings(NymeaSettings::SettingsRoleDeviceStates);
    statesSettings.clear();

    // Reset to default settings
    NymeaSettings nymeadSettings(NymeaSettings::SettingsRoleGlobal);
    nymeadSettings.clear();

    QLoggingCategory::setFilterRules("*.debug=false\nTests.debug=true\nMockDevice.debug=true");

    // Start the server
    NymeaCore::instance()->init();

    // Wait unitl the server is initialized
    QSignalSpy coreInitializedSpy(NymeaCore::instance(), SIGNAL(initialized()));
    QVERIFY(coreInitializedSpy.wait());

    // Yes, we're intentionally mixing upper/lower case email here... username should not be case sensitive
    NymeaCore::instance()->userManager()->removeUser("dummy@guh.io");
    NymeaCore::instance()->userManager()->createUser("dummy@guh.io", "DummyPW1!");
    m_apiToken = NymeaCore::instance()->userManager()->authenticate("Dummy@guh.io", "DummyPW1!", "testcase");

    if (MockTcpServer::servers().isEmpty()) {
        qWarning() << "no mock tcp server found";
        exit(-1);
    }

    // Add the mockdevice
    m_mockTcpServer = MockTcpServer::servers().first();
    m_clientId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(m_clientId);

    QVariant response = injectAndWait("JSONRPC.Hello");

    createMockDevice();

    response = injectAndWait("Devices.GetConfiguredDevices", {});
    foreach (const QVariant &device, response.toMap().value("params").toMap().value("devices").toList()) {
        if (device.toMap().value("deviceClassId").toUuid() == mockDeviceAutoDeviceClassId) {
            m_mockDeviceAutoId = DeviceId(device.toMap().value("id").toString());
        }
    }
}

void NymeaTestBase::cleanupTestCase()
{
    NymeaCore::instance()->destroy();
}

void NymeaTestBase::cleanup()
{
    // In case a test deleted the mock device, lets recreate it.
    if (NymeaCore::instance()->deviceManager()->findConfiguredDevices(mockDeviceClassId).count() == 0) {
        createMockDevice();
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
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parser error" << error.errorString();
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

void NymeaTestBase::restartServer()
{
    // Destroy and recreate the core instance...
    NymeaCore::instance()->destroy();
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

void NymeaTestBase::createMockDevice()
{
    QVariantMap params;
    params.insert("name", "Test Mock Device");
    params.insert("deviceClassId", mockDeviceClassId.toString());

    QVariantList deviceParams;
    QVariantMap httpPortParam;
    httpPortParam.insert("paramTypeId", mockDeviceHttpportParamTypeId.toString());
    httpPortParam.insert("value", m_mockDevice1Port);
    deviceParams.append(httpPortParam);
    params.insert("deviceParams", deviceParams);

    QVariant response = injectAndWait("Devices.AddConfiguredDevice", params);

    verifyError(response, "deviceError", "DeviceErrorNoError");

    m_mockDeviceId = DeviceId(response.toMap().value("params").toMap().value("deviceId").toString());
    QVERIFY2(!m_mockDeviceId.isNull(), "Newly created mock device must not be null.");
}

