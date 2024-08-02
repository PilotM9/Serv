// server.h
#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>

class Server : public QTcpServer
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    ~Server();



private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *socket;
    bool processRequest(const QByteArray &data, QString &response);
    bool validateConfiguration(const QString &configuration);
    bool validatePriority(const QString &priority);
};

#endif // SERVER_H
