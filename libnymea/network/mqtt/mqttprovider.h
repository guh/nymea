/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@nymea.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MQTTPROVIDER_H
#define MQTTPROVIDER_H

#include <QObject>
#include <QHostAddress>

#include "typeutils.h"
#include "hardwareresource.h"
#include "nymea-mqtt/mqttclient.h"

class MqttChannel;

class MqttProvider : public HardwareResource
{
    Q_OBJECT
public:
    explicit MqttProvider(QObject *parent = nullptr);

    virtual MqttChannel* createChannel(const DeviceId &deviceId, const QHostAddress &clientAddress) = 0;
    virtual void releaseChannel(MqttChannel *channel) = 0;

    virtual MqttClient* createInternalClient(const DeviceId &deviceId) = 0;
};

#endif // MQTTPROVIDER_H
