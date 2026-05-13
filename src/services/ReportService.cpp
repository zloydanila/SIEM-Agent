#include "ReportService.h"
#include "DatabaseService.h"
#include "../models/Event.h"
#include "../models/Alert.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>
#include <QDebug>

ReportService::ReportService(DatabaseService *db, QObject *parent)
    : QObject(parent), m_db(db)
{}

QString ReportService::defaultExportPath(const QString &name) const {
    QString dir = QDir::homePath() + "/siem_reports";
    QDir().mkpath(dir);
    return dir + "/" + name + "_"
         + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
}

bool ReportService::exportEventsCsv(const QString &filePath) {
    QFile file(filePath.endsWith(".csv") ? filePath : filePath + ".csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFailed("Не удалось открыть файл: " + file.fileName());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "ID,Устройство,Тип события,Действие,Серьёзность,Время,Локация,Лог\n";

    QVector<Event> events = m_db->getRecentEvents(10000);
    auto escape = [](const QString &s) {
        return "\"" + QString(s).replace("\"", "\"\"") + "\"";
    };
    for (const Event &e : events) {
        out << escape(e.id)          << ","
            << escape(e.deviceName)  << ","
            << escape(e.eventType)   << ","
            << escape(e.action)      << ","
            << escape(e.severity)    << ","
            << escape(e.timestamp.toString(Qt::ISODate)) << ","
            << escape(e.location)    << ","
            << escape(e.rawLog)      << "\n";
    }

    file.close();
    qInfo() << "[Report] События экспортированы:" << file.fileName()
            << "(" << events.size() << "записей)";
    emit exportSuccess(file.fileName());
    return true;
}

bool ReportService::exportAlertsCsv(const QString &filePath) {
    QFile file(filePath.endsWith(".csv") ? filePath : filePath + ".csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFailed("Не удалось открыть файл: " + file.fileName());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "ID,Заголовок,Описание,Серьёзность,Статус,Устройство,Правило,Время,Назначено,Комментарий\n";

    QVector<Alert> alerts = m_db->getAllAlerts();
    auto escape = [](const QString &s) {
        return "\"" + QString(s).replace("\"", "\"\"") + "\"";
    };
    for (const Alert &a : alerts) {
        out << escape(a.id)          << ","
            << escape(a.title)       << ","
            << escape(a.description) << ","
            << escape(a.severity)    << ","
            << escape(a.status)      << ","
            << escape(a.deviceName)  << ","
            << escape(a.ruleId)      << ","
            << escape(a.triggeredAt.toString(Qt::ISODate)) << ","
            << escape(a.assignedTo)  << ","
            << escape(a.comment)     << "\n";
    }

    file.close();
    qInfo() << "[Report] Алерты экспортированы:" << file.fileName()
            << "(" << alerts.size() << "записей)";
    emit exportSuccess(file.fileName());
    return true;
}

bool ReportService::exportReportJson(const QString &filePath) {
    QVector<Event> events = m_db->getRecentEvents(10000);
    QVector<Alert> alerts = m_db->getAllAlerts();

    QJsonObject stats;
    stats["total_events"] = events.size();
    stats["total_alerts"] = alerts.size();
    stats["generated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QMap<QString, int> eventsBySev;
    for (const Event &e : events) eventsBySev[e.severity]++;
    QJsonObject evSevObj;
    for (auto it = eventsBySev.begin(); it != eventsBySev.end(); ++it)
        evSevObj[it.key()] = it.value();
    stats["events_by_severity"] = evSevObj;

    QMap<QString, int> alertsBySev;
    QMap<QString, int> alertsByStatus;
    for (const Alert &a : alerts) {
        alertsBySev[a.severity]++;
        alertsByStatus[a.status]++;
    }
    QJsonObject alertSevObj;
    for (auto it = alertsBySev.begin(); it != alertsBySev.end(); ++it)
        alertSevObj[it.key()] = it.value();
    stats["alerts_by_severity"] = alertSevObj;

    QJsonObject alertStatusObj;
    for (auto it = alertsByStatus.begin(); it != alertsByStatus.end(); ++it)
        alertStatusObj[it.key()] = it.value();
    stats["alerts_by_status"] = alertStatusObj;

    QJsonArray eventsArr;
    for (int i = 0; i < qMin(100, (int)events.size()); ++i) {
        const Event &e = events[i];
        QJsonObject obj;
        obj["id"]         = e.id;
        obj["deviceName"] = e.deviceName;
        obj["eventType"]  = e.eventType;
        obj["severity"]   = e.severity;
        obj["timestamp"]  = e.timestamp.toString(Qt::ISODate);
        obj["location"]   = e.location;
        eventsArr.append(obj);
    }

    QJsonArray alertsArr;
    for (const Alert &a : alerts) {
        QJsonObject obj;
        obj["id"]          = a.id;
        obj["title"]       = a.title;
        obj["severity"]    = a.severity;
        obj["status"]      = a.status;
        obj["deviceName"]  = a.deviceName;
        obj["ruleId"]      = a.ruleId;
        obj["triggeredAt"] = a.triggeredAt.toString(Qt::ISODate);
        obj["assignedTo"]  = a.assignedTo;
        obj["comment"]     = a.comment;
        alertsArr.append(obj);
    }

    QJsonObject root;
    root["stats"]  = stats;
    root["events"] = eventsArr;
    root["alerts"] = alertsArr;

    QString path = filePath.endsWith(".json") ? filePath : filePath + ".json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit exportFailed("Не удалось открыть файл: " + path);
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qInfo() << "[Report] JSON-отчёт экспортирован:" << path;
    emit exportSuccess(path);
    return true;
}