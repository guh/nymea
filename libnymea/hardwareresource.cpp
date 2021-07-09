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
    \class HardwareResource
    \brief The base class for hardware resources.

    \inmodule libnymea

    \sa HardwareResource
*/

/*! \fn HardwareResource::~HardwareResource();
    The virtual destructor of the HardwareResource.
*/

/*! \fn bool HardwareResource::available() const;
    Returns true if the hardware resource is available.

    \sa availableChanged()
*/

/*! \fn bool HardwareResource::enabled() const;
    Returns true if the hardware resource is enabled.

    \sa enabledChanged()
*/


/*! \fn bool HardwareResource::setEnabled(bool enabled);
    Sets the hardware resource to \a enabled.

    \sa enabledChanged()
*/

// Signals
/*! \fn bool HardwareResource::enabledChanged(bool enabled);
    This signal will be emitted if the hardware resource was \a enabled or disabled.
*/

/*! \fn bool HardwareResource::availableChanged(bool available);
    This signal will be emitted if the hardware resource \a available changed.
*/

#include "hardwareresource.h"
#include "hardwaremanager.h"
#include "loggingcategories.h"
#include "nymeadbusservice.h"

/*! Constructs a new HardwareResource with the given \a name and \a parent. */
HardwareResource::HardwareResource(const QString &name, QObject *parent) :
    QObject(parent),
    m_name(name)
{
    new NymeaDBusService("/io/guh/nymead/HardwareManager/" + name, this);
}

/*! Returns the name of this resource. */
QString HardwareResource::name() const
{
    return m_name;
}
