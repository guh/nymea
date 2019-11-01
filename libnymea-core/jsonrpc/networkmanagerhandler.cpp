/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \class nymeaserver::NetworkManagerHandler
    \brief This subclass of \l{JsonHandler} processes the JSON requests for the \tt NetworkManager namespace of the JSON-RPC API.

    \ingroup json
    \inmodule core

    This \l{JsonHandler} will be created in the \l{JsonRPCServer} and used to handle JSON-RPC requests
    for the \tt {NetworkManager} namespace of the API.

    \sa NetworkManager, JsonHandler, JsonRPCServer
*/

/*! \fn void nymeaserver::NetworkManagerHandler::NetworkStatusChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when the state of the \l{NetworkManager} has changed.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::NetworkManagerHandler::WiredNetworkDeviceAdded(const QVariantMap &params);
    This signal is emitted to the API notifications when a \l{WiredNetworkDevice} has been added to the \l{NetworkManager}.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::NetworkManagerHandler::WiredNetworkDeviceRemoved(const QVariantMap &params);
    This signal is emitted to the API notifications when a \l{WiredNetworkDevice} has been removed from the \l{NetworkManager}.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::NetworkManagerHandler::WiredNetworkDeviceChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when a \l{WiredNetworkDevice} has changed in the \l{NetworkManager}.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::NetworkManagerHandler::WirelessNetworkDeviceAdded(const QVariantMap &params);
    This signal is emitted to the API notifications when a \l{WirelessNetworkDevice} has been added to the \l{NetworkManager}.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::NetworkManagerHandler::WirelessNetworkDeviceRemoved(const QVariantMap &params);
    This signal is emitted to the API notifications when a \l{WirelessNetworkDevice} has been removed from the \l{NetworkManager}.
    The \a params contains the map for the notification.
*/

/*! \fn void nymeaserver::NetworkManagerHandler::WirelessNetworkDeviceChanged(const QVariantMap &params);
    This signal is emitted to the API notifications when a \l{WirelessNetworkDevice} has changed in the \l{NetworkManager}.
    The \a params contains the map for the notification.
*/


#include "nymeacore.h"
#include "loggingcategories.h"
#include "networkmanagerhandler.h"
#include "networkmanager/networkmanager.h"


