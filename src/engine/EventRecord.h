#ifndef EVENTRECORD_H
#define EVENTRECORD_H

#include <QString>
#include <QDateTime>

struct EventRecord {
    QString deviceName;
    QString eventType;
    QDateTime timestamp;
};

#endif