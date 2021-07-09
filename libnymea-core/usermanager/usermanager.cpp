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
    \class nymeaserver::UserManager
    \brief This class represents the manager for the users in nymead.

    \ingroup user
    \inmodule core

    The user manager is responsible for managing the user database, tokens and authentication. The user manager
    creates a user database where all relevant information will be stored.

    \sa TokenInfo, PushButtonDBusService
*/

/*! \enum nymeaserver::UserManager::UserError

    This enum represents the possible errors the \l{UserManager} can have.

    \value UserErrorNoError
        No error occurred. Everything is ok.4
    \value UserErrorBackendError
        Something went wrong in the manager. This is probably caused by a database error.
    \value UserErrorInvalidUserId
        The given user name is not valid.
    \value UserErrorDuplicateUserId
        The given user name already exits. Please use a different user name.
    \value UserErrorBadPassword
        The given password is to weak. Please use a stronger password.
    \value UserErrorTokenNotFound
        The given token is unknown to the UserManager.
    \value UserErrorPermissionDenied
        The permission is denied. Either invalid username, password or token.
*/

/*! \fn void nymeaserver::UserManager::pushButtonAuthFinished(int transactionId, bool success, const QByteArray &token);
    This signal is emitted when the push authentication for the given \a transactionId is finished.
    If \a success is true, the resulting \a token contains a non empty string.

    \sa requestPushButtonAuth
*/

#include "usermanager.h"
#include "nymeasettings.h"
#include "loggingcategories.h"
#include "pushbuttondbusservice.h"
#include "nymeacore.h"

#include <QUuid>
#include <QCryptographicHash>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QRegExpValidator>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

namespace nymeaserver {

/*! Constructs a new UserManager with the given \a dbName and \a parent. */
UserManager::UserManager(const QString &dbName, QObject *parent):
    QObject(parent)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), "users");
    m_db.setDatabaseName(dbName);

    qCDebug(dcUserManager()) << "Opening user database" << m_db.databaseName();

    if (!m_db.isValid()) {
        qCWarning(dcUserManager()) << "The database is not valid:" << m_db.lastError().driverText() << m_db.lastError().databaseText();
        rotate(m_db.databaseName());
    }

    if (!initDB()) {
        qCWarning(dcUserManager()) << "Error initializing user database. Trying to correct it.";
        if (QFileInfo(m_db.databaseName()).exists()) {
            rotate(m_db.databaseName());
            if (!initDB()) {
                qCWarning(dcUserManager()) << "Error fixing user database. Giving up. Users can't be stored.";
            }
        }
    }

    m_pushButtonDBusService = new PushButtonDBusService("/io/guh/nymead/UserManager", this);
    connect(m_pushButtonDBusService, &PushButtonDBusService::pushButtonPressed, this, &UserManager::onPushButtonPressed);
    m_pushButtonTransaction = qMakePair<int, QString>(-1, QString());
}

/*! Will return true if the database is working fine but doesn't have any information on users whatsoever.
 *  That is, neither a user nor an anonymous token.
 *  This may be used to determine whether a first-time setup is required.
 */
bool UserManager::initRequired() const
{
    QString getTokensQuery = QString("SELECT id, username, creationdate, deviceName FROM tokens;");
    QSqlQuery result = m_db.exec(getTokensQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Query for tokens failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << getTokensQuery;
        // Note: do not return true in case the database access fails.
        return false;
    }

    return users().isEmpty() && !result.first();
}

/*! Returns the list of user names for this UserManager. */
UserInfoList UserManager::users() const
{
    QString userQuery("SELECT username, scopes FROM users;");
    QSqlQuery result = m_db.exec(userQuery);
    UserInfoList users;
    while (result.next()) {
        UserInfo info = UserInfo(result.value("username").toString());
        info.setScopes(Types::scopesFromStringList(result.value("scopes").toString().split(',')));
        users.append(info);
    }
    return users;
}

