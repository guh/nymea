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
  \class IntegrationPlugin
  \brief This is the base class interface for integration plugins.

  \ingroup integrations

  Integration plugins extend nymea to allow integrating a device or online service, commonly referred to as thing in IoT.

  This is the base class to be subclassed when starting a new integration plugin.

*/


/*! \fn void IntegrationPlugin::configValueChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    This signal is emitted when the \l{Param} with a certain \a paramTypeId of a \l{Thing} configuration changed the \a value.
*/

/*! \fn void IntegrationPlugin::autoThingsAppeared(const ThingDescriptors &thingDescriptors)
    A plugin emits this signal when new \l{Things} appears. For instance, a plugin connected to a bridge might detect
    that new devices have been connected to the bridge. Emitting this signal will cause those devices to be added as
    \l{Thing} to the nymea system and setupThing will be called for them.
*/

/*! \fn void IntegrationPlugin::autoThingDisappeared(const ThingId &id)
    A plugin should emit this signal when a thing with the given \a id is to be removed from the system. When emitting this signal,
    nymea will remove the \l{Thing} from the system and with it all the associated rules and child. Because of this, this signal
    should only be emitted when it's certain that the given device or online service  will never return to be available any more.
    This can only be used for things that have been added using \l{IntegrationPlugin::autoThingsAppeared}.
*/

/*! \fn void IntegrationPlugin::emitEvent(const Event &event)
    To produce a new event in the system, a plugin should create a new \l{Event} and emit this signal it with that \a event.
    Usually events are emitted in response to incoming data or other other events happening on the actua device or online service.
*/

/*! \fn void IntegrationPlugin::init()
    This will be called after constructing the IntegrationPlugin. Override this to do any initialisation work you need to do.
    While some initialization can also be done in the constructor, some resources like the hardwareManager might not be available
    at that point yet. When init() is called, it is guaranteed that the system is fully started up and ready for operation.
*/

#include "integrationplugin.h"
#include "thingmanager.h"
#include "thingutils.h"
#include "loggingcategories.h"
#include "thingdiscoveryinfo.h"
#include "thingsetupinfo.h"
#include "thingpairinginfo.h"
#include "thingactioninfo.h"
#include "browseresult.h"
#include "browseritemresult.h"
#include "browseractioninfo.h"
#include "browseritemactioninfo.h"

#include "nymeasettings.h"

#include "hardware/radio433/radio433.h"
#include "network/upnp/upnpdiscovery.h"

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

/*! IntegrationPlugin constructor. IntegrationPlugins will be instantiated by the system.
    This should never be called manually by a plugin implementation.
*/

NYMEA_LOGGING_CATEGORY(dcIntegrations, "Integrations")

IntegrationPlugin::IntegrationPlugin(QObject *parent):
    QObject(parent)
{
}

IntegrationPlugin::~IntegrationPlugin()
{

}

PluginMetadata IntegrationPlugin::metadata()
{
    return m_metaData;
}

/*! Returns the name of this IntegrationPlugin. It returns the name value defined in the plugin's JSON file. */
QString IntegrationPlugin::pluginName() const
{
    return m_metaData.pluginName();
}

/*! Returns the displayName of this IntegrationPlugin, to be shown to the user. It returns the displayName value defined in the plugin's JSON file. */
QString IntegrationPlugin::pluginDisplayName() const
{
    return m_metaData.pluginDisplayName();
}

/*! Returns the id of this IntegrationPlugin.
 *  When implementing a plugin, generate a new uuid and return it here. Always return the
 *  same uuid and don't change it or configurations can't be matched any more. */
PluginId IntegrationPlugin::pluginId() const
{
    return m_metaData.pluginId();
}

/*! Returns the list of \l{Vendor}{Vendors} supported by this plugin. */
Vendors IntegrationPlugin::supportedVendors() const
{
    return m_metaData.vendors();
}

/*! Return a list of \l{ThingClass}{ThingClasses} describing all the \l{Thing}{Things} supported by this plugin.
    If a ThingClass has an invalid parameter it will be ignored.
*/
ThingClasses IntegrationPlugin::supportedThings() const
{
    return m_metaData.thingClasses();
}

