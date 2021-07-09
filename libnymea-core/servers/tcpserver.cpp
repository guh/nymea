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
    \class nymeaserver::SslServer
    \brief This class represents the SSL server for nymead.

    \ingroup server
    \inmodule core

    \inherits TcpServer

    The SSL server allows clients to connect to the JSON-RPC API over an encrypted SSL/TLS connection.

    \sa WebSocketServer, TransportInterface, TcpServer
*/

/*! \fn nymeaserver::SslServer::SslServer(bool sslEnabled, const QSslConfiguration &config, QObject *parent = nullptr)
    Constructs a \l{SslServer} with the given \a sslEnabled, \a config and \a parent.
*/

/*! \fn void nymeaserver::SslServer::clientConnected(QSslSocket *socket);
    This signal is emitted when a new SSL \a socket connected.
*/

/*! \fn void nymeaserver::SslServer::clientDisconnected(QSslSocket *socket);
    This signal is emitted when a \a socket disconnected.
*/

/*! \fn void nymeaserver::SslServer::dataAvailable(QSslSocket *socket, const QByteArray &data);
    This signal is emitted when \a data from \a socket is available.
*/


/*!
    \class nymeaserver::TcpServer
    \brief This class represents the tcp server for nymead.

    \ingroup server
    \inmodule core

    \inherits TransportInterface

    The TCP server allows clients to connect to the JSON-RPC API.

    \sa WebSocketServer, TransportInterface
*/

#include "tcpserver.h"
#include "nymeacore.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <QDebug>

namespace nymeaserver {

/*! Constructs a \l{TcpServer} with the given \a configuration, \a sslConfiguration and \a parent.
 *
 *  \sa ServerManager
 */
TcpServer::TcpServer(const ServerConfiguration &configuration, const QSslConfiguration &sslConfiguration, QObject *parent) :
    TransportInterface(configuration, parent),
    m_server(nullptr),
    m_sslConfig(sslConfiguration)
{
}

/*! Destructor of this \l{TcpServer}. */
TcpServer::~TcpServer()
{
    qCDebug(dcTcpServer()) << "Shutting down \"TCP Server\"" << serverUrl().toString();
    stopServer();
}

/*! Returns the URL of this server. */
QUrl TcpServer::serverUrl() const
{
    return QUrl(QString("%1://%2:%3").arg((configuration().sslEnabled ? "nymeas" : "nymea")).arg(configuration().address.toString()).arg(configuration().port));
}

/*! Sending \a data to a list of \a clients.*/
void TcpServer::sendData(const QList<QUuid> &clients, const QByteArray &data)
{
    foreach (const QUuid &client, clients) {
        sendData(client, data);
    }
}

void TcpServer::terminateClientConnection(const QUuid &clientId)
{
    QTcpSocket *client = m_clientList.value(clientId);
    if (client) {
        client->close();
    }
}

/*! Sending \a data to the client with the given \a clientId.*/
void TcpServer::sendData(const QUuid &clientId, const QByteArray &data)
{
    QTcpSocket *client = nullptr;
    client = m_clientList.value(clientId);
    if (client) {
        qCDebug(dcTcpServerTraffic()) << "Sending to client" << clientId.toString() << data;
        client->write(data + '\n');
    } else {
        qCWarning(dcTcpServer()) << "Client" << clientId.toString() << "unknown to this transport";
    }
}

void TcpServer::onClientConnected(QSslSocket *socket)
{
    QUuid clientId = QUuid::createUuid();
    qCDebug(dcTcpServer()) << "New client connected:" << clientId.toString() << "(Remote address:" << socket->peerAddress().toString() << ")";
    m_clientList.insert(clientId, socket);
    emit clientConnected(clientId);
}

void TcpServer::onClientDisconnected(QSslSocket *socket)
{
    QUuid clientId = m_clientList.key(socket);
    qCDebug(dcTcpServer()) << "Client disconnected:" << clientId.toString() << "(Remote address:" << socket->peerAddress().toString() << ")";
    m_clientList.take(clientId);
    emit clientDisconnected(clientId);
}

void TcpServer::onError(QAbstractSocket::SocketError error)
{
    QTcpServer *server = qobject_cast<QTcpServer *>(sender());
    qCWarning(dcTcpServer) << "Server error on" << server->serverAddress().toString() << ":" << error << server->errorString();
    stopServer();
}

void TcpServer::onDataAvailable(QSslSocket * socket, const QByteArray &data)
{
    qCDebug(dcTcpServerTraffic()) << "Emitting data available";
    QUuid clientId = m_clientList.key(socket);
    emit dataAvailable(clientId, data);
}

/*! Sets the name of this server to the given \a serverName. */
void TcpServer::setServerName(const QString &serverName)
{
    m_serverName = serverName;
}

/*! Returns true if this \l{TcpServer} started successfully.
 *
 * \sa TransportInterface::startServer()
 */
bool TcpServer::startServer()
{
    // open server and listen on given port
    int sockfd = 0;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        qCWarning(dcTcpServer()) << "Cannot create tcp socket";
        return false;
    }