/*! Creates a new user with the given \a username and \a password. Returns the \l UserError to inform about the result. */
UserManager::UserError UserManager::createUser(const QString &username, const QString &password, Types::PermissionScopes scopes)
{
    if (!validateUsername(username)) {
        qCWarning(dcUserManager) << "Error creating user. Invalid username:" << username;
        return UserErrorInvalidUserId;
    }

    if (!validatePassword(password)) {
        qCWarning(dcUserManager) << "Password failed character validation. Must contain a letter, a number and a special charactar. Minimum length: 8";
        return UserErrorBadPassword;
    }

    QString checkForDuplicateUserQuery = QString("SELECT * FROM users WHERE lower(username) = \"%1\";").arg(username.toLower());
    QSqlQuery result = m_db.exec(checkForDuplicateUserQuery);
    if (result.first()) {
        qCWarning(dcUserManager) << "Username already in use";
        return UserErrorDuplicateUserId;
    }

    QByteArray salt = QUuid::createUuid().toString().remove(QRegExp("[{}]")).toUtf8();
    QByteArray hashedPassword = QCryptographicHash::hash(QString(password + salt).toUtf8(), QCryptographicHash::Sha512).toBase64();
    QString queryString = QString("INSERT INTO users(username, password, salt, scopes) values(\"%1\", \"%2\", \"%3\", \"%4\");")
            .arg(username.toLower())
            .arg(QString::fromUtf8(hashedPassword))
            .arg(QString::fromUtf8(salt))
            .arg(Types::scopesToStringList(scopes).join(','));
    m_db.exec(queryString);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Error creating user:" << m_db.lastError().databaseText() << m_db.lastError().driverText();
        return UserErrorBackendError;
    }

    qCInfo(dcUserManager()) << "New user" << username << "added to the system with permissions:" << Types::scopesToStringList(scopes);
    emit userAdded(username);
    return UserErrorNoError;
}

UserManager::UserError UserManager::changePassword(const QString &username, const QString &newPassword)
{
    if (!validateUsername(username)) {
        qCWarning(dcUserManager) << "Invalid username:" << username;
        return UserErrorInvalidUserId;
    }

    if (!validatePassword(newPassword)) {
        qCWarning(dcUserManager) << "Password failed character validation. Must contain a letter, a number and a special charactar. Minimum length: 8";
        return UserErrorBadPassword;
    }

    QString checkForUserExistingQuery = QString("SELECT * FROM users WHERE lower(username) = \"%1\";").arg(username.toLower());
    QSqlQuery result = m_db.exec(checkForUserExistingQuery);
    if (!result.first()) {
        qCWarning(dcUserManager) << "Username does not exist.";
        return UserErrorInvalidUserId;
    }

    // Update the password
    QByteArray salt = QUuid::createUuid().toString().remove(QRegExp("[{}]")).toUtf8();
    QByteArray hashedPassword = QCryptographicHash::hash(QString(newPassword + salt).toUtf8(), QCryptographicHash::Sha512).toBase64();
    QString queryString = QString("UPDATE users SET password = \"%1\", salt = \"%2\" WHERE lower(username) = \"%3\";")
            .arg(QString::fromUtf8(hashedPassword))
            .arg(QString::fromUtf8(salt))
            .arg(username.toLower());
    m_db.exec(queryString);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Error updating password for user:" << m_db.lastError().databaseText() << m_db.lastError().driverText();
        return UserErrorBackendError;
    }
    qCDebug(dcUserManager()) << "Password updated for user" << username;
    return UserErrorNoError;
}

UserManager::UserError UserManager::removeUser(const QString &username)
{
    if (!username.isEmpty()) {
        QString dropUserQuery = QString("DELETE FROM users WHERE lower(username) =\"%1\";").arg(username.toLower());
        QSqlQuery result = m_db.exec(dropUserQuery);
        if (result.numRowsAffected() == 0) {
            return UserErrorInvalidUserId;
        }
    }

    QString dropTokensQuery = QString("DELETE FROM tokens WHERE lower(username) = \"%1\";").arg(username.toLower());
    m_db.exec(dropTokensQuery);

    emit userRemoved(username);
    return UserErrorNoError;
}

UserManager::UserError UserManager::setUserScopes(const QString &username, Types::PermissionScopes scopes)
{
    QString scopesString = Types::scopesToStringList(scopes).join(',');
    QString setScopesQuery = QString("UPDATE users SET scopes = '%1' WHERE username = '%2'").arg(scopesString).arg(username);
    QSqlQuery result = m_db.exec(setScopesQuery);
    if (result.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager()) << "Error updating scopes for user" << username << m_db.lastError().databaseText() << m_db.lastError().driverText();
        return UserErrorBackendError;
    }

    emit userChanged(username);
    return UserErrorNoError;
}

/*! Returns true if the push button authentication is available for this system. */
bool UserManager::pushButtonAuthAvailable() const
{
    return m_pushButtonDBusService->agentAvailable();
}

