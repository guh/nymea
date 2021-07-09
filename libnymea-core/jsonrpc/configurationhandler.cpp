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

/*!
    \class nymeaserver::ConfigurationHandler
    \brief This subclass of \l{JsonHandler} processes the JSON requests for the \tt Configuration namespace.

    \ingroup json
    \inmodule core

    This \l{JsonHandler} will be created in the \l{JsonRPCServer} and used to handle JSON-RPC requests
    for the \tt {Configuration} namespace of the API.

    \sa JsonHandler, JsonRPCServer
*/

/*! \fn void nymeaserver::ConfigurationHandler::BasicConfigurationChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when the configurations of the server have been changed.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::ConfigurationHandler::TcpServerConfigurationChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when the configurations of the \l{TcpServer} have been changed.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::ConfigurationHandler::WebServerConfigurationChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when the configurations of the \l{WebServer} have been changed.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::ConfigurationHandler::WebSocketServerConfigurationChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when the configurations of the \l{WebSocketServer} have been changed.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::ConfigurationHandler::LanguageChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when the language of the system has changed.
    The \a params contains the map for the notification.
*/



#include "configurationhandler.h"
#include "nymeacore.h"
#include "nymeaconfiguration.h"
#include "platform/platform.h"
#include "platform/platformsystemcontroller.h"

