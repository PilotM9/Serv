#include "Server.h"
#include <QDebug>
#include <QStringList>

Server::Server(QObject *parent)
    : QTcpServer(parent), clientSocket(nullptr), busy(false)
{
    connect(this, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!listen(QHostAddress::Any, 1234)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
    }
}

Server::~Server()
{
    if (clientSocket) {
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
    }
}

void Server::onNewConnection()
{
    if (clientSocket) {
        qDebug() << "Disconnecting old client";
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
    }

    clientSocket = nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onDisconnected);
    qDebug() << "Client connected";
}

void Server::onReadyRead()
{
    if (clientSocket) {
        QByteArray data = clientSocket->readAll();

        if (data == "TICK") {
            processTick();
        } else if (data == "START_PROCESSING") {
            startProcessingRequests();
        } else if (data == "BUSY") {
            busy = true;
            clientSocket->write("Server is busy");
            clientSocket->flush();
        } else if (data == "AVAILABLE") {
            busy = false;
            clientSocket->write("Server is available");
            clientSocket->flush();
        } else {
            if (busy) {
                // Если сервер занят, не принимаем заявки
                clientSocket->write("Server is busy, cannot process request");
                clientSocket->flush();
            } else {
                QString response;
                if (processRequest(data, response)) {
                    clientSocket->write(response.toUtf8());
                } else {
                    clientSocket->write("Invalid request");
                }
                clientSocket->flush();
            }
        }
    } else {
        qDebug() << "clientSocket is nullptr in onReadyRead()!";
    }
}



void Server::onDisconnected()
{
    qDebug() << "Client disconnected";
    if (clientSocket) {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }
}

void Server::processTick()
{
    if (!busy) {
        // Сервер не занят, готов к обработке
        clientSocket->write("ACCEPTED");
        clientSocket->flush();
        busy = true; // Устанавливаем флаг, что сервер занят
    } else {
        clientSocket->write("BUSY");
        clientSocket->flush();
    }
}


void Server::startProcessingRequests()
{
    if (!requestQueue.isEmpty() && !busy) {
        // Запускаем обработку первой заявки в очереди
        QString request = requestQueue.dequeue();
        sendRequest(request);
    }
}

void Server::sendRequest(const QString &request)
{
    // Логика для обработки заявки
    QString response;
    if (processRequest(request.toUtf8(), response)) {
        clientSocket->write(response.toUtf8());
    } else {
        clientSocket->write("Invalid request");
    }
    clientSocket->flush();
}

bool Server::processRequest(const QByteArray &data, QString &response)
{
    QStringList parts = QString(data).split(';');

    // Ожидаем, что parts[0] - это id, parts[1] - configuration, parts[2] - priority
    if (parts.size() == 3) {
        QString id = parts[0].trimmed();
        QString configuration = parts[1].trimmed();
        QString priority = parts[2].trimmed();

        if (validateConfiguration(configuration) && validatePriority(priority)) {
            response = QString("Accepted: ID %1").arg(id);
            return true;
        }
    }

    response = "Invalid request";
    return false;
}


bool Server::validateConfiguration(const QString &configuration)
{
    QStringList validConfigurations = {
        "одна строка", "две строки", "три строки", "четыре строки",
        "один столбец", "два столбца", "три столбца", "четыре столбца",
        "1х1", "1x2", "1x3",
        "2x3", "2x2", "3x3",
        "4х4", "8х8"
    };

    return validConfigurations.contains(configuration);
}

bool Server::validatePriority(const QString &priority)
{
    bool ok;
    int p = priority.toInt(&ok);
    return ok && p >= 1 && p <= 7;
}
