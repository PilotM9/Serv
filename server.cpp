#include "server.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QDateTime>

Server::Server(quint16 port, QObject *parent)
    : QObject(parent)
    , socket(new QUdpSocket(this))
    , tickTimer(new QTimer(this))
    , timeBroadcastTimer(new QTimer(this))
    , hasRequests(false)
    , busy(false)
{
    connect(tickTimer, &QTimer::timeout, this, &Server::processTick);
    connect(timeBroadcastTimer, &QTimer::timeout, this, &Server::broadcastCurrentTime);

    if (!socket->bind(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
        connect(socket, &QUdpSocket::readyRead, this, &Server::onReadyRead);
        timeBroadcastTimer->start(1000); // Запуск таймера для тиканья каждую секунду
    }
}

Server::~Server()
{
    tickTimer->stop();
    timeBroadcastTimer->stop();
    socket->close();
}

void Server::onReadyRead()
{
    while (socket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(socket->pendingDatagramSize());
        QHostAddress senderAddress;
        quint16 senderPort;
        socket->readDatagram(data.data(), data.size(), &senderAddress, &senderPort);

        qDebug() << "Data received:" << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            qDebug() << "Received invalid JSON";
            return;
        }

        QJsonObject jsonRequest = jsonDoc.object();
        QString method = jsonRequest["method"].toString();
        QString id = jsonRequest["id"].toString();

        qDebug() << "Method received:" << method;

        QJsonObject jsonResponse;
        jsonResponse["jsonrpc"] = "2.0";
        jsonResponse["id"] = id;

        if (method == "startProcessing") {
            startProcessing();
            jsonResponse["result"] = "Started processing requests";
        } else if (method == "stopProcessing") {
            stopProcessing();
            jsonResponse["result"] = "Stopped processing requests";
        } else if (method == "setBusy" || method == "setAvailable") {
            QString responseMessage = (method == "setBusy") ? "Server is busy" : "Server is available";
            jsonResponse["result"] = responseMessage;
        } else if (method == "processRequest") {
            QJsonObject params = jsonRequest["params"].toObject();
            qint64 requestTime = QDateTime::currentSecsSinceEpoch() + 3; // Задержка 3 такта
            QString uniqueId = QString::number(requestTime);
            params["uniqueId"] = uniqueId;

            // Заявки добавляются в очередь с указанным временем обработки
            delayedRequests.enqueue(qMakePair(params, requestTime));
            this->senderAddress = senderAddress;
            this->senderPort = senderPort;
            if (!hasRequests) {
                hasRequests = true;
                tickTimer->start(1000); // Запуск таймера с интервалом в 1 секунду
            }

            jsonResponse["result"] = QString("Request will be processed with ID: %1").arg(uniqueId);
            sendJsonRpcResponse(jsonResponse, senderAddress, senderPort);
        } else {
            jsonResponse["error"] = "Unknown method";
            sendJsonRpcResponse(jsonResponse, senderAddress, senderPort);
        }
    }
}

void Server::processTick()
{
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();

    // Обработка задержанных заявок
    while (!delayedRequests.isEmpty() && delayedRequests.head().second <= currentTime) {
        QPair<QJsonObject, qint64> requestPair = delayedRequests.dequeue();
        QJsonObject request = requestPair.first;
        qint64 requestTime = requestPair.second;

        qDebug() << "Processing request with ID:" << request["uniqueId"].toString() << "scheduled for:" << requestTime;

        QJsonObject response;
        busy = true; // Устанавливаем занятость перед обработкой
        if (validateConfiguration(request["configuration"].toString()) && validatePriority(request["priority"].toString())) {
            response["result"] = "Accepted: ID " + request["uniqueId"].toString();
        } else {
            response["result"] = "Invalid request";
        }
        response["id"] = request["uniqueId"].toString();
        sendJsonRpcResponse(response, senderAddress, senderPort);
        busy = false;

        qDebug() << "Request with ID:" << request["uniqueId"].toString() << "has been processed"; // Лог завершения обработки
    }

    // Остановка обработки, если нет заявок
    if (delayedRequests.isEmpty()) {
        tickTimer->stop();
        hasRequests = false;
        qDebug() << "No more requests to process. Stopping tick timer.";
    }
}





void Server::startProcessing()
{
    hasRequests = true;
    tickTimer->start(1000); // Запуск таймера с интервалом в 1 секунду
    qDebug() << "Started processing requests";
}

void Server::stopProcessing()
{
    hasRequests = false;
    tickTimer->stop(); // Остановка таймера
    qDebug() << "Stopped processing requests";
}

void Server::writeDatagram(const QByteArray &data, const QHostAddress &address, quint16 port)
{
    qDebug() << "Sending response:" << data;
    socket->writeDatagram(data, address, port);
}

void Server::sendJsonRpcResponse(const QJsonObject &response, const QHostAddress &address, quint16 port)
{
    QJsonDocument jsonDoc(response);
    QByteArray datagram = jsonDoc.toJson();
    qDebug() << "Sending response:" << datagram;
    writeDatagram(datagram, address, port);
}

bool Server::processRequest(const QJsonObject &request, QJsonObject &response)
{
    QString id = request["id"].toString();
    QString configuration = request["configuration"].toString();
    QString priority = request["priority"].toString();
    qDebug()<<"Validating request\n";
    if (validateConfiguration(configuration) && validatePriority(priority)) {
        response["result"] = QString("Accepted: ID %1").arg(id);
        return true;
    }

    response["result"] = "Invalid request";
    return false;
}

bool Server::validateConfiguration(const QString &configuration)
{
    QStringList validConfigurations = {
        "одна строка", "две строки", "три строки", "четыре строки",
        "один столбец", "два столбца", "три столбца", "четыре столбца",
        "1х1", "1x2", "1x3", "2x3", "2x2", "3x3", "4х4", "8х8"
    };

    return validConfigurations.contains(configuration);
}

bool Server::validatePriority(const QString &priority)
{
    bool ok;
    int p = priority.toInt(&ok);
    return ok && p >= 1 && p <= 7;
}

void Server::broadcastCurrentTime()
{
    qint64 unixTime = QDateTime::currentSecsSinceEpoch();

    qDebug() << "Current tick:" << unixTime;
}
