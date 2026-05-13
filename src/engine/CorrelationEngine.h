#ifndef CORRELATIONENGINE_H
#define CORRELATIONENGINE_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QDateTime>
#include "../models/Event.h"
#include "../models/Alert.h"
#include "Rule.h"
#include "../engine/EventRecord.h"

class DatabaseService;

class CorrelationEngine : public QObject {
    Q_OBJECT

public:
    explicit CorrelationEngine(DatabaseService *db, QObject *parent = nullptr);

    void analyze(const Event &event);
    void reloadRules();
    void addRule(const Rule &rule);
    void globalCleanup();

signals:
    void alertCreated();

private:

    DatabaseService *m_db;
    QList<Rule> m_rules;
    
    QHash<QString, QList<EventRecord>> m_history;
    QHash<QString, QDateTime> m_cooldowns;

    void analyzeThreshold(const Rule &rule, const Event &event, const EventRecord &record   );
    void analyzeCorrelation(const Rule &rule, const Event &event);
    void pruneHistory(const QString &deviceName, int windowSeconds);
    int  countMatches(const QString &deviceName, const QString &eventType, int windowSeconds);

    bool isOnCooldown(const QString &ruleId, const QString &deviceName);
    void setCooldown(const QString &ruleId, const QString &deviceName, int cooldownSeconds);

    Alert buildAlert(const Rule &rule, const QString &deviceName, const Event &triggerEvent);
};

#endif