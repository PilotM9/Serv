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
    , requestCount(0)  // Инициализация счетчика заявок
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
            qint64 currentTick = QDateTime::currentSecsSinceEpoch();
            QString uniqueId = QString::number(currentTick + (++requestCount));

            params["uniqueId"] = uniqueId;

            // Заявки добавляются в очередь с указанным временем обработки
            delayedRequests.enqueue(qMakePair(params, currentTick));
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
    // Проверяем, если сервер занят, выходим из функции
    if (busy) {
        return;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();

    // Обработка задержанных заявок
    while (!delayedRequests.isEmpty() && delayedRequests.head().second <= currentTime) {
        QPair<QJsonObject, qint64> requestPair = delayedRequests.dequeue();
        QJsonObject request = requestPair.first;

        QString uniqueId = request["uniqueId"].toString();

        qDebug() << "Processing request with ID:" << uniqueId;

        // Устанавливаем флаг занятости перед обработкой
        busy = true;

        QJsonObject response;
        if (validateConfiguration(request["configuration"].toString()) && validatePriority(request["priority"].toString())) {
            response["result"] = QString("Accepted: ID %1").arg(uniqueId);
            response["requestBody"] = request;  // Добавляем тело заявки в ответ
        } else {
            response["result"] = "Invalid request";
            response["requestBody"] = request;  // Все равно добавляем тело заявки для информации
        }
        response["id"] = uniqueId;
        sendJsonRpcResponse(response, senderAddress, senderPort);

        // Лог завершения обработки
        qDebug() << "Request with ID:" << uniqueId << " has been processed";
        qDebug() << "Request body:" << request;  // Лог тела заявки

        // Убираем флаг занятости после завершения обработки
        busy = false;

        // Уменьшаем счетчик заявок после обработки
        --requestCount;

        // Прерываем цикл, чтобы обработать только одну заявку за тик
        break;
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
