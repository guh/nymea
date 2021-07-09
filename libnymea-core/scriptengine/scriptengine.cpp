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

#include "scriptengine.h"
#include "integrations/thingmanager.h"

#include "nymeascript.h"
#include "scriptaction.h"
#include "scriptevent.h"
#include "scriptstate.h"
#include "scriptalarm.h"
#include "scriptinterfaceaction.h"
#include "scriptinterfaceevent.h"

#include "nymeasettings.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QJsonParseError>
#include <QJsonDocument>

#include "loggingcategories.h"

#include <QDir>

namespace nymeaserver {

QList<ScriptEngine*> ScriptEngine::s_engines;
QtMessageHandler ScriptEngine::s_upstreamMessageHandler;
QLoggingCategory::CategoryFilter ScriptEngine::s_oldCategoryFilter = nullptr;
QMutex ScriptEngine::s_loggerMutex;

ScriptEngine::ScriptEngine(ThingManager *deviceManager, QObject *parent) : QObject(parent),
    m_deviceManager(deviceManager)
{
    qmlRegisterType<NymeaScript>("nymea", 1, 0, "NymeaScript");
    qmlRegisterType<ScriptEvent>("nymea", 1, 0, "DeviceEvent");
    qmlRegisterType<ScriptAction>("nymea", 1, 0, "DeviceAction");
    qmlRegisterType<ScriptState>("nymea", 1, 0, "DeviceState");
    qmlRegisterType<ScriptEvent>("nymea", 1, 0, "ThingEvent");
    qmlRegisterType<ScriptAction>("nymea", 1, 0, "ThingAction");
    qmlRegisterType<ScriptState>("nymea", 1, 0, "ThingState");
    qmlRegisterType<ScriptInterfaceAction>("nymea", 1, 0, "InterfaceAction");
    qmlRegisterType<ScriptInterfaceEvent>("nymea", 1, 0, "InterfaceEvent");
    qmlRegisterType<ScriptAlarm>("nymea", 1, 0, "Alarm");

    m_engine = new QQmlEngine(this);
    m_engine->setProperty("thingManager", reinterpret_cast<quint64>(m_deviceManager));

    // Don't automatically print script warnings (that is, runtime errors, *not* console.warn() messages)
    // to stdout as they'd end up on the "default" logging category.
    // We collect them ourselves through the warnings() signal and print them to the dcScriptEngine category.
    m_engine->setOutputWarningsToStandardError(false);
    connect(m_engine, &QQmlEngine::warnings, this, [this](const QList<QQmlError> &warnings){
        foreach (const QQmlError &warning, warnings) {
            QMessageLogContext ctx(warning.url().toString().toUtf8(), warning.line(), "", "ScriptEngine");
            // Send to script logs
            onScriptMessage(
#if QT_VERSION >= QT_VERSION_CHECK(5,9,0)
                        warning.messageType(),
#else
                        QtMsgType::QtWarningMsg,
#endif
                        ctx, warning.description());
            // and to logging system
            qCWarning(dcScriptEngine()) << warning.toString();
        }
    });

    // console.log()/warn() messages instead are printed to the "qml" category. We install our own
    // filter to *always* get them, regardless of the configured logging categories
    if (!s_oldCategoryFilter) {
        s_oldCategoryFilter = QLoggingCategory::installFilter(&logCategoryFilter);
    }
    // and our own handler to redirect them to the ScriptEngine category
    if (s_engines.isEmpty()) {
        s_upstreamMessageHandler = qInstallMessageHandler(&logMessageHandler);
    }
    s_engines.append(this);


    QDir dir;
    if (!dir.exists(NymeaSettings::storagePath() + "/scripts/")) {
        dir.mkpath(NymeaSettings::storagePath() + "/scripts/");
    }

    loadScripts();

}

ScriptEngine::~ScriptEngine()
{
    foreach (Script *script, m_scripts) {
        unloadScript(script);
        delete script;
    }
    s_engines.removeAll(this);
    if (s_engines.isEmpty()) {
        qInstallMessageHandler(s_upstreamMessageHandler);
    }
}

Scripts ScriptEngine::scripts()
{
    Scripts ret;
    foreach (Script *script, m_scripts) {
        ret.append(*script);
    }
    return ret;
}

ScriptEngine::GetScriptReply ScriptEngine::scriptContent(const QUuid &id)
{
    GetScriptReply reply;
    if (!m_scripts.contains(id)) {
        reply.scriptError = ScriptErrorScriptNotFound;
        return reply;
    }
    QFile scriptFile(baseName(id) + ".qml");
    if (!scriptFile.open(QFile::ReadOnly)) {
        reply.scriptError = ScriptErrorHardwareFailure;
        return reply;
    }
    reply.content = scriptFile.readAll();
    reply.scriptError = ScriptErrorNoError;

    scriptFile.close();
    return reply;
}

ScriptEngine::AddScriptReply ScriptEngine::addScript(const QString &name, const QByteArray &content)
{
    QUuid id = QUuid::createUuid();
    QString fileName = baseName(id) + ".qml";
    QString jsonFileName = baseName(id) + ".json";

    AddScriptReply reply;

    QFile jsonFile(jsonFileName);
    if (!jsonFile.open(QFile::ReadWrite)) {
        qCWarning(dcScriptEngine()) << "Error opening script metadata" << jsonFileName;
        reply.scriptError = ScriptErrorHardwareFailure;
        return reply;
    }
    QVariantMap metadata;
    metadata.insert("name", name);
    jsonFile.write(QJsonDocument::fromVariant(metadata).toJson());
    jsonFile.close();

    QFile scriptFile(fileName);
    if (!scriptFile.open(QFile::WriteOnly)) {
        qCWarning(dcScriptEngine()) << "Error opening script file:" << fileName;
        reply.scriptError = ScriptErrorHardwareFailure;
        return reply;
    }

    qint64 len = scriptFile.write(content);
    if (len != content.length()) {
        qCWarning(dcScriptEngine()) << "Error writing script content";
        reply.scriptError = ScriptErrorHardwareFailure;
        return reply;
    }
    scriptFile.close();

    Script *script = new Script();
    script->setId(id);
    script->setName(name);
    bool loaded = loadScript(script);
    if (!loaded) {
        reply.scriptError = ScriptErrorInvalidScript;
        reply.errors = script->errors;
        delete script;
        QFile::remove(jsonFileName);
        QFile::remove(fileName);
        return reply;
    }

    m_scripts.insert(script->id(), script);

    reply.scriptError = ScriptErrorNoError;
    reply.script = *m_scripts.value(id);

    emit scriptAdded(reply.script);

    return reply;
}

ScriptEngine::ScriptError ScriptEngine::renameScript(const QUuid &id, const QString &name)
{
    if (!m_scripts.contains(id)) {
        qCWarning(dcScriptEngine()) << "No script with id" << id;
        return ScriptErrorScriptNotFound;
    }

    QString jsonFileName = baseName(id) + ".json";
    QFile jsonFile(jsonFileName);
    if (!jsonFile.open(QFile::ReadWrite)) {
        qCWarning(dcJsonRpc()) << "Erorr opening script json file" << jsonFileName;
        return ScriptErrorHardwareFailure;
    }
    QJsonParseError error;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonFile.readAll(), &error);
    QVariantMap jsonData = jsonDocument.toVariant().toMap();
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcScriptEngine()) << "Error parsing json file. Recreating it...";
        // This is non-critical as we could open it. We can recreate it now.
    }
    jsonData["name"] = name;
    QByteArray jsonString = QJsonDocument::fromVariant(jsonData).toJson();
    if (!jsonFile.resize(0) || jsonFile.write(jsonString) != jsonString.length()) {
        qCWarning(dcScriptEngine()) << "Error writing json metadata" << jsonFileName;
        return ScriptErrorHardwareFailure;
    }
    jsonFile.close();
    m_scripts[id]->setName(name);
    qCDebug(dcScriptEngine()) << "Script" << id << "renamed to" << name;
    emit scriptRenamed(*m_scripts.value(id));
    return ScriptErrorNoError;
}