    int flag = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
        qCWarning(dcTcpServer()) << "Cannot set SO_REUSEADDR to tcp socket";
        return false;
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(configuration().address.toString().toUtf8().data());
    serv_addr.sin_port = htons(configuration().port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(sockaddr_in)) < 0) {
        qCWarning(dcTcpServer()) << "Cannot bind tcp socket" << errno << configuration().address << configuration().port << configuration().address.toIPv4Address() << INADDR_LOOPBACK;
        return false;
    }

    if(listen(sockfd,SOMAXCONN) < 0) {
        qCWarning(dcTcpServer()) << "Tcp server error: can not listen on" << configuration().address.toString() << configuration().port;
        return false;
    }

    m_server = new SslServer(configuration().sslEnabled, m_sslConfig);

    m_server->setSocketDescriptor(sockfd);

//    if(!m_server->listen(configuration().address, static_cast<quint16>(configuration().port))) {
//        qCWarning(dcTcpServer()) << "Tcp server error: can not listen on" << configuration().address.toString() << configuration().port;
//        delete m_server;
//        m_server = nullptr;
//        return false;
//    }

    connect(m_server, SIGNAL(clientConnected(QSslSocket *)), SLOT(onClientConnected(QSslSocket *)));
    connect(m_server, SIGNAL(clientDisconnected(QSslSocket *)), SLOT(onClientDisconnected(QSslSocket *)));
    connect(m_server, &SslServer::dataAvailable, this, &TcpServer::onDataAvailable);

    qCDebug(dcTcpServer()) << "Started Tcp server" << serverUrl().toString();

    return true;
}

/*! Returns true if this \l{TcpServer} stopped successfully.
 *
 * \sa TransportInterface::startServer()
 */
bool TcpServer::stopServer()
{
    if (!m_server)
        return true;

    m_server->close();
    m_server->deleteLater();
    m_server = nullptr;
    return true;
}

/*! This method will be called if a new \a socketDescriptor is about to connect to this SslSocket. */
void SslServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket *sslSocket = new QSslSocket(this);

    qCDebug(dcTcpServer()) << "New client socket connection:" << sslSocket;

    connect(sslSocket, &QSslSocket::encrypted, [this, sslSocket](){ emit clientConnected(sslSocket); });
    connect(sslSocket, &QSslSocket::readyRead, this, &SslServer::onSocketReadyRead);
    connect(sslSocket, &QSslSocket::disconnected, this, &SslServer::onClientDisconnected);
    typedef void (QSslSocket:: *sslErrorsSignal)(const QList<QSslError> &);
    connect(sslSocket, static_cast<sslErrorsSignal>(&QSslSocket::sslErrors), this, [](const QList<QSslError> &errors) {
        qCWarning(dcTcpServer()) << "SSL Errors happened in the client connections:";
        foreach (const QSslError &error, errors) {
            qCWarning(dcTcpServer()) << "SSL Error:" << error.error() << error.errorString();
        }
    });

    if (!sslSocket->setSocketDescriptor(socketDescriptor)) {
        qCWarning(dcTcpServer()) << "Failed to set SSL socket descriptor.";
        delete sslSocket;
        return;
    }
    if (m_sslEnabled) {
        qCDebug(dcTcpServer()) << "Starting SSL encryption";
        sslSocket->setSslConfiguration(m_config);
        sslSocket->startServerEncryption();
    } else {
        emit clientConnected(sslSocket);
    }
}

void SslServer::onClientDisconnected()
{
    QSslSocket *socket = static_cast<QSslSocket*>(sender());
    qCDebug(dcTcpServer()) << "Client socket disconnected:" << socket;
    emit clientDisconnected(socket);
    socket->deleteLater();
}

void SslServer::onSocketReadyRead()
{
    QSslSocket *socket = static_cast<QSslSocket*>(sender());
    QByteArray data = socket->readAll();
    qCDebug(dcTcpServerTraffic()) << "Reading socket data:" << data;
    emit dataAvailable(socket, data);
}

}
