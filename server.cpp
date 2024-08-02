// server.cpp
#include "server.h"
#include <QDebug>
#include <QStringList>

Server::Server(QObject *parent)
    : QTcpServer(parent), socket(nullptr)
{
    connect(this, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket *clientSocket = nextPendingConnection();
        connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
            QByteArray data = clientSocket->readAll();
            QString response;


            if (processRequest(data, response)) {

                clientSocket->write(response.toUtf8());
            } else {
                clientSocket->write("Invalid request");
            }
        });
        connect(clientSocket, &QTcpSocket::disconnected, this, [this, clientSocket]() {
            qDebug() << "Client disconnected";
            clientSocket->deleteLater();
        });
        qDebug() << "Client connected";
    });

    if (!listen(QHostAddress::Any, 1234)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
    }
}

Server::~Server()
{
    if (socket) {
        socket->disconnectFromHost();
        socket->deleteLater();
    }
}

// Убедитесь, что этот метод удален, если не используется
// void Server::incomingConnection(qintptr socketDescriptor)
// {
//     // Этот метод больше не используется с текущим подходом
// }

void Server::onReadyRead()
{
    // Этот метод должен быть определен, если он используется
}

void Server::onDisconnected()
{
    // Этот метод должен быть определен, если он используется
}

bool Server::processRequest(const QByteArray &data, QString &response)
{
    QStringList parts = QString(data).split(';');

    if (parts.size() == 2) {
        QString configuration = parts[0].trimmed();
        QString priority = parts[1].trimmed();

        if (validateConfiguration(configuration) && validatePriority(priority)) {
            response = "Accepted";
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
        "один прожектор", "прямоугольник 1x2", "прямоугольник 1x3",
        "прямоугольник 2x3", "квадрат 2x2", "квадрат 3x3",
        "квадрат 4x4", "квадрат 8x8"
    };


    return validConfigurations.contains(configuration);
}

bool Server::validatePriority(const QString &priority)
{
    bool ok;
    int p = priority.toInt(&ok);
    return ok && p >= 1 && p <= 7;
}