ScriptEngine::EditScriptReply ScriptEngine::editScript(const QUuid &id, const QByteArray &content)
{
    EditScriptReply reply;

    if (!m_scripts.contains(id)) {
        qCWarning(dcScriptEngine()) << "No script with id" << id;
        reply.scriptError = ScriptErrorScriptNotFound;
        return reply;
    }

    Script *script = m_scripts.value(id);
    unloadScript(script);

    // Deleted compiled qml file to make sure we're reloading the new one
    QString compiledScriptFileName = baseName(id) + ".qmlc";
    QFile::remove(compiledScriptFileName);

    QString scriptFileName = baseName(id) + ".qml";
    QFile scriptFile(scriptFileName);
    if (!scriptFile.open(QFile::ReadWrite)) {
        qCWarning(dcScriptEngine()) << "Error opening script" << id;
        reply.scriptError = ScriptErrorHardwareFailure;
        return reply;
    }

    QByteArray oldContent = scriptFile.readAll();
    scriptFile.close();

    scriptFile.open(QFile::WriteOnly | QFile::Truncate);
    qint64 bytesWritten = scriptFile.write(content);
    scriptFile.flush();
    scriptFile.close();
    if (bytesWritten != content.length()) {
        qCWarning(dcScriptEngine()) << "Error writing script content";
        reply.scriptError = ScriptErrorHardwareFailure;
        return reply;
    }

    bool loaded = loadScript(script);
    if (!loaded) {
        qCDebug(dcScriptEngine()) << "Restoring old content";
        reply.scriptError = ScriptErrorInvalidScript;
        reply.errors = script->errors;

        // Restore old content
        scriptFile.open(QFile::WriteOnly | QFile::Truncate);
        scriptFile.write(oldContent);
        scriptFile.flush();
        scriptFile.close();
        loadScript(script);

        return reply;
    }

    qCDebug(dcScriptEngine()) << "Script updated" << script->name();
    reply.scriptError = ScriptErrorNoError;
    emit scriptChanged(*script);
    return reply;
}

