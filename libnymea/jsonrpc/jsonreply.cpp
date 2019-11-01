#include "jsonreply.h"

/*!
    \class JsonReply
    \brief This class represents a reply for the JSON-RPC API request.

    \ingroup json
    \inmodule core

    \sa JsonHandler
*/

/*! \enum JsonReply::Type

    This enum type specifies the type of a JsonReply.

    \value TypeSync
        The response is synchronous.
    \value TypeAsync
        The response is asynchronous.
*/

/*! \fn void JsonReply::finished();
    This signal will be emitted when a JsonReply is finished. A JsonReply is finished when
    the response is ready or then the reply timed out.
*/



/*! Constructs a new \l JsonReply with the given \a type, \a handler, \a method and \a data. */
JsonReply::JsonReply(Type type, JsonHandler *handler, const QString &method, const QVariantMap &data):
    m_type(type),
    m_data(data),
    m_handler(handler),
    m_method(method),
    m_timedOut(false)
{
    connect(&m_timeout, &QTimer::timeout, this, &JsonReply::timeout);
}

/*! Returns the pointer to a new \l{JsonReply} for the given \a handler and \a data. */
JsonReply *JsonReply::createReply(JsonHandler *handler, const QVariantMap &data)
{
    return new JsonReply(TypeSync, handler, QString(), data);
}

/*! Returns the pointer to a new asynchronous \l{JsonReply} for the given \a handler and \a method. */
JsonReply *JsonReply::createAsyncReply(JsonHandler *handler, const QString &method)
{
    return new JsonReply(TypeAsync, handler, method);
}

/*! Returns the type of this \l{JsonReply}.*/
JsonReply::Type JsonReply::type() const
{
    return m_type;
}

/*! Returns the data of this \l{JsonReply}.*/
QVariantMap JsonReply::data() const
{
    return m_data;
}

/*! Sets the \a data of this \l{JsonReply}.*/
void JsonReply::setData(const QVariantMap &data)
{
    m_data = data;
}

/*! Returns the handler of this \l{JsonReply}.*/
JsonHandler *JsonReply::handler() const
{
    return m_handler;
}

/*! Returns the method of this \l{JsonReply}.*/
QString JsonReply::method() const
{
    return m_method;
}

/*! Returns the client ID of this \l{JsonReply}.*/
QUuid JsonReply::clientId() const
{
    return m_clientId;
}

/*! Sets the \a clientId of this \l{JsonReply}.*/
void JsonReply::setClientId(const QUuid &clientId)
{
    m_clientId = clientId;
}

/*! Returns the command ID of this \l{JsonReply}.*/
int JsonReply::commandId() const
{
    return m_commandId;
}

/*! Returns the \a commandId of this \l{JsonReply}.*/
void JsonReply::setCommandId(int commandId)
{
    m_commandId = commandId;
}

/*! Start the timeout timer for this \l{JsonReply}. The default timeout is 15 seconds. */
void JsonReply::startWait()
{
    m_timeout.start(30000);
}

void JsonReply::timeout()
{
    m_timedOut = true;
    emit finished();
}

/*! Returns true if this \l{JsonReply} timed out.*/
bool JsonReply::timedOut() const
{
    return m_timedOut;
}
