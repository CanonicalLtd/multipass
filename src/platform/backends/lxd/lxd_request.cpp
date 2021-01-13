/*
 * Copyright (C) 2020 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "lxd_request.h"

#include <multipass/format.h>
#include <multipass/logging/log.h>
#include <multipass/network_access_manager.h>
#include <multipass/version.h>

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>

namespace mp = multipass;
namespace mpl = multipass::logging;

namespace
{
constexpr auto request_category = "lxd request";

template <typename Callable>
const QJsonObject lxd_request_common(const std::string& method, QUrl& url, int timeout, Callable&& handle_request)
{
    QEventLoop event_loop;
    QTimer download_timeout;
    download_timeout.setInterval(timeout);

    if (url.host().isEmpty())
    {
        url.setHost(mp::lxd_project_name);
    }

    const QString project_query_string = QString("project=%1").arg(mp::lxd_project_name);
    if (url.hasQuery())
    {
        url.setQuery(url.query() + "&" + project_query_string);
    }
    else
    {
        url.setQuery(project_query_string);
    }

    mpl::log(mpl::Level::trace, request_category, fmt::format("Requesting LXD: {} {}", method, url.toString()));
    QNetworkRequest request{url};

    request.setHeader(QNetworkRequest::UserAgentHeader, QString("Multipass/%1").arg(mp::version_string));

    auto verb = QByteArray::fromStdString(method);

    auto reply = handle_request(request, verb);

    QObject::connect(reply, &QNetworkReply::finished, &event_loop, &QEventLoop::quit);
    QObject::connect(&download_timeout, &QTimer::timeout, [&]() {
        download_timeout.stop();
        reply->abort();
    });

    if (!reply->isFinished())
    {
        download_timeout.start();
        event_loop.exec();
    }

    if (reply->error() == QNetworkReply::ContentNotFoundError)
        throw mp::LXDNotFoundException();

    if (reply->error() == QNetworkReply::OperationCanceledError)
        throw mp::LXDRuntimeError(
            fmt::format("Timeout getting response for {} operation on {}", method, url.toString()));

    auto bytearray_reply = reply->readAll();
    reply->deleteLater();

    if (bytearray_reply.isEmpty())
        throw mp::LXDRuntimeError(fmt::format("Empty reply received for {} operation on {}", method, url.toString()));

    QJsonParseError json_error;
    auto json_reply = QJsonDocument::fromJson(bytearray_reply, &json_error);

    if (json_error.error != QJsonParseError::NoError)
        throw mp::LXDRuntimeError(fmt::format("Error parsing JSON response for {}: {}\n{}", url.toString(),
                                              json_error.errorString(), bytearray_reply));

    if (json_reply.isNull() || !json_reply.isObject())
        throw mp::LXDRuntimeError(fmt::format("Invalid LXD response for {}: {}", url.toString(), bytearray_reply));

    mpl::log(mpl::Level::trace, request_category, fmt::format("Got reply: {}", QJsonDocument(json_reply).toJson()));

    if (reply->error() != QNetworkReply::NoError)
        throw mp::LXDRuntimeError(fmt::format("Network error for {}: {} - {}", url.toString(), reply->errorString(),
                                              json_reply.object()["error"].toString()));

    return json_reply.object();
}
} // namespace

const QJsonObject mp::lxd_request(mp::NetworkAccessManager* manager, const std::string& method, QUrl url,
                                  const mp::optional<QJsonObject>& json_data, int timeout)
try
{
    auto handle_request = [manager, &json_data](QNetworkRequest& request, const QByteArray& verb) {
        QByteArray data;
        if (json_data)
        {
            data = QJsonDocument(*json_data).toJson(QJsonDocument::Compact);

            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.size()));

            mpl::log(mpl::Level::trace, request_category, fmt::format("Sending data: {}", data));
        }

        return manager->sendCustomRequest(request, verb, data);
    };

    return lxd_request_common(method, url, timeout, handle_request);
}
catch (const LXDRuntimeError& e)
{
    mpl::log(mpl::Level::error, request_category, e.what());

    throw;
}

const QJsonObject mp::lxd_request(mp::NetworkAccessManager* manager, const std::string& method, QUrl url,
                                  QHttpMultiPart& multi_part, int timeout)
try
{
    auto handle_request = [manager, &multi_part](QNetworkRequest& request, const QByteArray& verb) {
        request.setRawHeader("Transfer-Encoding", "chunked");

        return manager->sendCustomRequest(request, verb, &multi_part);
    };

    return lxd_request_common(method, url, timeout, handle_request);
}
catch (const LXDRuntimeError& e)
{
    mpl::log(mpl::Level::error, request_category, e.what());

    throw;
}

const QJsonObject mp::lxd_wait(mp::NetworkAccessManager* manager, const QUrl& base_url, const QJsonObject& task_data,
                               int timeout)
try
{
    QJsonObject task_reply;

    if (task_data["metadata"].toObject()["class"] == QStringLiteral("task") &&
        task_data["status_code"].toInt(-1) == 100)
    {
        QUrl task_url(QString("%1/operations/%2/wait")
                          .arg(base_url.toString())
                          .arg(task_data["metadata"].toObject()["id"].toString()));

        task_reply = lxd_request(manager, "GET", task_url, mp::nullopt, timeout);

        if (task_reply["error_code"].toInt() >= 400)
        {
            throw mp::LXDRuntimeError(fmt::format("Error waiting on operation: ({}) {}",
                                                  task_reply["error_code"].toInt(), task_reply["error"].toString()));
        }
        else if (task_reply["status_code"].toInt() >= 400)
        {
            throw mp::LXDRuntimeError(fmt::format("Failure waiting on operation: ({}) {}",
                                                  task_reply["status_code"].toInt(), task_reply["status"].toString()));
        }
        else if (task_reply["metadata"].toObject()["status_code"].toInt() >= 400)
        {
            throw mp::LXDRuntimeError(fmt::format("Operation completed with error: ({}) {}",
                                                  task_reply["metadata"].toObject()["status_code"].toInt(),
                                                  task_reply["metadata"].toObject()["err"].toString()));
        }
    }

    return task_reply;
}
catch (const LXDRuntimeError& e)
{
    mpl::log(mpl::Level::error, request_category, e.what());

    throw;
}