ScriptEngine::ScriptError ScriptEngine::removeScript(const QUuid &id)
{
    Script *script = m_scripts.take(id);
    if (!script) {
        return ScriptErrorScriptNotFound;
    }

    unloadScript(script);

    QString jsonFileName = baseName(id) + ".json";
    QString scriptFileName = baseName(id) + ".qml";
    QString compiledScriptFileName = baseName(id) + ".qmlc";

    QFile::remove(scriptFileName);
    QFile::remove(jsonFileName);
    QFile::remove(compiledScriptFileName);

    emit scriptRemoved(script->id());

    delete script;
    return ScriptErrorNoError;
}

void ScriptEngine::loadScripts()
{
    QDir dir(NymeaSettings::storagePath() + "/scripts/");
    foreach (const QString &entry, dir.entryList({"*.json"})) {
        qCDebug(dcScriptEngine()) << "Have script:" << entry;
        QFileInfo jsonFileInfo(NymeaSettings::storagePath() + "/scripts/" + entry);
        QString jsonFileName = jsonFileInfo.absoluteFilePath();
        QString scriptFileName = jsonFileInfo.absolutePath() + "/" + jsonFileInfo.baseName() +  ".qml";
        if (!QFile::exists(scriptFileName)) {
            qCWarning(dcScriptEngine()) << "Missing script" << scriptFileName;
            continue;
        }

        QFile jsonFile(jsonFileName);
        if (!jsonFile.open(QFile::ReadOnly)) {
            qCWarning(dcScriptEngine()) << "Failed to open script metadata" << jsonFileName;
            continue;
        }

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll(), &error);
        jsonFile.close();
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcScriptEngine()) << "Error parsing script metadata" << jsonFileName;
            continue;
        }

        Script *script = new Script();
        script->setId(jsonFileInfo.baseName());
        script->setName(jsonDoc.toVariant().toMap().value("name").toString());

        bool loaded = loadScript(script);
        if (!loaded) {
            qCWarning(dcScriptEngine()) << "Script failed to load:";
            delete script;
            continue;
        }

        m_scripts.insert(script->id(), script);
        qCDebug(dcScriptEngine()) << "Script loaded" << scriptFileName;
    }
}

