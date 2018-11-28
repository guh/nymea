/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef RESTSERVER_H
#define RESTSERVER_H

#include <QObject>

#include "servers/webserver.h"
#include "jsonrpc/jsonhandler.h"
#include "devicesresource.h"
#include "deviceclassesresource.h"
#include "vendorsresource.h"
#include "pluginsresource.h"
#include "rulesresource.h"
#include "logsresource.h"

class QSslConfiguration;

namespace nymeaserver {

class HttpRequest;
class HttpReply;

class RestServer : public QObject
{
    Q_OBJECT
public:
    explicit RestServer(const QSslConfiguration &sslConfiguration = QSslConfiguration(), QObject *parent = 0);

    void registerWebserver(WebServer *webServer);

private:
    QHash<QUuid, WebServer*> m_clientList;
    QHash<QString, RestResource *> m_resources;

    QHash<QUuid, HttpReply *> m_asyncReplies;

    DevicesResource *m_deviceResource;
    DeviceClassesResource *m_deviceClassesResource;
    VendorsResource *m_vendorsResource;
    PluginsResource *m_pluginsResource;
    RulesResource *m_rulesResource;
    LogsResource *m_logsResource;

private slots:
    void setup();
    void clientConnected(const QUuid &clientId);
    void clientDisconnected(const QUuid &clientId);
    
    void processHttpRequest(const QUuid &clientId, const HttpRequest &request);
    void asyncReplyFinished();

};

}

#endif // RESTSERVER_H
