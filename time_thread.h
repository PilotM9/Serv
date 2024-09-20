#ifndef TIME_THREAD_H
#define TIME_THREAD_H

#include <QThread>
#include <QDateTime>

class TimeThread : public QThread
{
    Q_OBJECT

public:
    explicit TimeThread(QObject *parent = nullptr);
    void run() override;
    void stop(); // Добавьте эту строку

signals:
    void tick(const QDateTime &currentTime);

private:
    bool running;
};

#endif // TIME_THREAD_H
