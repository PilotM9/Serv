#include "time_thread.h"
#include <QThread>
#include <QDateTime>

TimeThread::TimeThread(QObject *parent)
    : QThread(parent), running(true)
{
}

void TimeThread::run()
{
    while (running) {
        QThread::sleep(1); // Таймер 1 секунда

        if (running) {
            QDateTime currentTime = QDateTime::currentDateTime();
            emit tick(currentTime);
        }
    }
}

void TimeThread::stop()
{
    running = false;
}