/*! Authenticated the given \a username with the given \a password for the \a deviceName. If the authentication was
    successful, the token will be returned, otherwise the return value will be an empty byte array.
*/
QByteArray UserManager::authenticate(const QString &username, const QString &password, const QString &deviceName)
{
    if (!validateUsername(username)) {
        qCWarning(dcUserManager) << "Username did not pass validation:" << username;
        return QByteArray();
    }

    QString passwordQuery = QString("SELECT password, salt FROM users WHERE lower(username) = \"%1\";").arg(username.toLower());
    QSqlQuery result = m_db.exec(passwordQuery);
    if (!result.first()) {
        qCWarning(dcUserManager) << "No such username" << username;
        return QByteArray();
    }
    QByteArray salt = result.value("salt").toByteArray();
    QByteArray hashedPassword = result.value("password").toByteArray();

    if (hashedPassword != QCryptographicHash::hash(QString(password + salt).toUtf8(), QCryptographicHash::Sha512).toBase64()) {
        qCWarning(dcUserManager) << "Authentication error for user:" << username;
        return QByteArray();
    }

    QByteArray token = QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha256).toBase64();
    QString storeTokenQuery = QString("INSERT INTO tokens(id, username, token, creationdate, devicename) VALUES(\"%1\", \"%2\", \"%3\", \"%4\", \"%5\");")
            .arg(QUuid::createUuid().toString())
            .arg(username.toLower())
            .arg(QString::fromUtf8(token))
            .arg(NymeaCore::instance()->timeManager()->currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(deviceName);

    m_db.exec(storeTokenQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Error storing token in DB:" << m_db.lastError().databaseText() << m_db.lastError().driverText();
        return QByteArray();
    }
    return token;
}

/*! Start the push button authentication for the device with the given \a deviceName. Returns the transaction id as refference to the request. */
int UserManager::requestPushButtonAuth(const QString &deviceName)
{
    if (m_pushButtonTransaction.first != -1) {
        qCWarning(dcUserManager()) << "PushButton authentication already in progress for device" << m_pushButtonTransaction.second << ". Cancelling...";
        cancelPushButtonAuth(m_pushButtonTransaction.first);
    }

    qCDebug(dcUserManager()) << "Starting PushButton authentication for device" << deviceName;
    int transactionId = ++m_pushButtonTransactionIdCounter;
    m_pushButtonTransaction = qMakePair<int, QString>(transactionId, deviceName);
    return transactionId;
}

/*! Cancel the push button authentication with the given \a transactionId.

    \sa requestPushButtonAuth
*/
void UserManager::cancelPushButtonAuth(int transactionId)
{
    if (m_pushButtonTransaction.first == -1) {
        qCWarning(dcUserManager()) << "No PushButton transaction in progress. Nothing to cancel.";
        return;
    }
    if (m_pushButtonTransaction.first != transactionId) {
        qCWarning(dcUserManager()) << "PushButton transaction" << transactionId << "not in progress. Cannot cancel.";
        return;
    }
    qCDebug(dcUserManager()) << "Cancelling PushButton transaction for device:" << m_pushButtonTransaction.second;
    emit pushButtonAuthFinished(m_pushButtonTransaction.first, false, QByteArray());
    m_pushButtonTransaction.first = -1;

}

/*! Request UserInfo.
 The UserInfo for the given username is returned.
*/
UserInfo UserManager::userInfo(const QString &username) const
{

    QString getUserQuery = QString("SELECT username, scopes FROM users WHERE lower(username) = \"%1\";")
            .arg(username);
    QSqlQuery result = m_db.exec(getUserQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Query for user" << username << "failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << getUserQuery;
        return UserInfo();
    }

    if (!result.first()) {
        return UserInfo();
    }

    UserInfo userInfo = UserInfo(result.value("username").toString());
    userInfo.setScopes(Types::scopesFromStringList(result.value("scopes").toString().split(',')));

    return userInfo;
}

QList<TokenInfo> UserManager::tokens(const QString &username) const
{
    QList<TokenInfo> ret;

    if (!validateUsername(username)) {
        qCWarning(dcUserManager) << "Username did not pass validation:" << username;
        return ret;
    }
    QString getTokensQuery = QString("SELECT id, username, creationdate, deviceName FROM tokens WHERE lower(username) = \"%1\";")
            .arg(username.toLower());
    QSqlQuery result = m_db.exec(getTokensQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Query for tokens failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << getTokensQuery;
        return ret;
    }

    while (result.next()) {
        ret << TokenInfo(result.value("id").toUuid(), result.value("username").toString(), result.value("creationdate").toDateTime(), result.value("devicename").toString());
    }
    return ret;
}