/*! Returns a \l{ThingClass}{ThingClass} for the given \a thingClassId */
ThingClass IntegrationPlugin::thingClass(const ThingClassId &thingClassId) const
{
    foreach (const ThingClass &thingClass, supportedThings()) {
        if (thingClass.id() == thingClassId) {
            return thingClass;
        }
    }
    return ThingClass();
}

/*! Override this if your plugin supports things with ThingClass::CreationMethodAuto.
    This will be called at startup, after the configured things have been loaded.
    This is the earliest time you should start emitting autoThingsAppeared(). If you
    are monitoring some hardware/service for things to appear, start monitoring now.
    If you are building the things based on a static list, you may emit
    autoThingsAppeard() in here.
*/
void IntegrationPlugin::startMonitoringAutoThings()
{

}

/*! A plugin must reimplement this if it supports a ThingClass with createMethod \l{Thing}{CreateMethodDiscovery}.
    When the nymea system needs to discover available things, this will be called on the plugin. The plugin implementation
    is set to discover devices or online service endpoints for the \l{ThingClassId} given in the \a info object.
    When things are discovered, they should be added to the info object by calling \l{ThingDiscoveryInfo::addThingDescriptor}.
    Once the discovery is complete, a plugin must finish it by calling \l{ThingDiscoveryInfo::finish} using \l{Thing::ThingErrorNoError}
    in case of success, or a matching error code otherwise. An optional display message can be passed which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.

    A discovery might be cancelled by nymea. In which case the \l{ThingDiscoveryInfo::aborted} signal will be emitted.
    The info object must not be accessed after \l{ThingDiscoveryInfo::destroyed} is emitted.
*/
void IntegrationPlugin::discoverThings(ThingDiscoveryInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

/*! This will be called when a new thing is created. The plugin can do a setup of the thing by reimplementing this method.
    The passed \a info object will contain the information about the new \l{Thing}. When the setup is completed, a plugin
    must finish it by calling \l{ThingSetupInfo::finish} on the \a info object. In case of success, \{Thing::ThingErrorNoError}
    must be used, or an appropriate \l{Thing::ThingError} in case of failure. An optional display message can be passed optionally which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.

    A setup might be cancelled by nymea. In which case the \{ThingSetupInfo::aborted} signal will be emitted.
    The info object must not be accessed after \l{ThingSetupInfo::destroyed} is emitted.
*/
void IntegrationPlugin::setupThing(ThingSetupInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

/*! This will be called when a new \a thing was added successfully and the thing setup is finished. A plugin can optionally
    trigger additional code to operate on the thing by reimplementing this method.*/
void IntegrationPlugin::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
}

/*! This will be called when a \a thing removed. The plugin has the chance to do some cleanup.
    The thing is still valid during this call, but already removed from the system.
    The thing will be deleted as soon as this method returns.
*/
void IntegrationPlugin::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
}

/*! This method will be called to initiate a pairing. The plugin can do a initialisation for an upcoming pairing process.
    Depending on the setupMethod of a thing class, different actions may be required here.
    SetupMethodDisplayPin should trigger the thing to display a pin that will be entered in the client.
    SetupMethodOAuth should generate the OAuthUrl which will be opened on the client to allow the user logging in and obtain
    the OAuth code.
    SetupMethodEnterPin, SetupMethodPushButton and SetupMethodUserAndPassword will typically not require to do anything here.
    It is not required to reimplement this method for those setup methods, however, a plugin reimplementing it must call
    \l{ThingPairingInfo::finish}{finish()} on the \l{ThingPairingInfo} object and can provide an optional displayMessage which
    might be presented to the user. Those strings need to be wrapped in QT_TR_NOOP() in order to be translatable for the client's
    locale.
*/
void IntegrationPlugin::startPairing(ThingPairingInfo *info)
{
    ThingClass thingClass = m_metaData.thingClasses().findById(info->thingClassId());
    if (!thingClass.isValid()) {
        info->finish(Thing::ThingErrorThingClassNotFound);
        return;
    }
    switch (thingClass.setupMethod()) {
    case ThingClass::SetupMethodJustAdd:
        info->finish(Thing::ThingErrorSetupMethodNotSupported);
        return;
    case ThingClass::SetupMethodEnterPin:
    case ThingClass::SetupMethodPushButton:
    case ThingClass::SetupMethodUserAndPassword:
        info->finish(Thing::ThingErrorNoError);
        return;
    case ThingClass::SetupMethodDisplayPin:
    case ThingClass::SetupMethodOAuth:
        // Those need to be handled by the plugin or it'll fail anyways.
        qCWarning(dcThing()) << "StartPairing called but Plugin does not reimplement it.";
        info->finish(Thing::ThingErrorUnsupportedFeature);
    }
}

