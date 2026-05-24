#ifndef ALERT_H
#define ALERT_H

#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QJsonObject>

class Alert{
public:

    Alert();

    QString id;
    QString title;
    QString description;
    QString severity;
    QString status;
    QString deviceName;
    QDateTime triggeredAt;
    QString ruleId;
    QString assignedTo;
    QString comment;
    QStringList relatedEventIds;

    QJsonObject toJson() const;
    static Alert fromJson(const QJsonObject &json);

};

Q_DECLARE_METATYPE(Alert)

#endif