TokenInfo UserManager::tokenInfo(const QByteArray &token) const
{
    if (!validateToken(token)) {
        qCWarning(dcUserManager) << "Token did not pass validation:" << token;
        return TokenInfo();
    }

    QString getTokenQuery = QString("SELECT id, username, creationdate, deviceName FROM tokens WHERE token = \"%1\";")
            .arg(QString::fromUtf8(token));
    QSqlQuery result = m_db.exec(getTokenQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Query for token failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << getTokenQuery;
        return TokenInfo();
    }

    if (!result.first()) {
        return TokenInfo();
    }
    return TokenInfo(result.value("id").toUuid(), result.value("username").toString(), result.value("creationdate").toDateTime(), result.value("devicename").toString());
}

TokenInfo UserManager::tokenInfo(const QUuid &tokenId) const
{

    QString getTokenQuery = QString("SELECT id, username, creationdate, deviceName FROM tokens WHERE id = \"%1\";")
            .arg(tokenId.toString());
    QSqlQuery result = m_db.exec(getTokenQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Query for token failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << getTokenQuery;
        return TokenInfo();
    }

    if (!result.first()) {
        return TokenInfo();
    }
    return TokenInfo(result.value("id").toUuid(), result.value("username").toString(), result.value("creationdate").toDateTime(), result.value("devicename").toString());
}

/*! Removes the token with the given \a tokenId. Returns \l{UserError} to inform about the result. */
UserManager::UserError UserManager::removeToken(const QUuid &tokenId)
{
    QString removeTokenQuery = QString("DELETE FROM tokens WHERE id = \"%1\";")
            .arg(tokenId.toString());
    QSqlQuery result = m_db.exec(removeTokenQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Removing token failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << removeTokenQuery;
        return UserErrorBackendError;
    }
    if (result.numRowsAffected() != 1) {
        qCWarning(dcUserManager) << "Token not found in DB";
        return UserErrorTokenNotFound;
    }

    qCDebug(dcUserManager) << "Token" << tokenId << "removed from DB";
    return UserErrorNoError;
}

/*! Returns true, if the given \a token is valid. */
bool UserManager::verifyToken(const QByteArray &token)
{
    if (!validateToken(token)) {
        qCWarning(dcUserManager) << "Token failed character validation" << token;
        return false;
    }
    QString getTokenQuery = QString("SELECT * FROM tokens WHERE token = \"%1\";")
            .arg(QString::fromUtf8(token));
    QSqlQuery result = m_db.exec(getTokenQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager) << "Query for token failed:" << m_db.lastError().databaseText() << m_db.lastError().driverText() << getTokenQuery;
        return false;
    }
    if (!result.first()) {
        qCDebug(dcUserManager) << "Authorization failed for token" << token;
        return false;
    }
    //qCDebug(dcUserManager) << "Token authorized for user" << result.value("username").toString();
    return true;
}

bool UserManager::initDB()
{
    m_db.close();

    if (!m_db.open()) {
        dumpDBError("Can't open user database. Init failed.");
        return false;
    }

    int currentVersion = -1;
    int newVersion = 1;
    if (m_db.tables().contains("metadata")) {
        QSqlQuery query = m_db.exec("SELECT data FROM metadata WHERE `key` = 'version';");
        if (query.next()) {
            currentVersion = query.value("data").toInt();
        }
    }

    if (!m_db.tables().contains("users")) {
        qCDebug(dcUserManager()) << "Empty user database. Setting up metadata...";
        m_db.exec("CREATE TABLE users (username VARCHAR(40) UNIQUE PRIMARY KEY, password VARCHAR(100), salt VARCHAR(100), scopes TEXT);");
        if (m_db.lastError().isValid()) {
            dumpDBError("Error initializing user database (table users).");
            m_db.close();
            return false;
        }
    } else if (currentVersion < 1) {
        m_db.exec("ALTER TABLE users ADD COLUMN scopes TEXT;");
        if (m_db.lastError().isValid()) {
            dumpDBError("Error migrating user database (table users).");
            m_db.close();
            return false;
        }
        // Migrated existing users from before multiuser support are admins by default
        m_db.exec(QString("UPDATE users SET scopes = '%1';").arg(Types::scopesToStringList(Types::PermissionScopeAdmin).join(',')));
        if (m_db.lastError().isValid()) {
            dumpDBError("Error migrating user database (updating existing users).");
            m_db.close();
            return false;
        }
    }

    if (!m_db.tables().contains("tokens")) {
        qCDebug(dcUserManager()) << "Empty user database. Setting up metadata...";
        m_db.exec("CREATE TABLE tokens (id VARCHAR(40) UNIQUE, username VARCHAR(40), token VARCHAR(100) UNIQUE, creationdate DATETIME, devicename VARCHAR(40));");
        if (m_db.lastError().isValid()) {
            dumpDBError("Error initializing user database (table tokens).");
            m_db.close();
            return false;
        }
    }

    if (m_db.tables().contains("metadata")) {
        if (currentVersion < newVersion) {
            m_db.exec(QString("UPDATE metadata SET data = %1 WHERE `key` = 'version')").arg(newVersion));
            if (m_db.lastError().isValid()) {
                dumpDBError("Error updating up user database schema version!");
                m_db.close();
                return false;
            }
            qCInfo(dcUserManager()) << "Successfully migrated user database.";
        }
    } else {
        m_db.exec("CREATE TABLE metadata (`key` VARCHAR(10), data VARCHAR(40));");
        if (m_db.lastError().isValid()) {
            dumpDBError("Error setting up user database (table metadata)!");
            m_db.close();
            return false;
        }
        m_db.exec(QString("INSERT INTO metadata (`key`, `data`) VALUES ('version', %1);").arg(newVersion));
        if (m_db.lastError().isValid()) {
            dumpDBError("Error setting up user database (setting version metadata)!");
            m_db.close();
            return false;
        }
        qCInfo(dcUserManager()) << "Successfully initialized user database.";
    }

    qCDebug(dcUserManager()) << "User database initialized successfully.";
    return true;
}