namespace nymeaserver {

/*! Constructs a new \l{NetworkManagerHandler} with the given \a parent. */
NetworkManagerHandler::NetworkManagerHandler(QObject *parent) :
    JsonHandler(parent)
{
    // Enums
    registerEnum<NetworkManager::NetworkManagerError>();
    registerEnum<NetworkManager::NetworkManagerState>();
    registerEnum<NetworkDevice::NetworkDeviceState>();

    // Objects
    QVariantMap wirelessAccessPoint;
    wirelessAccessPoint.insert("ssid", enumValueName(String));
    wirelessAccessPoint.insert("macAddress", enumValueName(String));
    wirelessAccessPoint.insert("frequency", enumValueName(Double));
    wirelessAccessPoint.insert("signalStrength", enumValueName(Int));
    wirelessAccessPoint.insert("protected", enumValueName(Bool));
    registerObject("WirelessAccessPoint", wirelessAccessPoint);

    QVariantMap wiredNetworkDevice;
    wiredNetworkDevice.insert("interface", enumValueName(String));
    wiredNetworkDevice.insert("macAddress", enumValueName(String));
    wiredNetworkDevice.insert("state", enumRef<NetworkDevice::NetworkDeviceState>());
    wiredNetworkDevice.insert("bitRate", enumValueName(String));
    wiredNetworkDevice.insert("pluggedIn", enumValueName(Bool));
    registerObject("WiredNetworkDevice", wiredNetworkDevice);

    QVariantMap wirelessNetworkDevice;
    wirelessNetworkDevice.insert("interface", enumValueName(String));
    wirelessNetworkDevice.insert("macAddress", enumValueName(String));
    wirelessNetworkDevice.insert("state", enumRef<NetworkDevice::NetworkDeviceState>());
    wirelessNetworkDevice.insert("bitRate", enumValueName(String));
    wirelessNetworkDevice.insert("o:currentAccessPoint", objectRef("WirelessAccessPoint"));
    registerObject("WirelessNetworkDevice", wirelessNetworkDevice);

    // Methods
    QString description; QVariantMap params; QVariantMap returns;
    description = "Get the current network manager status.";
    QVariantMap status;
    status.insert("networkingEnabled", enumValueName(Bool));
    status.insert("wirelessNetworkingEnabled", enumValueName(Bool));
    status.insert("state", enumRef<NetworkManager::NetworkManagerState>());
    returns.insert("o:status", status);
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("GetNetworkStatus", description, params, returns);

    params.clear(); returns.clear();
    description = "Enable or disable networking in the NetworkManager.";
    params.insert("enable", enumValueName(Bool));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("EnableNetworking", description, params, returns);

    params.clear(); returns.clear();
    description = "Enable or disable wireless networking in the NetworkManager.";
    params.insert("enable", enumValueName(Bool));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("EnableWirelessNetworking", description, params, returns);

    params.clear(); returns.clear();
    description = "Get the current list of wireless network access points for the given interface. The interface has to be a WirelessNetworkDevice.";
    params.insert("interface", enumValueName(String));
    returns.insert("o:wirelessAccessPoints", QVariantList() << objectRef("WirelessAccessPoint"));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("GetWirelessAccessPoints", description, params, returns);

    params.clear(); returns.clear();
    description = "Disconnect the given network interface. The interface will remain disconnected until the user connect it again.";
    params.insert("interface", enumValueName(String));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("DisconnectInterface", description, params, returns);

    params.clear(); returns.clear();
    description = "Get the list of current network devices.";
    returns.insert("wiredNetworkDevices",  QVariantList() << objectRef("WiredNetworkDevice"));
    returns.insert("wirelessNetworkDevices",  QVariantList() << objectRef("WirelessNetworkDevice"));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("GetNetworkDevices", description, params, returns);

    params.clear(); returns.clear();
    description = "Start a wifi scan for searching new networks.";
    params.insert("interface", enumValueName(String));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("ScanWifiNetworks", description, params, returns);

    params.clear(); returns.clear();
    description = "Connect to the wifi network with the given ssid and password.";
    params.insert("interface", enumValueName(String));
    params.insert("ssid", enumValueName(String));
    params.insert("o:password", enumValueName(String));
    returns.insert("networkManagerError", enumRef<NetworkManager::NetworkManagerError>());
    registerMethod("ConnectWifiNetwork", description, params, returns);

    // Notifications
    params.clear(); returns.clear();
    description = "Emitted whenever a status of a NetworkManager changes.";
    params.insert("status", status);
    registerNotification("NetworkStatusChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a new WirelessNetworkDevice was added.";
    params.insert("wirelessNetworkDevice", objectRef("WirelessNetworkDevice"));
    registerNotification("WirelessNetworkDeviceAdded", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a WirelessNetworkDevice was removed.";
    params.insert("interface", enumValueName(String));
    registerNotification("WirelessNetworkDeviceRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the given WirelessNetworkDevice has changed.";
    params.insert("wirelessNetworkDevice", objectRef("WirelessNetworkDevice"));
    registerNotification("WirelessNetworkDeviceChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a new WiredNetworkDevice was added.";
    params.insert("wiredNetworkDevice", objectRef("WiredNetworkDevice"));
    registerNotification("WiredNetworkDeviceAdded", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a WiredNetworkDevice was removed.";
    params.insert("interface", enumValueName(String));
    registerNotification("WiredNetworkDeviceRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the given WiredNetworkDevice has changed.";
    params.insert("wiredNetworkDevice", objectRef("WiredNetworkDevice"));
    registerNotification("WiredNetworkDeviceChanged", description, params);

    connect(NymeaCore::instance()->networkManager(), &NetworkManager::stateChanged, this, &NetworkManagerHandler::onNetworkManagerStatusChanged);
    connect(NymeaCore::instance()->networkManager(), &NetworkManager::networkingEnabledChanged, this, &NetworkManagerHandler::onNetworkManagerStatusChanged);
    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wirelessEnabledChanged, this, &NetworkManagerHandler::onNetworkManagerStatusChanged);

    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wirelessDeviceAdded, this, &NetworkManagerHandler::onWirelessNetworkDeviceAdded);
    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wirelessDeviceRemoved, this, &NetworkManagerHandler::onWirelessNetworkDeviceRemoved);
    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wirelessDeviceChanged, this, &NetworkManagerHandler::onWirelessNetworkDeviceChanged);

    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wiredDeviceAdded, this, &NetworkManagerHandler::onWiredNetworkDeviceAdded);
    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wiredDeviceRemoved, this, &NetworkManagerHandler::onWiredNetworkDeviceRemoved);
    connect(NymeaCore::instance()->networkManager(), &NetworkManager::wiredDeviceChanged, this, &NetworkManagerHandler::onWiredNetworkDeviceChanged);
}

/*! Returns the name of the \l{NetworkManagerHandler}. In this case \b NetworkManager. */
QString NetworkManagerHandler::name() const
{
    return "NetworkManager";
}

JsonReply *NetworkManagerHandler::GetNetworkStatus(const QVariantMap &params)
{
    Q_UNUSED(params)

    // Check available
    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    // Pack network manager status
    QVariantMap returns = statusToReply(NetworkManager::NetworkManagerErrorNoError);
    returns.insert("status", packNetworkManagerStatus());
    return createReply(returns);
}

JsonReply *NetworkManagerHandler::EnableNetworking(const QVariantMap &params)
{
    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    bool enable = params.value("enable").toBool();

    if (!NymeaCore::instance()->networkManager()->enableNetworking(enable))
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorUnknownError));

    return createReply(statusToReply(NetworkManager::NetworkManagerErrorNoError));
}

