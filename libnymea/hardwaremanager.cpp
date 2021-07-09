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
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \class HardwareManager
    \brief The main entry point when interacting with \l{HardwareResource}{hardware resources}

    \inmodule libnymea

    \sa HardwareResource
*/

/*! \fn HardwareManager::~HardwareManager();
    The virtual destructor of the HardwareManager.
*/

/*! \fn Radio433 * HardwareManager::radio433();
    Returns the Radio433 \l{HardwareResource}.
*/

/*! \fn PluginTimerManager * HardwareManager::pluginTimerManager();
    Returns the PluginTimerManager \l{HardwareResource}.
*/

/*! \fn NetworkAccessManager * HardwareManager::networkManager();
    Returns the NetworkAccessManager \l{HardwareResource}.
*/

/*! \fn UpnpDiscovery * HardwareManager::upnpDiscovery();
    Returns the UpnpDiscovery \l{HardwareResource}.
*/

/*! \fn QtAvahiServiceBrowser * HardwareManager::avahiBrowser();
    Returns the QtAvahiServiceBrowser \l{HardwareResource}.
*/

/*! \fn BluetoothLowEnergyManager * HardwareManager::bluetoothLowEnergyManager();
    Returns the BluetoothLowEnergyManager \l{HardwareResource}.
*/

/*! \fn MqttProvider* HardwareManager::mqttProvider();
    Returns the MqttProvider \l{HardwareResource}.
*/

/*! \fn ZigbeeHardwareResource *HardwareManager::zigbeeResource();
    Returns the Zigbee \l{HardwareResource}.
*/


#include "hardwaremanager.h"
#include "hardwareresource.h"

/*! Constructs a new HardwareManager with the given \a parent.*/
HardwareManager::HardwareManager(QObject *parent) :
    QObject(parent)
{
}

/*! Sets the given \a resource to \a enabled. This allows to enable/disable individual \l{HardwareResource}{HardwareResources}. */
void HardwareManager::setResourceEnabled(HardwareResource *resource, bool enabled)
{
    resource->setEnabled(enabled);
}
