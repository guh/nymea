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
    \class nymeaserver::StateHandler
    \brief This subclass of \l{JsonHandler} processes the JSON requests for the \tt States namespace of the JSON-RPC API.

    \ingroup json
    \inmodule core

    This \l{JsonHandler} will be created in the \l{JsonRPCServer} and used to handle JSON-RPC requests
    for the \tt {States} namespace of the API.

    \sa State, JsonHandler, JsonRPCServer
*/

#include "statehandler.h"
#include "nymeacore.h"
#include "loggingcategories.h"

#include "devicehandler.h"

namespace nymeaserver {

/*! Constructs a new \l{StateHandler} with the given \a parent. */
StateHandler::StateHandler(QObject *parent) :
    JsonHandler(parent)
{
    registerEnum<Types::Unit>();
    registerEnum<Types::IOType>();
    registerObject<State>();
    registerObject<StateType>();

    // Methods
    QString description; QVariantMap params; QVariantMap returns;
    description = "Get the StateType for the given stateTypeId.";
    params.insert("stateTypeId", enumValueName(Uuid));
    returns.insert("deviceError", enumRef<Device::DeviceError>());
    returns.insert("o:stateType", objectRef<StateType>());
    registerMethod("GetStateType", description, params, returns, Types::PermissionScopeNone, "Please use the Integrations namespace instead.");
}

/*! Returns the name of the \l{StateHandler}. In this case \b States.*/
QString StateHandler::name() const
{
    return "States";
}

JsonReply* StateHandler::GetStateType(const QVariantMap &params, const JsonContext &context) const
{
    qCDebug(dcJsonRpc) << "asked for state type" << params;
    StateTypeId stateTypeId(params.value("stateTypeId").toString());
    foreach (const ThingClass &deviceClass, NymeaCore::instance()->thingManager()->supportedThings()) {
        foreach (const StateType &stateType, deviceClass.stateTypes()) {
            if (stateType.id() == stateTypeId) {
                QVariantMap data;
                data.insert("deviceError", enumValueName<Thing::ThingError>(Thing::ThingErrorNoError).replace("ThingError", "DeviceError"));
                StateType translatedStateType = stateType;
                translatedStateType.setDisplayName(NymeaCore::instance()->thingManager()->translate(deviceClass.pluginId(), stateType.displayName(), context.locale()));
                data.insert("stateType", pack(translatedStateType));
                return createReply(data);
            }
        }
    }
    QVariantMap data;
    data.insert("deviceError", enumValueName<Thing::ThingError>(Thing::ThingErrorStateTypeNotFound).replace("ThingError", "DeviceError"));
    return createReply(data);
}

}