JsonReply *NetworkManagerHandler::EnableWirelessNetworking(const QVariantMap &params)
{
    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    if (!NymeaCore::instance()->networkManager()->wifiAvailable())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNotAvailable));

    if (!NymeaCore::instance()->networkManager()->enableWireless(params.value("enable").toBool()))
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorUnknownError));

    return createReply(statusToReply(NetworkManager::NetworkManagerErrorNoError));
}

JsonReply *NetworkManagerHandler::GetWirelessAccessPoints(const QVariantMap &params)
{
    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    if (!NymeaCore::instance()->networkManager()->wifiAvailable())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNotAvailable));

    if (!NymeaCore::instance()->networkManager()->networkingEnabled())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkingDisabled));

    if (!NymeaCore::instance()->networkManager()->wirelessEnabled())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNetworkingDisabled));

    QString interface = params.value("interface").toString();

    if (!NymeaCore::instance()->networkManager()->getNetworkDevice(interface))
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkInterfaceNotFound));

    foreach (WirelessNetworkDevice *networkDevice, NymeaCore::instance()->networkManager()->wirelessNetworkDevices()) {
        if (networkDevice->interface() == interface) {
            QVariantList wirelessAccessPoints;
            foreach (WirelessAccessPoint *wirelessAccessPoint, networkDevice->accessPoints())
                wirelessAccessPoints.append(packWirelessAccessPoint(wirelessAccessPoint));

            QVariantMap returns = statusToReply(NetworkManager::NetworkManagerErrorNoError);
            returns.insert("wirelessAccessPoints", wirelessAccessPoints);
            return createReply(returns);

        }
    }

    return createReply(statusToReply(NetworkManager::NetworkManagerErrorInvalidNetworkDeviceType));
}

