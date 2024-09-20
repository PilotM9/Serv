#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QQueue>
#include <QPair>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHostAddress>
#include <QDateTime>
#include "time_thread.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(quint16 port, QObject *parent = nullptr);
    ~Server();

private slots:
    void onReadyRead();
    void processTick(const QDateTime &currentTime);

private:
    struct ClientInfo {
        QHostAddress address;
        quint16 port;
    };

    void startProcessing();
    void stopProcessing();
    void writeDatagram(const QByteArray &data, const QHostAddress &address, quint16 port);
    void sendJsonRpcResponse(const QJsonObject &response, const QHostAddress &address, quint16 port);
    bool validateConfiguration(const QString &configuration);
    bool validatePriority(const QString &priority);

    QUdpSocket *socket;
    TimeThread *timeThread;
    QQueue<QPair<QJsonObject, ClientInfo>> delayedRequests;
    bool hasRequests;
    bool busy;
    int requestCount;
};

#endif // SERVER_H
