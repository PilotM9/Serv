#include "server.h"
#include "time_thread.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QDateTime>

Server::Server(quint16 port, QObject *parent)
    : QObject(parent)
    , socket(new QUdpSocket(this))
    , timeThread(new TimeThread(this))
    , hasRequests(false)
    , busy(false)
    , requestCount(0)  // Инициализация счетчика заявок
{
    connect(timeThread, &TimeThread::tick, this, &Server::processTick);

    if (!socket->bind(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
        connect(socket, &QUdpSocket::readyRead, this, &Server::onReadyRead);
        timeThread->start();  // Запуск отдельного потока для управления временем
    }
}

Server::~Server()
{
    timeThread->stop();  // Остановка потока перед удалением
    timeThread->wait();  // Ожидание завершения потока
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
        QString id = jsonRequest["id"].toString();  // Получаем ID

        qDebug() << "Method received:" << method;
        qDebug() << "Request ID received:" << id;  // Выводим ID для проверки

        QJsonObject jsonResponse;
        jsonResponse["jsonrpc"] = "2.0";
        jsonResponse["id"] = id;  // Вставляем ID в ответ

        if (method == "processRequest") {
            QJsonObject params = jsonRequest["params"].toObject();

            // Сохраняем информацию о клиенте и его запросе, включая ID
            ClientInfo clientInfo = {senderAddress, senderPort};
            delayedRequests.enqueue(qMakePair(params, clientInfo));

            if (!hasRequests) {
                hasRequests = true;
            }

            jsonResponse["result"] = QString("Request will be processed with ID: %1").arg(id);
            sendJsonRpcResponse(jsonResponse, senderAddress, senderPort);
        } else {
            jsonResponse["error"] = "Unknown method";
            sendJsonRpcResponse(jsonResponse, senderAddress, senderPort);
        }
    }
}





void Server::startProcessing()
{
    hasRequests = true;
    qDebug() << "Started processing requests";
}

void Server::stopProcessing()
{
    hasRequests = false;
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