bool ScriptEngine::loadScript(Script *script)
{
    qCDebug(dcScriptEngine()) << "Loading script" << script->name();

    QString fileName = baseName(script->id()) + ".qml";
    QString jsonFileName = baseName(script->id()) + ".json";

    QFile jsonFile(jsonFileName);
    if (!jsonFile.open(QFile::ReadOnly)) {
        qCWarning(dcScriptEngine()) << "Failed to open script metadata";
        return false;
    }
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll(), &error);
    jsonFile.close();

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcScriptEngine()) << "Failed to parse script metadata";
        return false;
    }

    QString name = jsonDoc.toVariant().toMap().value("name").toString();

    script->errors.clear();

    script->component = new QQmlComponent(m_engine, QUrl::fromLocalFile(fileName), this);
    script->context = new QQmlContext(m_engine, this);
    script->object = script->component->create(script->context);

    if (!script->object) {
        qCWarning(dcScriptEngine()) << "Script failed to load:";
        foreach (const QQmlError &error, script->component->errors()) {
            qCWarning(dcScriptEngine()) << error.toString();
            script->errors.append(QString("%1:%2: %3").arg(error.line()).arg(error.column()).arg(error.description()));
        }
        delete script->context;
        delete script->component;

        m_engine->clearComponentCache();
        return false;
    }
    return true;
}

void ScriptEngine::unloadScript(Script *script)
{
    if (!script->object || !script->component || !script->context) {
        qCWarning(dcScriptEngine()) << "Script seems not to be loaded. Cannot unload.";
        return;
    }
    delete script->object;
    script->object = nullptr;
    delete script->component;
    script->component = nullptr;
    delete script->context;
    script->context = nullptr;

    m_engine->clearComponentCache();
    qCDebug(dcScriptEngine()) << "Unloading script" << script->name();
}

QString ScriptEngine::baseName(const QUuid &id)
{
    QString path = NymeaSettings::storagePath() + "/scripts/";
    QString basename = id.toString().remove(QRegExp("[{}]"));
    return path + basename;
}

void ScriptEngine::onScriptMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QFileInfo fi(context.file);
    QUuid scriptId = fi.baseName();
    if (!m_scripts.contains(scriptId)) {
        return;
    }
    emit scriptConsoleMessage(scriptId, type == QtDebugMsg ? ScriptMessageTypeLog : ScriptMessageTypeWarning, QString::number(context.line) + ": " + message);
}

void ScriptEngine::logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (strcmp(context.category, "qml") != 0) {
        s_upstreamMessageHandler(type, context, message);
        return;
    }

    QMutexLocker locker(&s_loggerMutex);
    // Copy the message to the script engine
    foreach (ScriptEngine *engine, s_engines) {
        engine->onScriptMessage(type, context, message);
    }

    if (!s_oldCategoryFilter) {
        return;
    }

    // Redirect qml messages to the ScriptEngine handler
    QMessageLogContext newContext(context.file, context.line, context.function, "ScriptEngine");
    QLoggingCategory *category = new QLoggingCategory("ScriptEngine", type);
    s_oldCategoryFilter(category);
    if (category->isEnabled(type)) {
        QFileInfo fi(context.file);
        s_upstreamMessageHandler(type, newContext, fi.fileName() + ":" + QString::number(context.line) + ": " + message);
    }
}

void ScriptEngine::logCategoryFilter(QLoggingCategory *category)
{
    // always enable qml logs, regardless what the filters are
    if (qstrcmp(category->categoryName(), "qml") == 0) {
        category->setEnabled(QtDebugMsg, true);
        category->setEnabled(QtWarningMsg, true);
    } else if (s_oldCategoryFilter) {
        s_oldCategoryFilter(category);
    }
}

}