namespace nymeaserver {

/*! Constructs a new \l ConfigurationHandler with the given \a parent. */
ConfigurationHandler::ConfigurationHandler(QObject *parent):
    JsonHandler(parent)
{
    // Enums
    registerEnum<NymeaConfiguration::ConfigurationError>();

    // Objects
    registerObject<ServerConfiguration>();
    registerObject<WebServerConfiguration>();
    registerObject<MqttPolicy>();

    // Methods
    QString description; QVariantMap params; QVariantMap returns;
    description = "Get the list of available timezones.";
    returns.insert("timeZones", QVariantList() << enumValueName(String));
    registerMethod("GetTimeZones", description, params, returns, Types::PermissionScopeNone, "Use System.GetTimeZones instead.");

    params.clear(); returns.clear();
    description = "Returns a list of locale codes available for the server. i.e. en_US, de_AT";
    returns.insert("languages", QVariantList() << enumValueName(String));
    registerMethod("GetAvailableLanguages", description, params, returns, Types::PermissionScopeNone, "Use the locale property in the Handshake message instead.");

    params.clear(); returns.clear();
    description = "Get all configuration parameters of the server.";
    QVariantMap basicConfiguration;
    basicConfiguration.insert("serverName", enumValueName(String));
    basicConfiguration.insert("serverUuid", enumValueName(Uuid));
    basicConfiguration.insert("d:serverTime", enumValueName(Uint));
    basicConfiguration.insert("d:timeZone", enumValueName(String));
    basicConfiguration.insert("d:language", enumValueName(String));
    basicConfiguration.insert("debugServerEnabled", enumValueName(Bool));
    returns.insert("basicConfiguration", basicConfiguration);
    QVariantList tcpServerConfigurations;
    tcpServerConfigurations.append(objectRef<ServerConfiguration>());
    returns.insert("tcpServerConfigurations", tcpServerConfigurations);
    QVariantList webServerConfigurations;
    webServerConfigurations.append(objectRef<WebServerConfiguration>());
    returns.insert("webServerConfigurations", webServerConfigurations);
    QVariantList webSocketServerConfigurations;
    webSocketServerConfigurations.append(objectRef<ServerConfiguration>());
    returns.insert("webSocketServerConfigurations", webSocketServerConfigurations);
    QVariantList mqttServerConfigurations;
    mqttServerConfigurations.append(objectRef<ServerConfiguration>());
    QVariantMap cloudConfiguration;
    cloudConfiguration.insert("enabled", enumValueName(Bool));
    returns.insert("cloud", cloudConfiguration);
    registerMethod("GetConfigurations", description, params, returns);

    params.clear(); returns.clear();
    description = "Set the name of the server. Default is nymea.";
    params.insert("serverName",  enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetServerName", description, params, returns);

    params.clear(); returns.clear();
    description = "Set the time zone of the server. See also: \"GetTimeZones\"";
    params.insert("timeZone",  enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetTimeZone", description, params, returns, Types::PermissionScopeAdmin, "Use System.SetTimeZone instead.");

    params.clear(); returns.clear();
    description = "Sets the server language to the given language. See also: \"GetAvailableLanguages\"";
    params.insert("language",  enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetLanguage", description, params, returns, Types::PermissionScopeAdmin, "Use the locale property in the Handshake message instead.");

    params.clear(); returns.clear();
    description = "Enable or disable the debug server.";
    params.insert("enabled",  enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetDebugServerEnabled", description, params, returns);

    params.clear(); returns.clear();
    description = "Configure a TCP interface of the server. If the ID is an existing one, the existing config will be modified, otherwise a new one will be added. Note: if you are changing the configuration for the interface you are currently connected to, the connection will be dropped.";
    params.insert("configuration", objectRef<ServerConfiguration>());
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetTcpServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Delete a TCP interface of the server. Note: if you are deleting the configuration for the interface you are currently connected to, the connection will be dropped.";
    params.insert("id", enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("DeleteTcpServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Configure a WebSocket Server interface of the server. If the ID is an existing one, the existing config will be modified, otherwise a new one will be added. Note: if you are changing the configuration for the interface you are currently connected to, the connection will be dropped.";
    params.insert("configuration", objectRef<ServerConfiguration>());
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetWebSocketServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Delete a WebSocket Server interface of the server. Note: if you are deleting the configuration for the interface you are currently connected to, the connection will be dropped.";
    params.insert("id", enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("DeleteWebSocketServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Configure a WebServer interface of the server. If the ID is an existing one, the existing config will be modified, otherwise a new one will be added.";
    params.insert("configuration", objectRef<WebServerConfiguration>());
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetWebServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Delete a WebServer interface of the server.";
    params.insert("id", enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("DeleteWebServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Sets whether the cloud connection is enabled or disabled in the settings.";
    params.insert("enabled", enumValueName(Bool));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetCloudEnabled", description, params, returns);

    // MQTT
    params.clear(); returns.clear();
    description = "Get all MQTT Server configurations.";
    returns.insert("mqttServerConfigurations", QVariantList() << objectRef<ServerConfiguration>());
    registerMethod("GetMqttServerConfigurations", description, params, returns);

    params.clear(); returns.clear();
    description = "Configure a MQTT Server interface on the MQTT broker. If the ID is an existing one, the existing config will be modified, otherwise a new one will be added. Setting authenticationEnabled to true will require MQTT clients to use credentials set in the MQTT broker policies.";
    params.insert("configuration", objectRef<ServerConfiguration>());
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetMqttServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Delete a MQTT Server interface of the server.";
    params.insert("id", enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("DeleteMqttServerConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Get all MQTT broker policies.";
    returns.insert("mqttPolicies", QVariantList() << objectRef<MqttPolicy>());
    registerMethod("GetMqttPolicies", description, params, returns);

    params.clear(); returns.clear();
    description = "Configure a MQTT broker policy. If the ID is an existing one, the existing policy will be modified, otherwise a new one will be added.";
    params.insert("policy", objectRef<MqttPolicy>());
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("SetMqttPolicy", description, params, returns);

    params.clear(); returns.clear();
    description = "Delete a MQTT policy from the broker.";
    params.insert("clientId", enumValueName(String));
    returns.insert("configurationError", enumRef<NymeaConfiguration::ConfigurationError>());
    registerMethod("DeleteMqttPolicy", description, params, returns);

    // Notifications
    params.clear(); returns.clear();
    description = "Emitted whenever the basic configuration of this server changes.";
    params.insert("basicConfiguration", basicConfiguration);
    registerNotification("BasicConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the language of the server changed. The Plugins, Vendors and ThingClasses have to be reloaded to get the translated data.";
    params.insert("language", enumValueName(String));
    registerNotification("LanguageChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the TCP server configuration changes.";
    params.insert("tcpServerConfiguration", objectRef<ServerConfiguration>());
    registerNotification("TcpServerConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a TCP server configuration is removed.";
    params.insert("id", enumValueName(String));
    registerNotification("TcpServerConfigurationRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the web socket server configuration changes.";
    params.insert("webSocketServerConfiguration", objectRef<ServerConfiguration>());
    registerNotification("WebSocketServerConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a WebSocket server configuration is removed.";
    params.insert("id", enumValueName(String));
    registerNotification("WebSocketServerConfigurationRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the MQTT broker configuration is changed.";
    params.insert("mqttServerConfiguration", objectRef<ServerConfiguration>());
    registerNotification("MqttServerConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a MQTT server configuration is removed.";
    params.insert("id", enumValueName(String));
    registerNotification("MqttServerConfigurationRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the web server configuration changes.";
    params.insert("webServerConfiguration", objectRef<WebServerConfiguration>());
    registerNotification("WebServerConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a Web server configuration is removed.";
    params.insert("id", enumValueName(String));
    registerNotification("WebServerConfigurationRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the cloud configuration is changed.";
    params.insert("cloudConfiguration", cloudConfiguration);
    registerNotification("CloudConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a MQTT broker policy is changed.";
    params.insert("policy", objectRef<MqttPolicy>());
    registerNotification("MqttPolicyChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a MQTT broker policy is removed.";
    params.insert("clientId", enumValueName(String));
    registerNotification("MqttPolicyRemoved", description, params);

    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::serverNameChanged, this, &ConfigurationHandler::onBasicConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::timeZoneChanged, this, &ConfigurationHandler::onBasicConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::localeChanged, this, &ConfigurationHandler::onBasicConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::debugServerEnabledChanged, this, &ConfigurationHandler::onBasicConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::localeChanged, this, &ConfigurationHandler::onLanguageChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::tcpServerConfigurationChanged, this, &ConfigurationHandler::onTcpServerConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::tcpServerConfigurationRemoved, this, &ConfigurationHandler::onTcpServerConfigurationRemoved);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::webServerConfigurationChanged, this, &ConfigurationHandler::onWebServerConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::webServerConfigurationRemoved, this, &ConfigurationHandler::onWebServerConfigurationRemoved);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::webSocketServerConfigurationChanged, this, &ConfigurationHandler::onWebSocketServerConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::webSocketServerConfigurationRemoved, this, &ConfigurationHandler::onWebSocketServerConfigurationRemoved);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::mqttServerConfigurationChanged, this, &ConfigurationHandler::onMqttServerConfigurationChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::mqttServerConfigurationRemoved, this, &ConfigurationHandler::onMqttServerConfigurationRemoved);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::mqttPolicyChanged, this, &ConfigurationHandler::onMqttPolicyChanged);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::mqttPolicyRemoved, this, &ConfigurationHandler::onMqttPolicyRemoved);
    connect(NymeaCore::instance()->configuration(), &NymeaConfiguration::cloudEnabledChanged, this, &ConfigurationHandler::onCloudConfigurationChanged);
}

/*! Returns the name of the \l{ConfigurationHandler}. In this case \b Configuration.*/
QString ConfigurationHandler::name() const
{
    return "Configuration";
}

JsonReply *ConfigurationHandler::GetConfigurations(const QVariantMap &params) const
{
    Q_UNUSED(params)
    QVariantMap returns;
    returns.insert("basicConfiguration", packBasicConfiguration());
    QVariantList tcpServerConfigs;
    foreach (const ServerConfiguration &config, NymeaCore::instance()->configuration()->tcpServerConfigurations()) {
        tcpServerConfigs.append(pack(config));
    }
    returns.insert("tcpServerConfigurations", tcpServerConfigs);

    QVariantList webServerConfigs;
    foreach (const WebServerConfiguration &config, NymeaCore::instance()->configuration()->webServerConfigurations()) {
        webServerConfigs.append(pack(config));

    }
    returns.insert("webServerConfigurations", webServerConfigs);

    QVariantList webSocketServerConfigs;
    foreach (const ServerConfiguration &config, NymeaCore::instance()->configuration()->webSocketServerConfigurations()) {
        webSocketServerConfigs.append(pack(config));
    }
    returns.insert("webSocketServerConfigurations", webSocketServerConfigs);

    QVariantMap cloudConfig;
    cloudConfig.insert("enabled", NymeaCore::instance()->configuration()->cloudEnabled());
    returns.insert("cloud", cloudConfig);

    return createReply(returns);
}

JsonReply *ConfigurationHandler::GetTimeZones(const QVariantMap &params) const
{
    Q_UNUSED(params)
    QVariantList timeZones;
    foreach (const QByteArray &timeZoneId, QTimeZone::availableTimeZoneIds()) {
        timeZones.append(QString::fromUtf8(timeZoneId));
    }

    QVariantMap returns;
    returns.insert("timeZones", timeZones);
    return createReply(returns);
}

JsonReply *ConfigurationHandler::GetAvailableLanguages(const QVariantMap &params) const
{
    Q_UNUSED(params)
    QVariantList languages;
    foreach (const QString &language, NymeaCore::getAvailableLanguages()) {
        languages.append(language);
    }
    QVariantMap returns;
    returns.insert("languages", languages);
    return createReply(returns);
}

JsonReply *ConfigurationHandler::SetServerName(const QVariantMap &params) const
{
    QString serverName = params.value("serverName").toString();
    NymeaCore::instance()->configuration()->setServerName(serverName);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::SetTimeZone(const QVariantMap &params) const
{
    qCDebug(dcJsonRpc()) << "Setting time zone to" << params.value("timeZone").toString();

    QByteArray timeZoneName = params.value("timeZone").toString().toUtf8();

    QTimeZone timeZone(timeZoneName);
    if (!timeZone.isValid()) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidTimeZone));
    }

    bool success = NymeaCore::instance()->platform()->systemController()->setTimeZone(timeZone);
    if (!success) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidTimeZone));
    }

    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::SetLanguage(const QVariantMap &params) const
{
    qCDebug(dcJsonRpc()) << "Setting language to" << params.value("language").toString();
    QLocale locale(params.value("language").toString());

    NymeaCore::instance()->configuration()->setLocale(locale);

    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::SetTcpServerConfiguration(const QVariantMap &params) const
{
    ServerConfiguration config = unpack<ServerConfiguration>(params.value("configuration").toMap());
    if (config.id.isEmpty()) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    if (config.address.isNull())
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidHostAddress));

    if (config.port <= 0 || config.port > 65535) {
        qCWarning(dcJsonRpc()) << "Port out of range";
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidPort));
    }

    qCDebug(dcJsonRpc()) << QString("Configure TCP server %1:%2").arg(config.address.toString()).arg(config.port);

    NymeaCore::instance()->configuration()->setTcpServerConfiguration(config);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::DeleteTcpServerConfiguration(const QVariantMap &params) const
{
    QString id = params.value("id").toString();
    if (id.isEmpty() || !NymeaCore::instance()->configuration()->tcpServerConfigurations().contains(id)) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    NymeaCore::instance()->configuration()->removeTcpServerConfiguration(id);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::SetWebServerConfiguration(const QVariantMap &params) const
{
    WebServerConfiguration config = unpack<WebServerConfiguration>(params.value("configuration").toMap());

    if (config.id.isEmpty()) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    if (config.address.isNull())
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidHostAddress));

    if (config.port <= 0 || config.port > 65535) {
        qCWarning(dcJsonRpc()) << "Port out of range";
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidPort));
    }

    qCDebug(dcJsonRpc()) << QString("Configure web server %1:%2").arg(config.address.toString()).arg(config.port);

    NymeaCore::instance()->configuration()->setWebServerConfiguration(config);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::DeleteWebServerConfiguration(const QVariantMap &params) const
{
    QString id = params.value("id").toString();
    if (id.isEmpty() || !NymeaCore::instance()->configuration()->webServerConfigurations().contains(id)) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    NymeaCore::instance()->configuration()->removeWebServerConfiguration(id);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::SetWebSocketServerConfiguration(const QVariantMap &params) const
{
    ServerConfiguration config = unpack<ServerConfiguration>(params.value("configuration").toMap());
    if (config.id.isEmpty()) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    if (config.address.isNull())
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidHostAddress));

    if (config.port <= 0 || config.port > 65535) {
        qCWarning(dcJsonRpc()) << "Port out of range";
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidPort));
    }

    qCDebug(dcJsonRpc()) << QString("Configuring web socket server %1:%2").arg(config.address.toString()).arg(config.port);

    NymeaCore::instance()->configuration()->setWebSocketServerConfiguration(config);

    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::DeleteWebSocketServerConfiguration(const QVariantMap &params) const
{
    QString id = params.value("id").toString();
    if (id.isEmpty() || !NymeaCore::instance()->configuration()->webSocketServerConfigurations().contains(id)) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    NymeaCore::instance()->configuration()->removeWebSocketServerConfiguration(id);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::GetMqttServerConfigurations(const QVariantMap &params) const
{
    Q_UNUSED(params)
    QVariantMap ret;
    QVariantList mqttServerConfigs;
    foreach (const ServerConfiguration &config, NymeaCore::instance()->configuration()->mqttServerConfigurations()) {
        mqttServerConfigs << pack(config);
    }
    ret.insert("mqttServerConfigurations", mqttServerConfigs);
    return createReply(ret);
}

JsonReply *ConfigurationHandler::SetMqttServerConfiguration(const QVariantMap &params) const
{
    ServerConfiguration config = unpack<ServerConfiguration>(params.value("configuration").toMap());
    if (config.id.isEmpty()) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    if (config.address.isNull())
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidHostAddress));

    if (config.port <= 0 || config.port > 65535) {
        qCWarning(dcJsonRpc()) << "Port out of range";
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidPort));
    }

    qCDebug(dcJsonRpc()) << QString("Configure MQTT server %1:%2").arg(config.address.toString()).arg(config.port);

    NymeaCore::instance()->configuration()->setMqttServerConfiguration(config);

    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::DeleteMqttServerConfiguration(const QVariantMap &params) const
{
    QString id = params.value("id").toString();
    if (id.isEmpty() || !NymeaCore::instance()->configuration()->mqttServerConfigurations().contains(id)) {
        return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorInvalidId));
    }
    NymeaCore::instance()->configuration()->removeMqttServerConfiguration(id);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::GetMqttPolicies(const QVariantMap &params) const
{
    Q_UNUSED(params)
    QVariantList mqttPolicies;
    foreach (const MqttPolicy &policy, NymeaCore::instance()->configuration()->mqttPolicies()) {
        mqttPolicies << pack(policy);
    }
    QVariantMap ret;
    ret.insert("mqttPolicies", mqttPolicies);
    return createReply(ret);
}

JsonReply *ConfigurationHandler::SetMqttPolicy(const QVariantMap &params) const
{
    MqttPolicy policy = unpack<MqttPolicy>(params.value("policy").toMap());
    NymeaCore::instance()->configuration()->updateMqttPolicy(policy);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::DeleteMqttPolicy(const QVariantMap &params) const
{
    QString clientId = params.value("clientId").toString();
    bool success = NymeaCore::instance()->configuration()->removeMqttPolicy(clientId);
    return createReply(statusToReply(success ? NymeaConfiguration::ConfigurationErrorNoError : NymeaConfiguration::ConfigurationErrorInvalidId));
}

JsonReply *ConfigurationHandler::SetCloudEnabled(const QVariantMap &params) const
{
    bool enabled = params.value("enabled").toBool();
    NymeaCore::instance()->configuration()->setCloudEnabled(enabled);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

JsonReply *ConfigurationHandler::SetDebugServerEnabled(const QVariantMap &params) const
{
    bool enabled = params.value("enabled").toBool();
    NymeaCore::instance()->configuration()->setDebugServerEnabled(enabled);
    return createReply(statusToReply(NymeaConfiguration::ConfigurationErrorNoError));
}

void ConfigurationHandler::onBasicConfigurationChanged()
{
    qCDebug(dcJsonRpc()) << "Notification: Basic configuration changed";
    QVariantMap params;
    params.insert("basicConfiguration", packBasicConfiguration());
    emit BasicConfigurationChanged(params);
}

void ConfigurationHandler::onTcpServerConfigurationChanged(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: TCP server configuration changed";
    QVariantMap params;
    params.insert("tcpServerConfiguration", pack(NymeaCore::instance()->configuration()->tcpServerConfigurations().value(id)));
    emit TcpServerConfigurationChanged(params);
}

void ConfigurationHandler::onTcpServerConfigurationRemoved(const QString &id)
{
    qCDebug(dcJsonRpc) << "Notification: TCP server configuration removed";
    QVariantMap params;
    params.insert("id", id);
    emit TcpServerConfigurationRemoved(params);
}

void ConfigurationHandler::onWebServerConfigurationChanged(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: web server configuration changed";
    QVariantMap params;
    params.insert("webServerConfiguration", pack(NymeaCore::instance()->configuration()->webServerConfigurations().value(id)));
    emit WebServerConfigurationChanged(params);
}

void ConfigurationHandler::onWebServerConfigurationRemoved(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: Web server configuration removed";
    QVariantMap params;
    params.insert("id", id);
    emit WebServerConfigurationRemoved(params);
}

void ConfigurationHandler::onWebSocketServerConfigurationChanged(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: web socket server configuration changed";
    QVariantMap params;
    params.insert("webSocketServerConfiguration", pack(NymeaCore::instance()->configuration()->webSocketServerConfigurations().value(id)));
    emit WebSocketServerConfigurationChanged(params);
}

void ConfigurationHandler::onWebSocketServerConfigurationRemoved(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: WebSocket server configuration removed";
    QVariantMap params;
    params.insert("id", id);
    emit WebSocketServerConfigurationRemoved(params);
}

void ConfigurationHandler::onMqttServerConfigurationChanged(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: MQTT server configuration changed";
    QVariantMap params;
    params.insert("mqttServerConfiguration", pack(NymeaCore::instance()->configuration()->mqttServerConfigurations().value(id)));
    emit MqttServerConfigurationChanged(params);
}

void ConfigurationHandler::onMqttServerConfigurationRemoved(const QString &id)
{
    qCDebug(dcJsonRpc()) << "Notification: MQTT server configuration removed";
    QVariantMap params;
    params.insert("id", id);
    emit MqttServerConfigurationRemoved(params);
}

void ConfigurationHandler::onMqttPolicyChanged(const QString &clientId)
{
    qCDebug(dcJsonRpc()) << "Notification: MQTT policy changed";
    QVariantMap params;
    params.insert("policy", pack(NymeaCore::instance()->configuration()->mqttPolicies().value(clientId)));
    emit MqttPolicyChanged(params);
}

void ConfigurationHandler::onMqttPolicyRemoved(const QString &clientId)
{
    qCDebug(dcJsonRpc()) << "Notification: MQTT policy removed";
    QVariantMap params;
    params.insert("clientId", clientId);
    emit MqttPolicyRemoved(params);
}

QVariantMap ConfigurationHandler::packBasicConfiguration()
{
    QVariantMap basicConfiguration;
    basicConfiguration.insert("serverName", NymeaCore::instance()->configuration()->serverName());
    basicConfiguration.insert("serverUuid", NymeaCore::instance()->configuration()->serverUuid().toString());
    basicConfiguration.insert("serverTime", NymeaCore::instance()->timeManager()->currentDateTime().toTime_t());
    basicConfiguration.insert("timeZone", QTimeZone::systemTimeZoneId());
    basicConfiguration.insert("language", NymeaCore::instance()->configuration()->locale().name());
    basicConfiguration.insert("debugServerEnabled", NymeaCore::instance()->configuration()->debugServerEnabled());
    return basicConfiguration;
}

QVariantMap ConfigurationHandler::statusToReply(NymeaConfiguration::ConfigurationError status) const
{
    QVariantMap returns;
    returns.insert("configurationError", enumValueName<NymeaConfiguration::ConfigurationError>(status));
    return returns;
}

void ConfigurationHandler::onCloudConfigurationChanged(bool enabled)
{
    qCDebug(dcJsonRpc()) << "Notification: cloud configuration changed";
    QVariantMap params;
    QVariantMap cloudConfiguration;
    cloudConfiguration.insert("enabled", enabled);
    params.insert("cloudConfiguration", cloudConfiguration);
    emit CloudConfigurationChanged(params);
}

void ConfigurationHandler::onLanguageChanged()
{
    qCDebug(dcJsonRpc()) << "Notification: language configuration changed";
    QVariantMap params;
    params.insert("language", NymeaCore::instance()->configuration()->locale().name());
    emit LanguageChanged(params);
}

}