JsonReply *NetworkManagerHandler::GetNetworkDevices(const QVariantMap &params)
{
    Q_UNUSED(params);

    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    QVariantList wirelessNetworkDevices;
    foreach (WirelessNetworkDevice *networkDevice, NymeaCore::instance()->networkManager()->wirelessNetworkDevices())
        wirelessNetworkDevices.append(packWirelessNetworkDevice(networkDevice));

    QVariantList wiredNetworkDevices;
    foreach (WiredNetworkDevice *networkDevice, NymeaCore::instance()->networkManager()->wiredNetworkDevices())
        wiredNetworkDevices.append(packWiredNetworkDevice(networkDevice));

    QVariantMap returns = statusToReply(NetworkManager::NetworkManagerErrorNoError);
    returns.insert("wirelessNetworkDevices", wirelessNetworkDevices);
    returns.insert("wiredNetworkDevices", wiredNetworkDevices);
    return createReply(returns);
}

JsonReply *NetworkManagerHandler::ScanWifiNetworks(const QVariantMap &params)
{
    Q_UNUSED(params)

    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    if (!NymeaCore::instance()->networkManager()->wifiAvailable())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNotAvailable));

    if (!NymeaCore::instance()->networkManager()->networkingEnabled())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkingDisabled));

    if (!NymeaCore::instance()->networkManager()->wirelessEnabled())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNetworkingDisabled));


    QString interface = params.value("interface").toString();

    if (!NymeaCore::instance()->networkManager()->getNetworkDevice(interface))
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkInterfaceNotFound));

    foreach (WirelessNetworkDevice *networkDevice, NymeaCore::instance()->networkManager()->wirelessNetworkDevices()) {
        if (networkDevice->interface() == interface) {
            networkDevice->scanWirelessNetworks();
            return createReply(statusToReply(NetworkManager::NetworkManagerErrorNoError));
        }
    }

    return createReply(statusToReply(NetworkManager::NetworkManagerErrorInvalidNetworkDeviceType));
}

JsonReply *NetworkManagerHandler::ConnectWifiNetwork(const QVariantMap &params)
{
    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    if (!NymeaCore::instance()->networkManager()->wifiAvailable())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNotAvailable));

    if (!NymeaCore::instance()->networkManager()->networkingEnabled())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkingDisabled));

    if (!NymeaCore::instance()->networkManager()->wirelessEnabled())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorWirelessNetworkingDisabled));


    QString ssid = params.value("ssid").toString();
    QString password = params.value("password").toString();
    QString interface = params.value("interface").toString();

    return createReply(statusToReply(NymeaCore::instance()->networkManager()->connectWifi(interface, ssid, password)));
}

JsonReply *NetworkManagerHandler::DisconnectInterface(const QVariantMap &params)
{
    if (!NymeaCore::instance()->networkManager()->available())
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkManagerNotAvailable));

    QString interface = params.value("interface").toString();
    NetworkDevice *networkDevice = NymeaCore::instance()->networkManager()->getNetworkDevice(interface);
    if (!networkDevice)
        return createReply(statusToReply(NetworkManager::NetworkManagerErrorNetworkInterfaceNotFound));

    networkDevice->disconnectDevice();
    return createReply(statusToReply(NetworkManager::NetworkManagerErrorNoError));
}

QVariantMap NetworkManagerHandler::packNetworkManagerStatus()
{
    QVariantMap status;
    status.insert("networkingEnabled", NymeaCore::instance()->networkManager()->networkingEnabled());
    status.insert("wirelessNetworkingEnabled", NymeaCore::instance()->networkManager()->wirelessEnabled());
    status.insert("state", NymeaCore::instance()->networkManager()->stateString());
    return status;
}

void NetworkManagerHandler::onNetworkManagerStatusChanged()
{
    QVariantMap notification;
    notification.insert("status", packNetworkManagerStatus());
    emit NetworkStatusChanged(notification);
}

