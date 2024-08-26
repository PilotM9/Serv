#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QQueue>
#include <QTimer>

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(quint16 port,QObject *parent = nullptr);
    ~Server();

private slots:
    void onReadyRead();
    void processTick();

private:
    void startProcessing();
    void stopProcessing();
    void writeDatagram(const QByteArray &data, const QHostAddress &address, quint16 port);
    void sendJsonRpcResponse(const QJsonObject &response, const QHostAddress &address, quint16 port);
    bool processRequest(const QJsonObject &request, QJsonObject &response);
    bool validateConfiguration(const QString &configuration);
    bool validatePriority(const QString &priority);

    QUdpSocket *socket;
    QTimer *tickTimer;
    QQueue<QJsonObject> requestQueue;
    bool hasRequests;
    bool busy;
    QHostAddress senderAddress;
    quint16 senderPort;
};

#endif // SERVER_H
