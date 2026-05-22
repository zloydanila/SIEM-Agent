#include "CorrelationEngine.h"
#include "../services/DatabaseService.h"
#include <QDebug>
#include <QUuid>

CorrelationEngine::CorrelationEngine(DatabaseService *db, QObject *parent)
    : QObject(parent), m_db(db) {
    if (m_db) {
        m_db->seedDefaultRules();
        reloadRules();
        auto persistentHistory = m_db->loadRecentHistory(2000);
        for (const auto& rec : persistentHistory) {
            m_history[rec.deviceName].append(rec);
        }
        qDebug() << "[CorrelationEngine] Restored" << persistentHistory.size() << "events from DB history";
    }
}

void CorrelationEngine::reloadRules() {
    if (!m_db) {
        m_rules.clear();
        return;
    }
    m_rules = m_db->getAllRules().toList();
    qDebug() << "[CorrelationEngine] Loaded" << m_rules.size() << "rules from DB";
}

void CorrelationEngine::addRule(const Rule &rule) {
    m_rules.append(rule);
}

void CorrelationEngine::analyze(const Event &event) {
    if (!m_db) return;

    EventRecord record;
    record.deviceName = event.deviceName;
    record.eventType = event.eventType;
    record.timestamp = event.timestamp.isValid()
                    ? event.timestamp.toUTC()
                    : QDateTime::currentDateTimeUtc();

    m_history[event.deviceName].append(record);
    m_db->saveEventForCorrelation(event.deviceName, event.eventType, record.timestamp);

    if (m_history[event.deviceName].size() > 2000) {
        m_history[event.deviceName].removeFirst();
    }

    for (const Rule &rule : m_rules) {
        if (!rule.isEnabled) continue;
        if (rule.ruleType == "threshold") {
            analyzeThreshold(rule, event, record);
        } else if (rule.ruleType == "correlation") {
            analyzeCorrelation(rule, event);
        }
    }
}

void CorrelationEngine::analyzeThreshold(const Rule &rule, const Event &event, const EventRecord &record) {
    Q_UNUSED(record);
    if (event.eventType != rule.matchEventType) return;

    pruneHistory(event.deviceName, rule.windowSeconds);
    int count = countMatches(event.deviceName, rule.matchEventType, rule.windowSeconds);

    if (count >= rule.threshold) {
        if (!isOnCooldown(rule.id, event.deviceName)) {
            Alert alert = buildAlert(rule, event.deviceName, event);
            if (m_db->createAlert(alert)) {
                qDebug() << "[CorrelationEngine] Alert fired:" << rule.name << "on" << event.deviceName;
                setCooldown(rule.id, event.deviceName, rule.cooldownSeconds);
                emit alertCreated();
            }
        }
    }
}

void CorrelationEngine::analyzeCorrelation(const Rule &rule, const Event &event) {
    if (event.eventType != rule.matchEventType) return;

    pruneHistory(event.deviceName, rule.windowSeconds);
    int contextCount = countMatches(event.deviceName, rule.secondaryEventType, rule.windowSeconds);

    if (contextCount >= rule.threshold) {
        if (!isOnCooldown(rule.id, event.deviceName)) {
            Alert alert = buildAlert(rule, event.deviceName, event);
            if (m_db->createAlert(alert)) {
                qDebug() << "[CorrelationEngine] Correlation alert fired:" << rule.name << "on" << event.deviceName;
                setCooldown(rule.id, event.deviceName, rule.cooldownSeconds);
                emit alertCreated();
            }
        }
    }
}

void CorrelationEngine::pruneHistory(const QString &deviceName, int windowSeconds) {
    if (!m_history.contains(deviceName)) return;

    QDateTime cutoff = QDateTime::currentDateTimeUtc().addSecs(-windowSeconds);
    QList<EventRecord> &records = m_history[deviceName];

    while (!records.isEmpty() && records.first().timestamp < cutoff) {
        records.removeFirst();
    }
}

int CorrelationEngine::countMatches(const QString &deviceName, const QString &eventType, int windowSeconds) {
    if (!m_history.contains(deviceName)) return 0;

    QDateTime cutoff = QDateTime::currentDateTimeUtc().addSecs(-windowSeconds);
    int count = 0;

    for (const EventRecord &r : m_history[deviceName]) {
        if (r.eventType == eventType && r.timestamp >= cutoff) {
            count++;
        }
    }
    return count;
}

bool CorrelationEngine::isOnCooldown(const QString &ruleId, const QString &deviceName) {
    QString key = ruleId + ":" + deviceName;
    if (!m_cooldowns.contains(key)) return false;
    return m_cooldowns[key] > QDateTime::currentDateTimeUtc();
}

void CorrelationEngine::setCooldown(const QString &ruleId, const QString &deviceName, int cooldownSeconds) {
    m_cooldowns[ruleId + ":" + deviceName] = QDateTime::currentDateTimeUtc().addSecs(cooldownSeconds);
}

Alert CorrelationEngine::buildAlert(const Rule &rule, const QString &deviceName, const Event &triggerEvent) {
    Alert alert;
    alert.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    alert.title = rule.alertTitle;
    alert.description = rule.alertDescription
                        + "\nУстройство: " + deviceName
                        + "\nСобытие-триггер: " + triggerEvent.eventType;
    alert.severity = rule.alertSeverity;
    alert.status = "open";
    alert.deviceName = deviceName;
    alert.triggeredAt = QDateTime::currentDateTimeUtc();
    alert.ruleId = rule.id;
    alert.assignedTo = "";
    alert.comment = "Авто-создан движком корреляции";
    return alert;
}

void CorrelationEngine::globalCleanup() {
    if (!m_db) return;

    for (auto it = m_history.begin(); it != m_history.end(); ) {
        pruneHistory(it.key(), 3600);
        if (it.value().isEmpty())
            it = m_history.erase(it);
        else
            ++it;
    }

    for (auto it = m_cooldowns.begin(); it != m_cooldowns.end(); ) {
        if (it.value() < QDateTime::currentDateTimeUtc())
            it = m_cooldowns.erase(it);
        else
            ++it;
    }

    m_db->pruneCorrelationHistory(24 * 3600);
    qDebug() << "[CorrelationEngine] Global cleanup completed";
}