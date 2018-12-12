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

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QUrlQuery>
#include <QString>
#include <QHash>

namespace nymeaserver {

class HttpRequest
{
public:
    enum RequestMethod {
        Get,
        Post,
        Put,
        Delete,
        Options,
        Unhandled
    };

    HttpRequest();
    HttpRequest(QByteArray rawData);

    QByteArray rawHeader() const;
    QHash<QByteArray, QByteArray> rawHeaderList() const;

    RequestMethod method() const;
    QString methodString() const;
    QByteArray httpVersion() const;

    QUrl url() const;
    QUrlQuery urlQuery() const;

    QByteArray payload() const;

    bool isValid() const;
    bool isComplete() const;
    bool hasPayload() const;

    void appendData(const QByteArray &data);

private:
    QByteArray m_rawData;
    QByteArray m_rawHeader;
    QHash<QByteArray, QByteArray> m_rawHeaderList;

    RequestMethod m_method;
    QString m_methodString;
    QByteArray m_httpVersion;

    QUrl m_url;
    QUrlQuery m_urlQuery;

    QByteArray m_payload;

    bool m_valid;
    bool m_isComplete;

    void validate();
    RequestMethod getRequestMethodType(const QString &methodString);
};

QDebug operator<< (QDebug debug, const HttpRequest &httpRequest);

}
#endif // HTTPREQUEST_H