/*! Confirms the pairing of the given \a info. \a username and \a secret are filled in depending on the setup method of the thing class.
    \a username will be used for SetupMethodUserAndPassword. \a secret will be used for SetupMethodUserAndPassword, SetupMethodDisplayPin
    and SetupMethodOAuth.
    Once the pairing is completed, the plugin implementation should call the info's finish() method reporting about the status of
    the pairing operation. The optional displayMessage needs to be wrapped in QT_TR_NOOP in order to be translatable to the client's
    locale.
*/
void IntegrationPlugin::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)

    qCWarning(dcIntegrations()) << "Plugin does not implement pairing.";
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

/*! This will be called to execute actions on the thing. The given \a info object contains
    information about the target \l{Thing} and the \l{Action} to be executed.

    When the execution is completed, a plugin must finish it by calling \l{ThingActionInfo::finish} on the \a info
    object. In case of success, \l{Thing::ThingErrorNoError} must be used, or an appropriate \l{Thing::ThingError}
    in case of failure. An optional display message can be passed which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.

    An action execution might be cancelled by nymea. In which case the \l{ThingActionInfo::aborted} signal will be emitted.
    The info object must not be accessed after \l{ThingActionInfo::destroyed} is emitted.
*/
void IntegrationPlugin::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

/*! A plugin must implement this if its things support browsing ("browsable" being true in the metadata JSON).
    When the system calls this method, the \a result must be filled with entries from the browser using
    \l{BrowseResult::addItems}. The \a info object will contain information about which thing and which item/node
    should be browsed. If the itemId is empty it means that the root node of the file system should be returned otherwise
    all the children of the given item/node should be returned.


    Each item in the result set shall be uniquely identifiable using its \l{BrowserItem::id}{id} property.
    The system might call this method again, with an itemId returned in a previous query, provided
    that item's \l{BrowserItem::browsable} property is true. In this case all children of the given
    item shall be returned. All browser \l{BrowserItem::displayName} properties shall be localized
    using the given locale in the  \a info object.

    If a returned item's \l{BrowserItem::executable} property is set to true, the system might call \l{ThingPlugin::executeBrowserAction}
    for this itemId.


    An item might have additional actions which must be defined in the plugin metadata JSON as "browserItemActionTypes". Such actions
    might be context actions to items in a browser. For instance, a file browser might add copy/cut/paste actions to an item. The
    system might call \l{ThingPlugin::executeBrowserItemAction} on such items.

    When done, the browse result must be completed by calling \l{BrowserResult::finish} with \l{Thing::ThingErrorNoError}
    in case of success or an appropriate \l{Thing::ThingError} otherwise. An optional display message can be passed which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.
 */
void IntegrationPlugin::browseThing(BrowseResult *result)
{
    qCWarning(dcIntegrations()) << "Thing claims" << result->thing()->thingClass().name() << "to be browsable but plugin does not reimplement browseThing!";
    result->finish(Thing::ThingErrorUnsupportedFeature);
}

/*! A plugin must implement this if its things support browsing ("browsable" being true in the metadata JSON).
    When the system calls this method, the \a result must be filled with a single item identified by the \l{BrowserItemResult::itemId}
    using \l{BrowseResult::addItem}. The \a result object will contain information about which thing and which item/node
    should be browsed. If the itemId is empty it means that the root node of the file system should be returned.

    Each item in the result set shall be uniquely identifiable using its \l{BrowserItem::id}{id} property. The system might
    call this method again, with an itemId returned in a previous query, provided that item's \l{BrowserItem::browsable}
    property is true. In this case all children of the given item shall be returned. All browser \l{BrowserItem::displayName} properties shall be localized
    using the given locale in the \a info object.


    When done, the browse result must be completed by calling \l{BrowserItemResult::finish} with \l{Thing::ThingErrorNoError}
    in case of success or an appropriate \l{Thing::ThingError} otherwise. An optional display message can be passed which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.
 */
