#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

class Event{
public:

    Event();

    QString id;
    QString deviceName;
    QString eventType;
    QString action;
    QString severity;
    QDateTime timestamp;
    QString rawLog;
    QString location;

    QJsonObject toJson() const;
    static Event fromJson(const QJsonObject &json);

};

Q_DECLARE_METATYPE(Event)

#endif