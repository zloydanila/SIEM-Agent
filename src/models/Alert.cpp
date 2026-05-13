#include "Alert.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

Alert::Alert() {
    id = "";
    title = "";
    description = "";
    severity = "";
    status = "";
    deviceName = "";
    ruleId = "";
    assignedTo = "";
    comment = "";
}

QJsonObject Alert::toJson() const {
    QJsonObject obj;

    obj["id"] = id;
    obj["title"] = title;
    obj["description"] = description;
    obj["severity"] = severity;
    obj["status"] = status;
    obj["deviceName"] = deviceName;
    obj["ruleId"] = ruleId;
    obj["assignedTo"] = assignedTo;
    obj["comment"] = comment;
    
    QJsonArray eventArray;
    for (const QString &eventId : relatedEventIds) {
        eventArray.append(eventId);
    }
    obj["relatedEventIds"] = eventArray;

    if (triggeredAt.isValid()) {
        obj["triggeredAt"] = triggeredAt.toString("yyyy-MM-dd hh:mm:ss");
    } else {
        obj["triggeredAt"] = "";
    }

    return obj;
}

Alert Alert::fromJson(const QJsonObject &json) {
    Alert alert;

    alert.id = json["id"].toString();
    alert.title = json["title"].toString();
    alert.description = json["description"].toString();
    alert.severity = json["severity"].toString();
    alert.status = json["status"].toString();
    alert.deviceName = json["deviceName"].toString();
    alert.ruleId = json["ruleId"].toString();
    alert.assignedTo = json["assignedTo"].toString();
    alert.comment = json["comment"].toString();

    QJsonArray eventArray = json["relatedEventIds"].toArray();
    for (const QJsonValue &val : eventArray) {
        alert.relatedEventIds.append(val.toString());
    }

    QString triggeredAtStr = json["triggeredAt"].toString();
    if (!triggeredAtStr.isEmpty()) {
        alert.triggeredAt = QDateTime::fromString(triggeredAtStr, "yyyy-MM-dd hh:mm:ss");
    }

    return alert;
}