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

#include "eventhandler.h"
#include "nymeacore.h"
#include "loggingcategories.h"
#include "devicehandler.h"

namespace nymeaserver {

/*! Constructs a new \l EventHandler with the given \a parent. */
EventHandler::EventHandler(QObject *parent) :
    JsonHandler(parent)
{
    registerEnum<Types::InputType>();
    registerEnum<Types::Unit>();
    // Objects
    registerObject<Param, ParamList>();
    registerObject<Event>();
    registerObject<ParamType, ParamTypes>();
    registerObject<EventType>();

    // Methods
    QString description; QVariantMap params; QVariantMap returns;
    description = "Get the EventType for the given eventTypeId.";
    params.insert("eventTypeId", enumValueName(Uuid));
    returns.insert("deviceError", enumRef<Device::DeviceError>());
    returns.insert("o:eventType", objectRef<EventType>());
    registerMethod("GetEventType", description, params, returns, Types::PermissionScopeNone, "Please use the Devices namespace instead.");

    // Notifications
    params.clear(); returns.clear();
    description = "Emitted whenever an Event is triggered.";
    params.insert("event", objectRef<Event>());
    registerNotification("EventTriggered", description, params, "Please use Devices.EventTriggered instead.");
    connect(NymeaCore::instance(), &NymeaCore::eventTriggered, this, &EventHandler::eventTriggered);
}

/*! Returns the name of the \l{EventHandler}. In this case \b Events.*/
QString EventHandler::name() const
{
    return "Events";
}

void EventHandler::eventTriggered(const Event &event)
{
    QVariantMap params;
    params.insert("event", pack(event));
    emit EventTriggered(params);
}

JsonReply* EventHandler::GetEventType(const QVariantMap &params, const JsonContext &context) const
{
    qCDebug(dcJsonRpc) << "asked for event type" << params;
    EventTypeId eventTypeId(params.value("eventTypeId").toString());
    foreach (const ThingClass &deviceClass, NymeaCore::instance()->thingManager()->supportedThings()) {
        foreach (const EventType &eventType, deviceClass.eventTypes()) {
            if (eventType.id() == eventTypeId) {
                EventType translatedEventType = eventType;
                translatedEventType.setDisplayName(NymeaCore::instance()->thingManager()->translate(deviceClass.pluginId(), eventType.displayName(), context.locale()));
                QVariantMap data;
                data.insert("deviceError", enumValueName<Thing::ThingError>(Thing::ThingErrorNoError).replace("ThingError", "DeviceError"));
                data.insert("eventType", pack(translatedEventType));
                return createReply(data);
            }
        }
    }
    QVariantMap data;
    data.insert("deviceError", enumValueName<Thing::ThingError>(Thing::ThingErrorEventTypeNotFound).replace("ThingError", "DeviceError"));
    return createReply(data);
}

}