void UserManager::rotate(const QString &dbName)
{
    int index = 1;
    while (QFileInfo(QString("%1.%2").arg(dbName).arg(index)).exists()) {
        index++;
    }
    qCDebug(dcUserManager()) << "Backing up old database file to" << QString("%1.%2").arg(dbName).arg(index);
    QFile f(dbName);
    if (!f.rename(QString("%1.%2").arg(dbName).arg(index))) {
        qCWarning(dcUserManager()) << "Error backing up old database.";
    } else {
        qCDebug(dcUserManager()) << "Successfully moved old database";
    }
}

bool UserManager::validateUsername(const QString &username) const
{
    QRegExp validator("(^[a-zA-Z0-9_\\.+-]+@[a-zA-Z0-9-_]+(\\.[a-zA-Z]+){1,2}$)");
    return validator.exactMatch(username);
}

bool UserManager::validatePassword(const QString &password) const
{
    if (password.length() < 8) {
        return false;
    }
    if (!password.contains(QRegExp("[a-z]"))) {
        return false;
    }
    if (!password.contains(QRegExp("[A-Z]"))) {
        return false;
    }
    if (!password.contains(QRegExp("[0-9]"))) {
        return false;
    }
    return true;
}

bool UserManager::validateToken(const QByteArray &token) const
{
    QRegExp validator(QRegExp("(^[a-zA-Z0-9_\\.+-/=]+$)"));
    return validator.exactMatch(token);
}

void UserManager::dumpDBError(const QString &message)
{
    qCCritical(dcUserManager) << message << "Driver error:" << m_db.lastError().driverText() << "Database error:" << m_db.lastError().databaseText();
}

void UserManager::onPushButtonPressed()
{
    if (m_pushButtonTransaction.first == -1) {
        qCDebug(dcUserManager()) << "PushButton pressed without a client waiting for it. Ignoring the signal.";
        return;
    }

    QByteArray token = QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha256).toBase64();
    QString storeTokenQuery = QString("INSERT INTO tokens(id, username, token, creationdate, devicename) VALUES(\"%1\", \"%2\", \"%3\", \"%4\", \"%5\");")
            .arg(QUuid::createUuid().toString())
            .arg("")
            .arg(QString::fromUtf8(token))
            .arg(NymeaCore::instance()->timeManager()->currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(m_pushButtonTransaction.second);

    m_db.exec(storeTokenQuery);
    if (m_db.lastError().type() != QSqlError::NoError) {
        qCWarning(dcUserManager()) << "Error storing token in DB:" << m_db.lastError().databaseText() << m_db.lastError().driverText();
        qCWarning(dcUserManager()) << "PushButton Auth failed.";
        emit pushButtonAuthFinished(m_pushButtonTransaction.first, false, QByteArray());
    } else {
        qCDebug(dcUserManager()) << "PushButton Auth succeeded.";
        emit pushButtonAuthFinished(m_pushButtonTransaction.first, true, token);
    }

    m_pushButtonTransaction.first = -1;
}

}
