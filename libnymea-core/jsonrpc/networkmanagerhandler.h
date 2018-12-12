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

#ifndef NETWORKMANAGERHANDLER_H
#define NETWORKMANAGERHANDLER_H

#include <QObject>

#include "jsonhandler.h"

namespace nymeaserver {

class NetworkManagerHandler : public JsonHandler
{
    Q_OBJECT
public:
    explicit NetworkManagerHandler(QObject *parent = nullptr);

    QString name() const;

    Q_INVOKABLE JsonReply *GetNetworkStatus(const QVariantMap &params);
    Q_INVOKABLE JsonReply *EnableNetworking(const QVariantMap &params);
    Q_INVOKABLE JsonReply *EnableWirelessNetworking(const QVariantMap &params);
    Q_INVOKABLE JsonReply *GetWirelessAccessPoints(const QVariantMap &params);
    Q_INVOKABLE JsonReply *GetNetworkDevices(const QVariantMap &params);
    Q_INVOKABLE JsonReply *ScanWifiNetworks(const QVariantMap &params);
    Q_INVOKABLE JsonReply *ConnectWifiNetwork(const QVariantMap &params);
    Q_INVOKABLE JsonReply *DisconnectInterface(const QVariantMap &params);

private:
    QVariantMap packNetworkManagerStatus();

signals:
    // NetworkManager
    void NetworkStatusChanged(const QVariantMap &params);

    // NetworkDevices
    void WiredNetworkDeviceAdded(const QVariantMap &params);
    void WiredNetworkDeviceRemoved(const QVariantMap &params);
    void WiredNetworkDeviceChanged(const QVariantMap &params);

    void WirelessNetworkDeviceAdded(const QVariantMap &params);
    void WirelessNetworkDeviceRemoved(const QVariantMap &params);
    void WirelessNetworkDeviceChanged(const QVariantMap &params);

private slots:
    // NetworkManager
    void onNetworkManagerStatusChanged();

    // NetworkDevices
    void onWirelessNetworkDeviceAdded(WirelessNetworkDevice *networkDevice);
    void onWirelessNetworkDeviceRemoved(const QString &interface);
    void onWirelessNetworkDeviceChanged(WirelessNetworkDevice *networkDevice);

    void onWiredNetworkDeviceAdded(WiredNetworkDevice *networkDevice);
    void onWiredNetworkDeviceRemoved(const QString &interface);
    void onWiredNetworkDeviceChanged(WiredNetworkDevice *networkDevice);

};

}

#endif // NETWORKMANAGERHANDLER_H
