#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QQueue>
#include <QPair>
#include <QJsonObject>
#include <QHostAddress>

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(quint16 port, QObject *parent = nullptr);
    ~Server();

public slots:
    void onReadyRead();
    void processTick();
    void startProcessing();
    void stopProcessing();
    void broadcastCurrentTime();

private:
    void writeDatagram(const QByteArray &data, const QHostAddress &address, quint16 port);
    void sendJsonRpcResponse(const QJsonObject &response, const QHostAddress &address, quint16 port);
    bool processRequest(const QJsonObject &request, QJsonObject &response);
    bool validateConfiguration(const QString &configuration);
    bool validatePriority(const QString &priority);

private:
    QUdpSocket *socket;
    QTimer *tickTimer;
    QTimer *timeBroadcastTimer;
    QQueue<QJsonObject> requestQueue;
    QQueue<QPair<QJsonObject, qint64>> delayedRequests; // Очередь для отложенных заявок
    QHostAddress senderAddress;
    quint16 senderPort;
    bool hasRequests;
    bool busy;
    int requestCount;  // Счетчик заявок
};

#endif // SERVER_H
