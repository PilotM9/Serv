#include "server.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>

Server::Server(quint16 port, QObject *parent)
    : QObject(parent)
    , tickTimer(new QTimer(this))
    , socket(new QUdpSocket(this))
    , hasRequests(false)
    , busy(false)
{
    connect(tickTimer, &QTimer::timeout, this, &Server::processTick);

    if (!socket->bind(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
        connect(socket, &QUdpSocket::readyRead, this, &Server::onReadyRead);
    }
}

Server::~Server()
{
    tickTimer->stop();
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
            if (processRequest(params, jsonResponse)) {
                requestQueue.enqueue(params);
                this->senderAddress = senderAddress;
                this->senderPort = senderPort;
                if (!hasRequests) {
                    hasRequests = true;
                    tickTimer->start(256);
                }
            } else {
                jsonResponse["result"] = "Invalid request";
                sendJsonRpcResponse(jsonResponse, senderAddress, senderPort);
            }
        } else {
            jsonResponse["error"] = "Unknown method";
            sendJsonRpcResponse(jsonResponse, senderAddress, senderPort);
        }
    }
}

void Server::processTick()
{
    if (!busy && !requestQueue.isEmpty()) {
        QJsonObject request = requestQueue.dequeue();
        QJsonObject response;
        busy = true; // Set busy before processing
        if (processRequest(request, response)) {
            response["result"] = "Accepted: ID " + request["id"].toString();
        } else {
            response["result"] = "Invalid request";
        }
        response["id"] = request["id"];
        sendJsonRpcResponse(response, senderAddress, senderPort);
        busy = false;

        if (requestQueue.isEmpty()) {
            tickTimer->stop();
            hasRequests = false;
        }
    }
}

void Server::startProcessing()
{
    hasRequests = true;
    tickTimer->start(256); // Start the timer with 128 ms interval
    qDebug() << "Started processing requests";
}

void Server::stopProcessing()
{
    hasRequests = false;
    tickTimer->stop(); // Stop the timer
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
