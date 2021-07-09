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

#include "integrationshandler.h"
#include "nymeacore.h"
#include "integrations/thingmanager.h"
#include "integrations/thing.h"
#include "integrations/integrationplugin.h"
#include "loggingcategories.h"
#include "types/thingclass.h"
#include "types/browseritem.h"
#include "types/mediabrowseritem.h"
#include "integrations/translator.h"
#include "integrations/thingdiscoveryinfo.h"
#include "integrations/thingpairinginfo.h"
#include "integrations/thingsetupinfo.h"
#include "integrations/browseresult.h"
#include "integrations/browseritemresult.h"

#include <QDebug>
#include <QJsonDocument>

namespace nymeaserver {

IntegrationsHandler::IntegrationsHandler(ThingManager *thingManager, QObject *parent) :
    JsonHandler(parent),
    m_thingManager(thingManager)
{
    // Enums
    registerEnum<Thing::ThingError>();
    registerEnum<Thing::ThingSetupStatus>();
    registerEnum<ThingClass::SetupMethod>();
    registerEnum<ThingClass::CreateMethod, ThingClass::CreateMethods>();
    registerEnum<Types::Unit>();
    registerEnum<Types::InputType>();
    registerEnum<Types::IOType>();
    registerEnum<Types::StateValueFilter>();
    registerEnum<RuleEngine::RemovePolicy>();
    registerEnum<BrowserItem::BrowserIcon>();
    registerEnum<MediaBrowserItem::MediaBrowserIcon>();

    // Objects
    registerObject<ParamType, ParamTypes>();
    registerObject<Param, ParamList>();
    registerUncreatableObject<IntegrationPlugin, IntegrationPlugins>();
    registerObject<Vendor, Vendors>();
    registerObject<EventType, EventTypes>();
    registerObject<StateType, StateTypes>();
    registerObject<ActionType, ActionTypes>();
    registerObject<ThingClass, ThingClasses>();
    registerObject<ThingDescriptor, ThingDescriptors>();
    registerObject<Event>();
    registerObject<Action>();
    registerObject<State, States>();
    registerUncreatableObject<Thing, Things>();
    registerObject<IOConnection, IOConnections>();

    // Registering browseritem manually for now. Not sure how to deal with the
    // polymorphism in it (e.g MediaBrowserItem)
    QVariantMap browserItem;
    browserItem.insert("id", enumValueName(String));
    browserItem.insert("displayName", enumValueName(String));
    browserItem.insert("description", enumValueName(String));
    browserItem.insert("icon", enumRef<BrowserItem::BrowserIcon>());
    browserItem.insert("thumbnail", enumValueName(String));
    browserItem.insert("executable", enumValueName(Bool));
    browserItem.insert("browsable", enumValueName(Bool));
    browserItem.insert("disabled", enumValueName(Bool));
    browserItem.insert("actionTypeIds", QVariantList() << enumValueName(Uuid));
    browserItem.insert("o:mediaIcon", enumRef<MediaBrowserItem::MediaBrowserIcon>());
    registerObject("BrowserItem", browserItem);


    // Methods
    QString description; QVariantMap returns; QVariantMap params;
    description = "Returns a list of supported Vendors.";
    returns.insert("vendors", objectRef<Vendors>());
    registerMethod("GetVendors", description, params, returns);

    params.clear(); returns.clear();
    description = "Returns a list of supported thing classes, optionally filtered by vendorId or by a list of thing class ids.";
    params.insert("o:vendorId", enumValueName(Uuid));
    params.insert("o:thingClassIds", QVariantList() << enumValueName(Uuid));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:thingClasses", objectRef<ThingClasses>());
    registerMethod("GetThingClasses", description, params, returns);

    params.clear(); returns.clear();
    description = "Returns a list of loaded plugins.";
    returns.insert("plugins", objectRef<IntegrationPlugins>());
    registerMethod("GetPlugins", description, params, returns);

    params.clear(); returns.clear();
    description = "Get a plugin's params.";
    params.insert("pluginId", enumValueName(Uuid));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:configuration", objectRef<ParamList>());
    registerMethod("GetPluginConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Set a plugin's params.";
    params.insert("pluginId", enumValueName(Uuid));
    params.insert("configuration", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("SetPluginConfiguration", description, params, returns);

    params.clear(); returns.clear();
    description = "Add a new thing to the system. "
                    "Only things with a setupMethod of SetupMethodJustAdd can be added this way. "
                    "For things with a setupMethod different than SetupMethodJustAdd, use PairThing. "
                    "Things with CreateMethodJustAdd require all parameters to be supplied here. "
                    "Things with CreateMethodDiscovery require the use of a thingDescriptorId. For discovered "
                    "things, params are not required and will be taken from the ThingDescriptor, however, they "
                    "may be overridden by supplying thingParams.";
    params.insert("o:thingClassId", enumValueName(Uuid));
    params.insert("name", enumValueName(String));
    params.insert("o:thingDescriptorId", enumValueName(Uuid));
    params.insert("o:thingParams", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:thingId", enumValueName(Uuid));
    returns.insert("o:displayMessage", enumValueName(String));
    registerMethod("AddThing", description, params, returns);

    params.clear(); returns.clear();
    description = "Pair a new thing. "
                    "Use this to set up or reconfigure things for ThingClasses with a setupMethod different than SetupMethodJustAdd. "
                    "Depending on the CreateMethod and whether a new thing is set up or an existing one is reconfigured, different parameters "
                    "are required:\n"
                    "CreateMethodJustAdd takes the thingClassId and the parameters you want to have with that thing. "
                    "If an existing thing should be reconfigured, the thingId of said thing should be given additionally.\n"
                    "CreateMethodDiscovery requires the use of a thingDescriptorId, previously obtained with DiscoverThings. Optionally, "
                    "parameters can be overridden with the give thingParams. ThingDescriptors containing a thingId will reconfigure an "
                    "existing thing, descriptors without a thingId will add a new thing to the system.\n"
                    "If success is true, the return values will contain a pairingTransactionId, a displayMessage and "
                    "the setupMethod. Depending on the setupMethod, the application should present the use an appropriate login mask, "
                    "that is, For SetupMethodDisplayPin the user should enter a pin that is displayed on the device or online service, for SetupMethodEnterPin the "
                    "application should present the given PIN so the user can enter it on the device or online service. For SetupMethodPushButton, the displayMessage "
                    "shall be presented to the user as informational hints to press a button on the device. For SetupMethodUserAndPassword a login "
                    "mask for a user and password login should be presented to the user. In case of SetupMethodOAuth, an OAuth URL will be returned "
                    "which shall be opened in a web view to allow the user logging in.\n"
                    "Once the login procedure has completed, the application shall proceed with ConfirmPairing, providing the results of the pairing "
                    "procedure.";
    params.insert("o:thingClassId", enumValueName(Uuid));
    params.insert("o:name", enumValueName(String));
    params.insert("o:thingDescriptorId", enumValueName(Uuid));
    params.insert("o:thingParams", objectRef<ParamList>());
    params.insert("o:thingId", enumValueName(Uuid));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:setupMethod", enumRef<ThingClass::SetupMethod>());
    returns.insert("o:pairingTransactionId", enumValueName(Uuid));
    returns.insert("o:displayMessage", enumValueName(String));
    returns.insert("o:oAuthUrl", enumValueName(String));
    returns.insert("o:pin", enumValueName(String));
    registerMethod("PairThing", description, params, returns);

    params.clear(); returns.clear();
    description = "Confirm an ongoing pairing. For SetupMethodUserAndPassword, provide the username in the \"username\" field "
                                     "and the password in the \"secret\" field. For SetupMethodEnterPin and provide the PIN in the \"secret\" "
                                     "field. In case of SetupMethodOAuth, the previously opened web view will eventually be redirected to http://128.0.0.1:8888 "
                                     "and the OAuth code as query parameters to this url. Provide the entire unmodified URL in the secret field.";
    params.insert("pairingTransactionId", enumValueName(Uuid));
    params.insert("o:username", enumValueName(String));
    params.insert("o:secret", enumValueName(String));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    returns.insert("o:thingId", enumValueName(Uuid));
    registerMethod("ConfirmPairing", description, params, returns);

    params.clear(); returns.clear();
    description = "Returns a list of configured things, optionally filtered by thingId.";
    params.insert("o:thingId", enumValueName(Uuid));
    returns.insert("o:things", objectRef<Things>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("GetThings", description, params, returns);

    params.clear(); returns.clear();
    description = "Performs a thing discovery for things of the given thingClassId and returns the results. "
                    "This function may take a while to return. Note that this method will include all the found "
                    "things, that is, including things that may already have been added. Those things will have "
                    "thingId set to the id of the already added thing. Such results may be used to reconfigure "
                    "existing things and might be filtered in cases where only unknown things are of interest.";
    params.insert("thingClassId", enumValueName(Uuid));
    params.insert("o:discoveryParams", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    returns.insert("o:thingDescriptors", objectRef<ThingDescriptors>());
    registerMethod("DiscoverThings", description, params, returns);

    params.clear(); returns.clear();
    description = "Reconfigure a thing. This comes down to removing and recreating a thing with new parameters "
                  "but keeping its thing id the same (and with that keeping rules, tags etc). For things with "
                  "create method CreateMethodDiscovery, a discovery (DiscoverThings) shall be performed first "
                  "and this method is to be called with a thingDescriptorId of the re-discovered thing instead of "
                  "the thingId directly. Thing parameters will be taken from the discovery, but can be overridden "
                  "individually here by providing them in the thingParams parameter. Only writable parameters can "
                  "be changed.";
    params.insert("o:thingId", enumValueName(Uuid));
    params.insert("o:thingDescriptorId", enumValueName(Uuid));
    params.insert("o:thingParams", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    registerMethod("ReconfigureThing", description, params, returns);

    params.clear(); returns.clear();
    description = "Edit the name of a thing.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("name", enumValueName(String));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("EditThing", description, params, returns);

    params.clear(); returns.clear();
    description = "Change the settings of a thing.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("settings", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("SetThingSettings", description, params, returns);

    params.clear(); returns.clear();
    description = "Enable/disable logging for the given event type on the given thing.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("eventTypeId", enumValueName(Uuid));
    params.insert("enabled", enumValueName(Bool));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("SetEventLogging", description, params, returns);

    params.clear(); returns.clear();
    description = "Set the filter for the given state on the given thing.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("stateTypeId", enumValueName(Uuid));
    params.insert("filter", enumRef<Types::StateValueFilter>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("SetStateFilter", description, params, returns);

    params.clear(); returns.clear();
    description = "Remove a thing from the system.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("o:removePolicy", enumRef<RuleEngine::RemovePolicy>());
    QVariantMap policy;
    policy.insert("ruleId", enumValueName(Uuid));
    policy.insert("policy", enumRef<RuleEngine::RemovePolicy>());
    QVariantList removePolicyList;
    removePolicyList.append(policy);
    params.insert("o:removePolicyList", removePolicyList);
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:ruleIds", QVariantList() << enumValueName(Uuid));
    registerMethod("RemoveThing", description, params, returns);

    params.clear(); returns.clear();
    description = "Get event types for a specified thingClassId.";
    params.insert("thingClassId", enumValueName(Uuid));
    returns.insert("eventTypes", objectRef<EventTypes>());
    registerMethod("GetEventTypes", description, params, returns);

    params.clear(); returns.clear();
    description = "Get action types for a specified thingClassId.";
    params.insert("thingClassId", enumValueName(Uuid));
    returns.insert("actionTypes", objectRef<ActionTypes>());
    registerMethod("GetActionTypes", description, params, returns);

    params.clear(); returns.clear();
    description = "Get state types for a specified thingClassId.";
    params.insert("thingClassId", enumValueName(Uuid));
    returns.insert("stateTypes", objectRef<StateTypes>());
    registerMethod("GetStateTypes", description, params, returns);

    params.clear(); returns.clear();
    description = "Get the value of the given thing and the given stateType";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("stateTypeId", enumValueName(Uuid));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:value", enumValueName(Variant));
    registerMethod("GetStateValue", description, params, returns);

    params.clear(); returns.clear();
    description = "Get all the state values of the given thing.";
    params.insert("thingId", enumValueName(Uuid));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:values", objectRef<States>());
    registerMethod("GetStateValues", description, params, returns);

    params.clear(); returns.clear();
    description = "Browse a thing. "
                    "If a ThingClass indicates a thing is browsable, this method will return the BrowserItems. If no "
                    "parameter besides the thingId is used, the root node of this thingwill be returned. Any "
                    "returned item which is browsable can be passed as node. Results will be children of the given node.\n"
                    "In case of an error during browsing, the error will be indicated and the displayMessage may contain "
                    "additional information for the user. The displayMessage will be translated. A client UI showing this "
                    "message to the user should be prepared for empty, but also longer strings.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("o:itemId", enumValueName(String));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    returns.insert("items", QVariantList() << objectRef("BrowserItem"));
    registerMethod("BrowseThing", description, params, returns);

    params.clear(); returns.clear();
    description = "Get a single item from the browser. "
                    "This won't give any more info on an item than a regular BrowseThing call, but it allows to fetch "
                    "details of an item if only the ID is known.\n"
                    "In case of an error during browsing, the error will be indicated and the displayMessage may contain "
                    "additional information for the user. The displayMessage will be translated. A client UI showing this "
                    "message to the user should be prepared for empty, but also longer strings.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("o:itemId", enumValueName(String));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    returns.insert("o:item", objectRef("BrowserItem"));
    registerMethod("GetBrowserItem", description, params, returns);

    params.clear(); returns.clear();
    description = "Execute a single action.";
    params.insert("actionTypeId", enumValueName(Uuid));
    params.insert("thingId", enumValueName(Uuid));
    params.insert("o:params", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    registerMethod("ExecuteAction", description, params, returns);

    params.clear(); returns.clear();
    description = "Execute the item identified by itemId on the given thing.\n"
                  "In case of an error during execution, the error will be indicated and the displayMessage may contain "
                  "additional information for the user. The displayMessage will be translated. A client UI showing this "
                  "message to the user should be prepared for empty, but also longer strings.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("itemId", enumValueName(String));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    registerMethod("ExecuteBrowserItem", description, params, returns);

    params.clear(); returns.clear();
    description = "Execute the action for the browser item identified by actionTypeId and the itemId on the given thing.\n"
                  "In case of an error during execution, the error will be indicated and the displayMessage may contain "
                  "additional information for the user. The displayMessage will be translated. A client UI showing this "
                  "message to the user should be prepared for empty, but also longer strings.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("itemId", enumValueName(String));
    params.insert("actionTypeId", enumValueName(Uuid));
    params.insert("o:params", objectRef<ParamList>());
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:displayMessage", enumValueName(String));
    registerMethod("ExecuteBrowserItemAction", description, params, returns);

    params.clear(); returns.clear();
    description = "Fetch IO connections. Optionally filtered by thingId and stateTypeId.";
    params.insert("o:thingId", enumValueName(Uuid));
    returns.insert("ioConnections", objectRef<IOConnections>());
    registerMethod("GetIOConnections", description, params, returns);

    params.clear(); returns.clear();
    description = "Connect two generic IO states. Input and output need to be compatible, that is, either a digital input "
                  "and a digital output, or an analog input and an analog output. If successful, the connectionId will be returned.";
    params.insert("inputThingId", enumValueName(Uuid));
    params.insert("inputStateTypeId", enumValueName(Uuid));
    params.insert("outputThingId", enumValueName(Uuid));
    params.insert("outputStateTypeId", enumValueName(Uuid));
    params.insert("o:inverted", enumValueName(Bool));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    returns.insert("o:ioConnectionId", enumValueName(Uuid));
    registerMethod("ConnectIO", description, params, returns);

    params.clear(); returns.clear();
    description = "Disconnect an existing IO connection.";
    params.insert("ioConnectionId", enumValueName(Uuid));
    returns.insert("thingError", enumRef<Thing::ThingError>());
    registerMethod("DisconnectIO", description, params, returns);


    // Notifications
    params.clear(); returns.clear();
    description = "Emitted whenever a state of a thing changes.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("stateTypeId", enumValueName(Uuid));
    params.insert("value", enumValueName(Variant));
    registerNotification("StateChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a thing was removed.";
    params.insert("thingId", enumValueName(Uuid));
    registerNotification("ThingRemoved", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a thing was added.";
    params.insert("thing", objectRef<Thing>());
    registerNotification("ThingAdded", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the params or name of a thing are changed (by EditThing or ReconfigureThing).";
    params.insert("thing", objectRef<Thing>());
    registerNotification("ThingChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever the setting of a thing is changed.";
    params.insert("thingId", enumValueName(Uuid));
    params.insert("paramTypeId", enumValueName(Uuid));
    params.insert("value", enumValueName(Variant));
    registerNotification("ThingSettingChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever a plugin's configuration is changed.";
    params.insert("pluginId", enumValueName(Uuid));
    params.insert("configuration", objectRef<ParamList>());
    registerNotification("PluginConfigurationChanged", description, params);

    params.clear(); returns.clear();
    description = "Emitted whenever an Event is triggered.";
    params.insert("event", objectRef<Event>());
    registerNotification("EventTriggered", description, params);
    connect(NymeaCore::instance(), &NymeaCore::eventTriggered, this, [this](const Event &event){
        QVariantMap params;
        params.insert("event", pack(event));
        emit EventTriggered(params);
    });

    params.clear(); returns.clear();
    description = "Emitted whenever an IO connection has been added.";
    params.insert("ioConnection", objectRef<IOConnection>());
    registerNotification("IOConnectionAdded", description, params);
    connect(m_thingManager, &ThingManager::ioConnectionAdded, this, [this](const IOConnection &connection) {
        QVariantMap params;
        params.insert("ioConnection", pack(connection));
        emit IOConnectionAdded(params);
    });

    params.clear(); returns.clear();
    description = "Emitted whenever an IO connection has been removed.";
    params.insert("ioConnectionId", enumValueName(Uuid));
    registerNotification("IOConnectionRemoved", description, params);
    connect(m_thingManager, &ThingManager::ioConnectionRemoved, this, [this](const IOConnectionId &ioConnectionId) {
        QVariantMap params;
        params.insert("ioConnectionId", ioConnectionId);
        emit IOConnectionRemoved(params);
    });

    connect(NymeaCore::instance(), &NymeaCore::pluginConfigChanged, this, &IntegrationsHandler::pluginConfigChanged);
    connect(NymeaCore::instance(), &NymeaCore::thingStateChanged, this, &IntegrationsHandler::thingStateChanged);
    connect(NymeaCore::instance(), &NymeaCore::thingRemoved, this, &IntegrationsHandler::thingRemovedNotification);
    connect(NymeaCore::instance(), &NymeaCore::thingAdded, this, &IntegrationsHandler::thingAddedNotification);
    connect(NymeaCore::instance(), &NymeaCore::thingChanged, this, &IntegrationsHandler::thingChangedNotification);
    connect(NymeaCore::instance(), &NymeaCore::thingSettingChanged, this, &IntegrationsHandler::thingSettingChangedNotification);

    connect(NymeaCore::instance(), &NymeaCore::initialized, this, [=](){
        // Generating cache hashes.
        // NOTE: We need to sort the lists to get a stable result
        QHash<ThingClassId, ThingClass> thingClassesMap;
        foreach (const ThingClass &tc, m_thingManager->supportedThings()) {
            thingClassesMap.insert(tc.id(), tc);
        }
        QList<ThingClassId> thingClassIds = thingClassesMap.keys();
        std::sort(thingClassIds.begin(), thingClassIds.end());
        ThingClasses thingClasses;
        foreach (const ThingClassId &id, thingClassIds) {
            thingClasses.append(thingClassesMap.value(id));
        }
        QByteArray hash = QCryptographicHash::hash(QJsonDocument::fromVariant(pack(thingClasses)).toJson(), QCryptographicHash::Md5).toHex();
        m_cacheHashes.insert("GetThingClasses", hash);

        QHash<VendorId, Vendor> vendorsMap;
        foreach (const Vendor &v, m_thingManager->supportedVendors()) {
            vendorsMap.insert(v.id(), v);
        }
        QList<VendorId> vendorIds = vendorsMap.keys();
        std::sort(vendorIds.begin(), vendorIds.end());
        Vendors vendors;
        foreach (const VendorId &id, vendorIds) {
            vendors.append(vendorsMap.value(id));
        }
        hash = QCryptographicHash::hash(QJsonDocument::fromVariant(pack(vendors)).toJson(), QCryptographicHash::Md5).toHex();
        m_cacheHashes.insert("GetVendors", hash);
    });
}

QString IntegrationsHandler::name() const
{
    return "Integrations";
}

QHash<QString, QString> IntegrationsHandler::cacheHashes() const
{
    return m_cacheHashes;
}

JsonReply* IntegrationsHandler::GetVendors(const QVariantMap &params, const JsonContext &context) const
{
    Q_UNUSED(params)
    QVariantList vendors;
    foreach (const Vendor &vendor, NymeaCore::instance()->thingManager()->supportedVendors()) {
        Vendor translatedVendor = NymeaCore::instance()->thingManager()->translateVendor(vendor, context.locale());
        vendors.append(pack(translatedVendor));
    }

    QVariantMap returns;
    returns.insert("vendors", vendors);
    return createReply(returns);
}

JsonReply* IntegrationsHandler::GetThingClasses(const QVariantMap &params, const JsonContext &context) const
{
    QVariantMap returns;
    QVariantList thingClasses;

    foreach (const ThingClass &thingClass, NymeaCore::instance()->thingManager()->supportedThings()) {
        if (params.contains("vendorId") && thingClass.vendorId() != VendorId(params.value("vendorId").toUuid())) {
            continue;
        }
        if (params.contains("thingClassIds")) {
            bool found = false;
            foreach (const QString &tcString, params.value("thingClassIds").toStringList()) {
                if (ThingClassId(tcString) == thingClass.id()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                continue;
            }
        }

        ThingClass translatedThingClass = NymeaCore::instance()->thingManager()->translateThingClass(thingClass, context.locale());
        thingClasses.append(pack(translatedThingClass));
    }

    returns.insert("thingError", enumValueName(Thing::ThingErrorNoError));
    returns.insert("thingClasses", thingClasses);
    return createReply(returns);
}

JsonReply *IntegrationsHandler::DiscoverThings(const QVariantMap &params, const JsonContext &context) const
{
    QLocale locale = context.locale();

    QVariantMap returns;

    ThingClassId thingClassId = ThingClassId(params.value("thingClassId").toString());
    ParamList discoveryParams = unpack<ParamList>(params.value("discoveryParams"));

    JsonReply *reply = createAsyncReply("DiscoverThings");
    ThingDiscoveryInfo *info = NymeaCore::instance()->thingManager()->discoverThings(thingClassId, discoveryParams);
    connect(info, &ThingDiscoveryInfo::finished, reply, [this, reply, info, locale](){
        QVariantMap returns;
        returns.insert("thingError", enumValueName<Thing::ThingError>(info->status()));

        if (info->status() == Thing::ThingErrorNoError) {
            QVariantList thingDescriptorList;
            foreach (const ThingDescriptor &thingDescriptor, info->thingDescriptors()) {
                thingDescriptorList.append(pack(thingDescriptor));
            }
            returns.insert("thingDescriptors", thingDescriptorList);
        }

        if (!info->displayMessage().isEmpty()) {
            returns.insert("displayMessage", info->translatedDisplayMessage(locale));
        }

        reply->setData(returns);
        reply->finished();

    });
    return reply;
}

JsonReply* IntegrationsHandler::GetPlugins(const QVariantMap &params, const JsonContext &context) const
{
    Q_UNUSED(params)
    QVariantList plugins;
    foreach (IntegrationPlugin* plugin, NymeaCore::instance()->thingManager()->plugins()) {
        QVariantMap packedPlugin = pack(*plugin).toMap();
        packedPlugin["displayName"] = NymeaCore::instance()->thingManager()->translate(plugin->pluginId(), plugin->pluginDisplayName(), context.locale());
        plugins.append(packedPlugin);
    }

    QVariantMap returns;
    returns.insert("plugins", plugins);
    return createReply(returns);
}

JsonReply *IntegrationsHandler::GetPluginConfiguration(const QVariantMap &params) const
{
    QVariantMap returns;

    IntegrationPlugin *plugin = NymeaCore::instance()->thingManager()->plugins().findById(PluginId(params.value("pluginId").toString()));
    if (!plugin) {
        returns.insert("thingError", enumValueName<Thing::ThingError>(Thing::ThingErrorPluginNotFound));
        return createReply(returns);
    }

    QVariantList paramVariantList;
    foreach (const Param &param, plugin->configuration()) {
        paramVariantList.append(pack(param));
    }
    returns.insert("configuration", paramVariantList);
    returns.insert("thingError", enumValueName<Thing::ThingError>(Thing::ThingErrorNoError));
    return createReply(returns);
}

JsonReply* IntegrationsHandler::SetPluginConfiguration(const QVariantMap &params)
{
    QVariantMap returns;
    PluginId pluginId = PluginId(params.value("pluginId").toString());
    ParamList pluginParams = unpack<ParamList>(params.value("configuration"));
    Thing::ThingError result = NymeaCore::instance()->thingManager()->setPluginConfig(pluginId, pluginParams);
    returns.insert("thingError",enumValueName<Thing::ThingError>(result));
    return createReply(returns);
}

JsonReply* IntegrationsHandler::AddThing(const QVariantMap &params, const JsonContext &context)
{
    ThingClassId thingClassId(params.value("thingClassId").toString());
    QString thingName = params.value("name").toString();
    ParamList thingParams = unpack<ParamList>(params.value("thingParams"));
    ThingDescriptorId thingDescriptorId(params.value("thingDescriptorId").toString());
    QLocale locale = context.locale();

    JsonReply *jsonReply = createAsyncReply("AddThing");

    ThingSetupInfo *info;
    if (thingDescriptorId.isNull()) {
        if (thingClassId.isNull()) {
            qCWarning(dcJsonRpc()) << "Either thingClassId or thingDescriptorId is required.";
            QVariantMap returns;
            returns.insert("thingError", enumValueName<Thing::ThingError>(Thing::ThingErrorMissingParameter));
            jsonReply->setData(returns);
            jsonReply->finished();
            return jsonReply;
        }
        info = NymeaCore::instance()->thingManager()->addConfiguredThing(thingClassId, thingParams, thingName);

    } else {
        info = NymeaCore::instance()->thingManager()->addConfiguredThing(thingDescriptorId, thingParams, thingName);
    }
    connect(info, &ThingSetupInfo::finished, jsonReply, [info, jsonReply, locale](){
        QVariantMap returns;
        returns.insert("thingError", enumValueName<Thing::ThingError>(info->status()));

        if (!info->displayMessage().isEmpty()) {
            returns.insert("displayMessage", info->translatedDisplayMessage(locale));
        }

        if(info->status() == Thing::ThingErrorNoError) {
            returns.insert("thingId", info->thing()->id());
        }
        jsonReply->setData(returns);
        jsonReply->finished();

    });
    return jsonReply;
}

JsonReply *IntegrationsHandler::PairThing(const QVariantMap &params, const JsonContext &context)
{
    QString thingName = params.value("name").toString();
    ParamList thingParams = unpack<ParamList>(params.value("thingParams"));
    QLocale locale = context.locale();

    ThingPairingInfo *info;
    if (params.contains("thingDescriptorId")) {
        ThingDescriptorId thingDescriptorId = ThingDescriptorId(params.value("thingDescriptorId").toString());
        info = NymeaCore::instance()->thingManager()->pairThing(thingDescriptorId, thingParams, thingName);
    } else if (params.contains("thingId")) {
        ThingId thingId = ThingId(params.value("thingId").toString());
        info = NymeaCore::instance()->thingManager()->pairThing(thingId, thingParams, thingName);
    } else {
        ThingClassId thingClassId(params.value("thingClassId").toString());
        info = NymeaCore::instance()->thingManager()->pairThing(thingClassId, thingParams, thingName);
    }

    JsonReply *jsonReply = createAsyncReply("PairThing");

    connect(info, &ThingPairingInfo::finished, jsonReply, [jsonReply, info, locale](){
        QVariantMap returns;
        returns.insert("thingError", enumValueName<Thing::ThingError>(info->status()));
        returns.insert("pairingTransactionId", info->transactionId().toString());

        if (info->status() == Thing::ThingErrorNoError) {
            ThingClass thingClass = NymeaCore::instance()->thingManager()->findThingClass(info->thingClassId());
            returns.insert("setupMethod", enumValueName<ThingClass::SetupMethod>(thingClass.setupMethod()));
        }

        if (!info->displayMessage().isEmpty()) {
            returns.insert("displayMessage", info->translatedDisplayMessage(locale));
        }

        if (!info->oAuthUrl().isEmpty()) {
            returns.insert("oAuthUrl", info->oAuthUrl());
        }

        jsonReply->setData(returns);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::ConfirmPairing(const QVariantMap &params)
{
    PairingTransactionId pairingTransactionId = PairingTransactionId(params.value("pairingTransactionId").toString());
    QString secret = params.value("secret").toString();
    QString username = params.value("username").toString();
    QLocale locale = params.value("locale").toLocale();

    JsonReply *jsonReply = createAsyncReply("ConfirmPairing");

    ThingPairingInfo *info = NymeaCore::instance()->thingManager()->confirmPairing(pairingTransactionId, username, secret);
    connect(info, &ThingPairingInfo::finished, jsonReply, [info, jsonReply, locale](){

        QVariantMap returns;
        returns.insert("thingError", enumValueName<Thing::ThingError>(info->status()));
        if (!info->displayMessage().isEmpty()) {
            returns.insert("displayMessage", info->translatedDisplayMessage(locale));
        }
        if (info->status() == Thing::ThingErrorNoError) {
            returns.insert("thingId", info->thingId().toString());
        }
        jsonReply->setData(returns);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply* IntegrationsHandler::GetThings(const QVariantMap &params, const JsonContext &context) const
{
    QVariantMap returns;
    QVariantList things;
    if (params.contains("thingId")) {
        Thing *thing = NymeaCore::instance()->thingManager()->findConfiguredThing(ThingId(params.value("thingId").toString()));
        if (!thing) {
            returns.insert("thingError", enumValueName<Thing::ThingError>(Thing::ThingErrorThingNotFound));
            return createReply(returns);
        } else {
            QVariantMap packedThing = pack(thing).toMap();
            QString translatedSetupStatus = NymeaCore::instance()->thingManager()->translate(thing->pluginId(), thing->setupDisplayMessage(), context.locale());
            if (!translatedSetupStatus.isEmpty()) {
                packedThing["setupDisplayMessage"] = translatedSetupStatus;
            }
            things.append(packedThing);
        }
    } else {
        foreach (Thing *thing, NymeaCore::instance()->thingManager()->configuredThings()) {
            QVariantMap packedThing = pack(thing).toMap();
            QString translatedSetupStatus = NymeaCore::instance()->thingManager()->translate(thing->pluginId(), thing->setupDisplayMessage(), context.locale());
            if (!translatedSetupStatus.isEmpty()) {
                packedThing["setupDisplayMessage"] = translatedSetupStatus;
            }
            things.append(packedThing);
        }
    }
    returns.insert("thingError", enumValueName<Thing::ThingError>(Thing::ThingErrorNoError));
    returns.insert("things", things);
    return createReply(returns);
}

JsonReply *IntegrationsHandler::ReconfigureThing(const QVariantMap &params, const JsonContext &context)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    ParamList thingParams = unpack<ParamList>(params.value("thingParams"));
    ThingDescriptorId thingDescriptorId(params.value("thingDescriptorId").toString());
    QLocale locale = context.locale();

    JsonReply *jsonReply = createAsyncReply("ReconfigureThing");

    ThingSetupInfo *info;
    if (!thingDescriptorId.isNull()) {
        info = NymeaCore::instance()->thingManager()->reconfigureThing(thingDescriptorId, thingParams);
    } else if (!thingId.isNull()){
        info = NymeaCore::instance()->thingManager()->reconfigureThing(thingId, thingParams);
    } else {
        qCWarning(dcJsonRpc()) << "Either thingId or thingDescriptorId are required";
        QVariantMap ret;
        ret.insert("thingError", enumValueName(Thing::ThingErrorMissingParameter));
        return createReply(ret);
    }

    connect(info, &ThingSetupInfo::finished, jsonReply, [info, jsonReply, locale](){

        QVariantMap returns;
        returns.insert("thingError", enumValueName<Thing::ThingError>(info->status()));
        returns.insert("displayMessage", info->translatedDisplayMessage(locale));
        jsonReply->setData(returns);
        jsonReply->finished();

    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::EditThing(const QVariantMap &params)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    QString name = params.value("name").toString();

    qCDebug(dcJsonRpc()) << "Edit thing" << thingId << name;

    Thing::ThingError status = NymeaCore::instance()->thingManager()->editThing(thingId, name);

    return createReply(statusToReply(status));
}

JsonReply* IntegrationsHandler::RemoveThing(const QVariantMap &params)
{
    QVariantMap returns;
    ThingId thingId = ThingId(params.value("thingId").toString());

    // global removePolicy has priority
    if (params.contains("removePolicy")) {
        RuleEngine::RemovePolicy removePolicy = params.value("removePolicy").toString() == "RemovePolicyCascade" ? RuleEngine::RemovePolicyCascade : RuleEngine::RemovePolicyUpdate;
        Thing::ThingError status = NymeaCore::instance()->removeConfiguredThing(thingId, removePolicy);
        returns.insert("thingError", enumValueName<Thing::ThingError>(status));
        return createReply(returns);
    }

    QHash<RuleId, RuleEngine::RemovePolicy> removePolicyList;
    foreach (const QVariant &variant, params.value("removePolicyList").toList()) {
        RuleId ruleId = RuleId(variant.toMap().value("ruleId").toString());
        RuleEngine::RemovePolicy policy = variant.toMap().value("policy").toString() == "RemovePolicyCascade" ? RuleEngine::RemovePolicyCascade : RuleEngine::RemovePolicyUpdate;
        removePolicyList.insert(ruleId, policy);
    }

    QPair<Thing::ThingError, QList<RuleId> > status = NymeaCore::instance()->removeConfiguredThing(thingId, removePolicyList);
    returns.insert("thingError", enumValueName<Thing::ThingError>(status.first));

    if (!status.second.isEmpty()) {
        QVariantList ruleIdList;
        foreach (const RuleId &ruleId, status.second) {
            ruleIdList.append(ruleId.toString());
        }
        returns.insert("ruleIds", ruleIdList);
    }

    return createReply(returns);
}

JsonReply *IntegrationsHandler::SetThingSettings(const QVariantMap &params)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    ParamList settings = unpack<ParamList>(params.value("settings"));
    Thing::ThingError status = NymeaCore::instance()->thingManager()->setThingSettings(thingId, settings);
    return createReply(statusToReply(status));
}

JsonReply *IntegrationsHandler::SetEventLogging(const QVariantMap &params)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    EventTypeId eventTypeId = EventTypeId(params.value("eventTypeId").toUuid());
    bool enabled = params.value("enabled").toBool();
    Thing::ThingError status = NymeaCore::instance()->thingManager()->setEventLogging(thingId, eventTypeId, enabled);
    return createReply(statusToReply(status));
}

JsonReply *IntegrationsHandler::SetStateFilter(const QVariantMap &params)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    StateTypeId stateTypeId = StateTypeId(params.value("stateTypeId").toUuid());
    QString filterString = params.value("filter").toString();
    QMetaEnum metaEnum = QMetaEnum::fromType<Types::StateValueFilter>();
    Types::StateValueFilter filter = static_cast<Types::StateValueFilter>(metaEnum.keyToValue(filterString.toUtf8()));
    Thing::ThingError status = NymeaCore::instance()->thingManager()->setStateFilter(thingId, stateTypeId, filter);
    return createReply(statusToReply(status));
}

JsonReply* IntegrationsHandler::GetEventTypes(const QVariantMap &params, const JsonContext &context) const
{
    ThingClass thingClass = NymeaCore::instance()->thingManager()->findThingClass(ThingClassId(params.value("thingClassId").toString()));
    ThingClass translatedThingClass = NymeaCore::instance()->thingManager()->translateThingClass(thingClass, context.locale());

    QVariantMap returns;
    returns.insert("eventTypes", pack(translatedThingClass.eventTypes()));
    return createReply(returns);
}

JsonReply* IntegrationsHandler::GetActionTypes(const QVariantMap &params, const JsonContext &context) const
{
    ThingClass thingClass = NymeaCore::instance()->thingManager()->findThingClass(ThingClassId(params.value("thingClassId").toString()));
    ThingClass translatedThingClass = NymeaCore::instance()->thingManager()->translateThingClass(thingClass, context.locale());

    QVariantMap returns;
    returns.insert("actionTypes", pack(translatedThingClass.actionTypes()));
    return createReply(returns);
}

JsonReply* IntegrationsHandler::GetStateTypes(const QVariantMap &params, const JsonContext &context) const
{
    ThingClass thingClass = NymeaCore::instance()->thingManager()->findThingClass(ThingClassId(params.value("thingClassId").toString()));
    ThingClass translatedThingClass = NymeaCore::instance()->thingManager()->translateThingClass(thingClass, context.locale());

    QVariantMap returns;
    returns.insert("stateTypes", pack(translatedThingClass.stateTypes()));
    return createReply(returns);
}

JsonReply* IntegrationsHandler::GetStateValue(const QVariantMap &params) const
{
    Thing *thing = NymeaCore::instance()->thingManager()->findConfiguredThing(ThingId(params.value("thingId").toString()));
    if (!thing) {
        return createReply(statusToReply(Thing::ThingErrorThingNotFound));
    }
    StateTypeId stateTypeId = StateTypeId(params.value("stateTypeId").toString());
    if (!thing->hasState(stateTypeId)) {
        return createReply(statusToReply(Thing::ThingErrorStateTypeNotFound));
    }

    QVariantMap returns = statusToReply(Thing::ThingErrorNoError);
    returns.insert("value", thing->state(stateTypeId).value());
    return createReply(returns);
}

JsonReply *IntegrationsHandler::GetStateValues(const QVariantMap &params) const
{
    Thing *thing = NymeaCore::instance()->thingManager()->findConfiguredThing(ThingId(params.value("thingId").toString()));
    if (!thing) {
        return createReply(statusToReply(Thing::ThingErrorThingNotFound));
    }

    QVariantMap returns = statusToReply(Thing::ThingErrorNoError);
    returns.insert("values", pack(thing->states()));
    return createReply(returns);
}

JsonReply *IntegrationsHandler::BrowseThing(const QVariantMap &params, const JsonContext &context) const
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    QString itemId = params.value("itemId").toString();

    JsonReply *jsonReply = createAsyncReply("BrowseThing");

    BrowseResult *result = NymeaCore::instance()->thingManager()->browseThing(thingId, itemId, context.locale());
    connect(result, &BrowseResult::finished, jsonReply, [this, jsonReply, result, context](){

        QVariantMap returns = statusToReply(result->status());
        QVariantList list;
        foreach (const BrowserItem &item, result->items()) {
            list.append(packBrowserItem(item));
        }
        returns.insert("items", list);
        if (!result->displayMessage().isEmpty()) {
            returns.insert("displayMessage", result->translatedDisplayMessage(context.locale()));
        }
        jsonReply->setData(returns);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::GetBrowserItem(const QVariantMap &params, const JsonContext &context) const
{
    QVariantMap returns;
    ThingId thingId = ThingId(params.value("thingId").toString());
    QString itemId = params.value("itemId").toString();

    JsonReply *jsonReply = createAsyncReply("GetBrowserItem");

    BrowserItemResult *result = NymeaCore::instance()->thingManager()->browserItemDetails(thingId, itemId, context.locale());
    connect(result, &BrowserItemResult::finished, jsonReply, [this, jsonReply, result, context](){
        QVariantMap params = statusToReply(result->status());
        if (result->status() == Thing::ThingErrorNoError) {
            params.insert("item", packBrowserItem(result->item()));
        }
        if (!result->displayMessage().isEmpty()) {
            params.insert("displayMessage", result->translatedDisplayMessage(context.locale()));
        }
        jsonReply->setData(params);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::ExecuteAction(const QVariantMap &params, const JsonContext &context)
{
    ThingId thingId(params.value("thingId").toString());
    ActionTypeId actionTypeId(params.value("actionTypeId").toString());
    ParamList actionParams = unpack<ParamList>(params.value("params"));
    QLocale locale = context.locale();

    Action action(actionTypeId, thingId);
    action.setParams(actionParams);

    JsonReply *jsonReply = createAsyncReply("ExecuteAction");

    ThingActionInfo *info = NymeaCore::instance()->thingManager()->executeAction(action);
    connect(info, &ThingActionInfo::finished, jsonReply, [info, jsonReply, locale](){
        QVariantMap data;
        data.insert("thingError", enumValueName(info->status()));
        if (!info->displayMessage().isEmpty()) {
            data.insert("displayMessage", info->translatedDisplayMessage(locale));
        }
        jsonReply->setData(data);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::ExecuteBrowserItem(const QVariantMap &params, const JsonContext &context)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    QString itemId = params.value("itemId").toString();
    BrowserAction action(thingId, itemId);

    JsonReply *jsonReply = createAsyncReply("ExecuteBrowserItem");

    BrowserActionInfo *info = NymeaCore::instance()->executeBrowserItem(action);
    connect(info, &BrowserActionInfo::finished, jsonReply, [info, jsonReply, context](){
        QVariantMap data;
        data.insert("thingError", enumValueName<Thing::ThingError>(info->status()));
        if (!info->displayMessage().isEmpty()) {
            data.insert("displayMessage", info->translatedDisplayMessage(context.locale()));
        }
        jsonReply->setData(data);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::ExecuteBrowserItemAction(const QVariantMap &params, const JsonContext &context)
{
    ThingId thingId = ThingId(params.value("thingId").toString());
    QString itemId = params.value("itemId").toString();
    ActionTypeId actionTypeId = ActionTypeId(params.value("actionTypeId").toString());
    ParamList paramList = unpack<ParamList>(params.value("params"));
    BrowserItemAction browserItemAction(thingId, itemId, actionTypeId, paramList);

    JsonReply *jsonReply = createAsyncReply("ExecuteBrowserItemAction");

    BrowserItemActionInfo *info = NymeaCore::instance()->executeBrowserItemAction(browserItemAction);
    connect(info, &BrowserItemActionInfo::finished, jsonReply, [info, jsonReply, context](){
        QVariantMap data;
        data.insert("thingError", enumValueName<Thing::ThingError>(info->status()));
        if (!info->displayMessage().isEmpty()) {
            data.insert("displayMessage", info->translatedDisplayMessage(context.locale()));
        }
        jsonReply->setData(data);
        jsonReply->finished();
    });

    return jsonReply;
}

JsonReply *IntegrationsHandler::GetIOConnections(const QVariantMap &params)
{
    ThingId thingId = params.value("thingId").toUuid();
    IOConnections ioConnections = m_thingManager->ioConnections(thingId);
    QVariantMap returns;
    QVariant bla = pack(ioConnections);
    returns.insert("ioConnections", pack(ioConnections));
    return createReply(returns);
}

JsonReply *IntegrationsHandler::ConnectIO(const QVariantMap &params)
{
    ThingId inputThingId = params.value("inputThingId").toUuid();
    StateTypeId inputStateTypeId = params.value("inputStateTypeId").toUuid();
    ThingId outputThingId = params.value("outputThingId").toUuid();
    StateTypeId outputStateTypeId = params.value("outputStateTypeId").toUuid();
    bool inverted = params.value("inverted", false).toBool();
    IOConnectionResult result = m_thingManager->connectIO(inputThingId, inputStateTypeId, outputThingId, outputStateTypeId, inverted);
    QVariantMap reply = statusToReply(result.error);
    if (result.error == Thing::ThingErrorNoError) {
        reply.insert("ioConnectionId", result.ioConnectionId);
    }
    return createReply(reply);
}

JsonReply *IntegrationsHandler::DisconnectIO(const QVariantMap &params)
{
    IOConnectionId ioConnectionId = params.value("ioConnectionId").toUuid();
    Thing::ThingError error = m_thingManager->disconnectIO(ioConnectionId);
    return createReply(statusToReply(error));
}

QVariantMap IntegrationsHandler::packBrowserItem(const BrowserItem &item)
{
    QVariantMap ret;
    ret.insert("id", item.id());
    ret.insert("displayName", item.displayName());
    ret.insert("description", item.description());
    ret.insert("icon", enumValueName<BrowserItem::BrowserIcon>(item.icon()));
    if (item.extendedPropertiesFlags().testFlag(BrowserItem::ExtendedPropertiesMedia)) {
        ret.insert("mediaIcon", enumValueName<MediaBrowserItem::MediaBrowserIcon>(static_cast<MediaBrowserItem::MediaBrowserIcon>(item.extendedProperty("mediaIcon").toInt())));
    }
    ret.insert("thumbnail", item.thumbnail());
    ret.insert("executable", item.executable());
    ret.insert("browsable", item.browsable());
    ret.insert("disabled", item.disabled());
    QVariantList actionTypeIds;
    foreach (const ActionTypeId &id, item.actionTypeIds()) {
        actionTypeIds.append(id.toString());
    }
    ret.insert("actionTypeIds", actionTypeIds);
    return ret;
}

void IntegrationsHandler::pluginConfigChanged(const PluginId &id, const ParamList &config)
{
    QVariantMap params;
    params.insert("pluginId", id);
    QVariantList configList;
    foreach (const Param &param, config) {
        configList << pack(param);
    }
    params.insert("configuration", configList);
    emit PluginConfigurationChanged(params);
}

void IntegrationsHandler::thingStateChanged(Thing *thing, const QUuid &stateTypeId, const QVariant &value)
{
    QVariantMap params;
    params.insert("thingId", thing->id());
    params.insert("stateTypeId", stateTypeId);
    params.insert("value", value);
    emit StateChanged(params);
}

void IntegrationsHandler::thingRemovedNotification(const ThingId &thingId)
{
    QVariantMap params;
    params.insert("thingId", thingId);
    emit ThingRemoved(params);
}

void IntegrationsHandler::thingAddedNotification(Thing *thing)
{
    QVariantMap params;
    params.insert("thing", pack(thing));
    emit ThingAdded(params);
}

void IntegrationsHandler::thingChangedNotification(Thing *thing)
{
    QVariantMap params;
    params.insert("thing", pack(thing));
    emit ThingChanged(params);
}

void IntegrationsHandler::thingSettingChangedNotification(const ThingId &thingId, const ParamTypeId &paramTypeId, const QVariant &value)
{
    QVariantMap params;
    params.insert("thingId", thingId);
    params.insert("paramTypeId", paramTypeId.toString());
    params.insert("value", value);
    emit ThingSettingChanged(params);
}

QVariantMap IntegrationsHandler::statusToReply(Thing::ThingError status) const
{
    QVariantMap returns;
    returns.insert("thingError", enumValueName<Thing::ThingError>(status));
    return returns;
}

}