void NetworkManagerHandler::onWirelessNetworkDeviceAdded(WirelessNetworkDevice *networkDevice)
{
    QVariantMap notification;
    notification.insert("wirelessNetworkDevice", packWirelessNetworkDevice(networkDevice));
    emit WirelessNetworkDeviceAdded(notification);
}

void NetworkManagerHandler::onWirelessNetworkDeviceRemoved(const QString &interface)
{
    QVariantMap notification;
    notification.insert("interface", interface);
    emit WirelessNetworkDeviceRemoved(notification);
}

void NetworkManagerHandler::onWirelessNetworkDeviceChanged(WirelessNetworkDevice *networkDevice)
{
    QVariantMap notification;
    notification.insert("wirelessNetworkDevice", packWirelessNetworkDevice(networkDevice));
    emit WirelessNetworkDeviceChanged(notification);
}

void NetworkManagerHandler::onWiredNetworkDeviceAdded(WiredNetworkDevice *networkDevice)
{
    QVariantMap notification;
    notification.insert("wiredNetworkDevice", packWiredNetworkDevice(networkDevice));
    emit WiredNetworkDeviceAdded(notification);
}

void NetworkManagerHandler::onWiredNetworkDeviceRemoved(const QString &interface)
{
    QVariantMap notification;
    notification.insert("interface", interface);
    emit WiredNetworkDeviceRemoved(notification);
}

void NetworkManagerHandler::onWiredNetworkDeviceChanged(WiredNetworkDevice *networkDevice)
{
    QVariantMap notification;
    notification.insert("wiredNetworkDevice", packWiredNetworkDevice(networkDevice));
    emit WiredNetworkDeviceChanged(notification);
}

QVariantMap NetworkManagerHandler::packWirelessAccessPoint(WirelessAccessPoint *wirelessAccessPoint)
{
    QVariantMap wirelessAccessPointVariant;
    wirelessAccessPointVariant.insert("ssid", wirelessAccessPoint->ssid());
    wirelessAccessPointVariant.insert("macAddress", wirelessAccessPoint->macAddress());
    wirelessAccessPointVariant.insert("frequency", wirelessAccessPoint->frequency());
    wirelessAccessPointVariant.insert("signalStrength", wirelessAccessPoint->signalStrength());
    wirelessAccessPointVariant.insert("protected", wirelessAccessPoint->isProtected());
    return wirelessAccessPointVariant;
}

QVariantMap NetworkManagerHandler::packWiredNetworkDevice(WiredNetworkDevice *networkDevice)
{
    QVariantMap networkDeviceVariant;
    networkDeviceVariant.insert("interface", networkDevice->interface());
    networkDeviceVariant.insert("macAddress", networkDevice->macAddress());
    networkDeviceVariant.insert("state", networkDevice->deviceStateString());
    networkDeviceVariant.insert("bitRate", QString("%1 [Mb/s]").arg(QString::number(networkDevice->bitRate())));
    networkDeviceVariant.insert("pluggedIn", networkDevice->pluggedIn());
    return networkDeviceVariant;
}

QVariantMap NetworkManagerHandler::packWirelessNetworkDevice(WirelessNetworkDevice *networkDevice)
{
    QVariantMap networkDeviceVariant;
    networkDeviceVariant.insert("interface", networkDevice->interface());
    networkDeviceVariant.insert("macAddress", networkDevice->macAddress());
    networkDeviceVariant.insert("state", networkDevice->deviceStateString());
    networkDeviceVariant.insert("bitRate", QString("%1 [Mb/s]").arg(QString::number(networkDevice->bitRate())));
    if (networkDevice->activeAccessPoint())
        networkDeviceVariant.insert("currentAccessPoint", packWirelessAccessPoint(networkDevice->activeAccessPoint()));

    return networkDeviceVariant;
}

QVariantMap NetworkManagerHandler::statusToReply(NetworkManager::NetworkManagerError status) const
{
    QVariantMap returns;
    returns.insert("networkManagerError", enumValueName<NetworkManager::NetworkManagerError>(status));
    return returns;
}

}
