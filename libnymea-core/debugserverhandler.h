/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEBUGSERVERHANDLER_H
#define DEBUGSERVERHANDLER_H

#include <QTimer>
#include <QObject>
#include <QProcess>
#include <QUrlQuery>
#include <QWebSocketServer>

#include "debugreportgenerator.h"
#include "servers/httpreply.h"

namespace nymeaserver {

class DebugServerHandler : public QObject
{
    Q_OBJECT
public:
    explicit DebugServerHandler(QObject *parent = nullptr);

    HttpReply *processDebugRequest(const QString &requestPath, const QUrlQuery &requestQuery);

private:
    static QtMessageHandler s_oldLogMessageHandler;
    static QList<QWebSocket*> s_websocketClients;
    static void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);

    QWebSocketServer *m_websocketServer = nullptr;

    QProcess *m_pingProcess = nullptr;
    HttpReply *m_pingReply = nullptr;

    QProcess *m_digProcess = nullptr;
    HttpReply *m_digReply = nullptr;

    QProcess *m_tracePathProcess = nullptr;
    HttpReply *m_tracePathReply = nullptr;

    QHash<DebugReportGenerator *, HttpReply *> m_runningReportGenerators;
    QHash<QString, DebugReportGenerator *> m_finishedReportGenerators;

    QByteArray loadResourceData(const QString &resourceFileName);
    QString getResourceFileName(const QString &requestPath);
    bool resourceFileExits(const QString &requestPath);

    HttpReply *processDebugFileRequest(const QString &requestPath);

    QByteArray createDebugXmlDocument();
    QByteArray createErrorXmlDocument(HttpReply::HttpStatusCode statusCode, const QString &errorMessage);

private slots:
    void onDebugServerEnabledChanged(bool enabled);

    void onWebsocketClientConnected();
    void onWebsocketClientDisconnected();
    void onWebsocketClientError(QAbstractSocket::SocketError error);

    void onPingProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDigProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTracePathProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDebugReportGeneratorFinished(bool success);
    void onDebugReportGeneratorTimeout();
};

}

#endif // DEBUGSERVERHANDLER_H
