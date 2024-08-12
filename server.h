#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QQueue>

class Server : public QTcpServer
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void processTick();
    void startProcessingRequests();

private:
    QTcpSocket *clientSocket;
    bool busy;  // Флаг, указывающий, занят ли сервер
    QQueue<QString> requestQueue;  // Очередь заявок

    bool processRequest(const QByteArray &data, QString &response);
    bool validateConfiguration(const QString &configuration);
    bool validatePriority(const QString &priority);
    void sendRequest(const QString &request);
};

#endif // SERVER_H