void IntegrationPlugin::browserItem(BrowserItemResult *result)
{
    qCWarning(dcIntegrations()) << "Thing claims" << result->thing()->thingClass().name() << "to be browsable but plugin does not reimplement browserItem!";
    result->finish(Thing::ThingErrorUnsupportedFeature);
}

/*! This will be called to execute browser items on the thing. For instance, a file browser might execute a file here.
    The given \a info object contains information about the target \l{Thing} and the \l{BrowserAction} to be executed.

    When the execution is completed, a plugin must finish it by calling \l{BrowserActionInfo::finish} on the \a info
    object. In case of success, \{Thing::ThingErrorNoError} must be used, or an appropriate \l{Thing::ThingError}
    in case of failure. An optional display message can be passed which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.

    An action execution might be cancelled by nymea. In which case the \{BrowserActionInfo::aborted} signal will be emitted.
    The info object must not be accessed after \l{BrowsereActionInfo::destroyed} is emitted.
*/
void IntegrationPlugin::executeBrowserItem(BrowserActionInfo *info)
{
    qCWarning(dcIntegrations()) << "Thing claims" << info->thing()->thingClass().name() << "to be browsable but plugin does not reimplement browserItem!";
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

/*! This will be called to execute browser item actions on the thing. For instance a file browser might have
    "browserItemActionTypes" defined in the JSON in order to support context options like copy/cut/paste.
    The given \a info object contains information about the target \l{Thing} and the \l{BrowserItemAction} to be executed.

    When the execution is completed, a plugin must finish it by calling \l{BrowserItemActionInfo::finish} on the \a info
    object. In case of success, \l{Thing::ThingErrorNoError} must be used, or an appropriate \l{Thing::ThingError}
    in case of failure. An optional display message can be passed which might be shown
    to the user, indicating more details about the error. The displayMessage must be made translatable by wrapping it in a QT_TR_NOOP()
    statement.

    An action execution might be cancelled by nymea. In which case the \{BrowserItemActionInfo::aborted} signal will be emitted.
    The info object must not be accessed after \l{BrowserItemActionInfo::destroyed} is emitted.
*/
void IntegrationPlugin::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    qCWarning(dcIntegrations()) << "Thing claims" << info->thing()->thingClass().name() << "to be browsable but plugin does not reimplement browserItemAction!";
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

/*!
 * \brief IntegrationPlugin::serviceInformation
 * This method can provide service data for a plugin. A service entity may query this data, either a single time
 * or repeatedly.
 * \return Return a list of \l{ServiceData} objects.
 *
 * Note: This method is experimental and will likely change in the future. Please provide feedback on missing and required features for this mechanism.
 */
QList<ServiceData> IntegrationPlugin::serviceInformation() const
{
    qCWarning(dcIntegrations()) << "Plugin" << pluginName() << "does not support providing service information";
    return QList<ServiceData>();
}

/*! Returns the configuration description of this IntegrationPlugin as a list of \l{ParamType}{ParamTypes}. */
ParamTypes IntegrationPlugin::configurationDescription() const
{
    return m_metaData.pluginSettings();
}

void IntegrationPlugin::initPlugin(ThingManager *thingManager, HardwareManager *hardwareManager, ApiKeyStorage *apiKeyStorage)
{
    m_thingManager = thingManager;
    m_hardwareManager = hardwareManager;
    m_apiKeyStorage = apiKeyStorage;
    m_storage = new QSettings(NymeaSettings::settingsPath() + "/pluginconfig-" + pluginId().toString().remove(QRegExp("[{}]")) + ".conf", QSettings::IniFormat, this);
}

/*! Returns a map containing the plugin configuration.
    When implementing a new plugin, override this and fill in the empty configuration if your plugin requires any.
*/
ParamList IntegrationPlugin::configuration() const
{
    return m_config;
}

/*! Use this to retrieve the values for your parameters. Values might not be set
    at the time when your plugin is loaded, but will be set soon after. Listen to
    configurationValueChanged() to know when something changes.
    When implementing a new plugin, specify in configurationDescription() what you want to see here.
    Returns the config value of a \l{Param} with the given \a paramTypeId of this IntegrationPlugin.
*/
QVariant IntegrationPlugin::configValue(const ParamTypeId &paramTypeId) const
{
    return m_config.paramValue(paramTypeId);
}

/*! Will be called by the systtem to set a plugin's \a configuration. */
Thing::ThingError IntegrationPlugin::setConfiguration(const ParamList &configuration)
{
    foreach (const Param &param, configuration) {
        qCDebug(dcThingManager()) << "* Set plugin configuration" << param;
        Thing::ThingError result = setConfigValue(param.paramTypeId(), param.value());
        if (result != Thing::ThingErrorNoError)
            return result;

    }
    return Thing::ThingErrorNoError;
}

/*! Can be called in the IntegrationPlugin to set a plugin's \l{Param} with the given \a paramTypeId and \a value. */
Thing::ThingError IntegrationPlugin::setConfigValue(const ParamTypeId &paramTypeId, const QVariant &value)
{
    bool found = false;
    foreach (const ParamType &paramType, configurationDescription()) {
        if (paramType.id() == paramTypeId) {
            found = true;
            Thing::ThingError result = ThingUtils::verifyParam(paramType, Param(paramTypeId, value));
            if (result != Thing::ThingErrorNoError)
                return result;

            break;
        }
    }

    if (!found) {
        qCWarning(dcThingManager()) << QString("Could not find plugin parameter with the id %1.").arg(paramTypeId.toString());
        return Thing::ThingErrorInvalidParameter;
    }

    if (m_config.hasParam(paramTypeId)) {
        if (!m_config.setParamValue(paramTypeId, value)) {
            qCWarning(dcThingManager()) << "Could not set param value" << value << "for param with id" << paramTypeId.toString();
            return Thing::ThingErrorInvalidParameter;
        }
    } else {
        m_config.append(Param(paramTypeId, value));
    }

    emit configValueChanged(paramTypeId, value);
    return Thing::ThingErrorNoError;
}

bool IntegrationPlugin::isBuiltIn() const
{
    return m_metaData.isBuiltIn();
}

/*! Returns a list of all configured things belonging to this plugin. */
Things IntegrationPlugin::myThings() const
{
    QList<Thing*> ret;
    foreach (Thing *thing, m_thingManager->configuredThings()) {
        if (thing->pluginId() == pluginId()) {
            ret.append(thing);
        }
    }
    return ret;
}


/*! Returns the pointer to the main \l{HardwareManager} of this server. */
HardwareManager *IntegrationPlugin::hardwareManager() const
{
    return m_hardwareManager;
}

/*! Returns a pointer to a QSettings object which is reserved for this plugin.
    The plugin can store arbitrary data in this.
    */
QSettings* IntegrationPlugin::pluginStorage() const
{
    return m_storage;
}

/*!
 * \brief IntegrationPlugin::apiKeyStorage
 * \return Returns the api key storage for this plugin. A plugin needs to list required API keys in the plugins JSON file.
 */
ApiKeyStorage *IntegrationPlugin::apiKeyStorage() const
{
    return m_apiKeyStorage;
}

void IntegrationPlugin::setMetaData(const PluginMetadata &metaData)
{
    m_metaData = metaData;
}

IntegrationPlugins::IntegrationPlugins()
{

}

IntegrationPlugins::IntegrationPlugins(const QList<IntegrationPlugin *> &other): QList<IntegrationPlugin *>(other)
{

}

IntegrationPlugin *IntegrationPlugins::findById(const PluginId &id) const
{
    foreach (IntegrationPlugin *plugin, *this) {
        if (plugin->pluginId() == id) {
            return plugin;
        }
    }
    return nullptr;
}

QVariant IntegrationPlugins::get(int index) const
{
    return QVariant::fromValue(at(index));
}

void IntegrationPlugins::put(const QVariant &variant)
{
    append(variant.value<IntegrationPlugin*>());
}
