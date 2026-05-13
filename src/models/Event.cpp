#include "Event.h"
#include <QJsonObject>

Event::Event() {
    id = "";
    deviceName = "";
    eventType = "";
    action = "";
    severity = "low";
    rawLog = "";
    location = "";
}

QJsonObject Event::toJson() const {
    QJsonObject obj;
    
    obj["id"] = id;
    obj["deviceName"] = deviceName;
    obj["eventType"] = eventType;
    obj["action"] = action;
    obj["severity"] = severity;
    obj["rawLog"] = rawLog;
    obj["location"] = location;
    
    if (timestamp.isValid()) {
        obj["timestamp"] = timestamp.toString("yyyy-MM-dd hh:mm:ss");
    } else {
        obj["timestamp"] = "";
    }
    
    return obj;
}

Event Event::fromJson(const QJsonObject &json) {
    Event event;
    
    event.id = json["id"].toString();
    event.deviceName = json["deviceName"].toString();
    event.eventType = json["eventType"].toString();
    event.action = json["action"].toString();
    event.severity = json["severity"].toString();
    event.rawLog = json["rawLog"].toString();
    event.location = json["location"].toString();
    
    QString timestampStr = json["timestamp"].toString();
    if (!timestampStr.isEmpty()) {
        if (!timestampStr.endsWith("Z")) {
            timestampStr += "Z";
        }
        event.timestamp = QDateTime::fromString(timestampStr, Qt::ISODate);
        if (!event.timestamp.isValid()) {
            event.timestamp = QDateTime::fromString(timestampStr, "yyyy-MM-dd hh:mm:ss");
        }
    }
    
    return event;
}