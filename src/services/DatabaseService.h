#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include "../models/User.h"
#include "../models/Alert.h"
#include "../models/Event.h"
#include "../engine/Rule.h"
#include "../engine/EventRecord.h"

class QSqlDatabase;
class CorrelationEngine;

class DatabaseService : public QObject{
    Q_OBJECT
public:
    explicit DatabaseService(const QString &connectionName = "siem_connection", QObject *parent = nullptr);
    ~DatabaseService();

    bool open();
    void close();
    bool initSchema();

    bool createUser(const User &user);
    bool updateUser(const User &user);
    bool deleteUser(const QString &userId);
    bool setUserActive(const QString &userId, bool active);

    User findUserByUsername(const QString &username) const;
    User findUserById(const QString &userId) const;
    QVector<User> getAllUsers() const;

    bool userExists(const QString &username) const;
    bool createDefaultAdmin();

    bool createEvent(const Event &event);
    QVector<Event> getRecentEvents(int limit, int offset = 0) const;
    QVector<Event> getEventsPaged(int limit, const QString &lastId = {}, const QDateTime &lastTimestamp = {}) const;
    QVector<Event> getEventsByDateRange(const QDateTime &from, const QDateTime &to) const;
    int getTotalEventsCount() const;

    bool createAlert(const Alert &alert);
    bool updateAlert(const Alert &alert);
    QVector<Alert> getActiveAlerts() const;
    QVector<Alert> getAllAlerts() const;
    int getAlertCountBySeverity(const QString &severity) const;
    int getAlertCountByStatus(const QString &status) const;

    int getEventCountBySeverity(const QString &severity) const;
    QVariantList getTopDevices(int limit = 5) const;
    QVariantList getActivityLast7Hours() const;

    int getAlertCount() const;
    int getEventCount() const;

    bool setMustChangePassword(const QString &userId, bool value);
    QString generateSalt() const;

    bool createRule(const Rule &rule);
    bool updateRule(const Rule &rule);
    bool deleteRule(const QString &ruleId);
    bool setRuleEnabled(const QString &ruleId, bool enabled);
    QVector<Rule> getAllRules() const;
    bool ruleExists(const QString &rileId) const;
    void seedDefaultRules();
    bool clearEvents();
    bool clearAlerts();
    bool saveEventForCorrelation(const QString &deviceName, const QString &eventType, const QDateTime &timestamp);
    QVector<EventRecord> loadRecentHistory(int maxEvents = 5000) const;
    bool clearCorrelationHistory();
    bool pruneCorrelationHistory(int olderThanSeconds);

    QString dbPath() const { return m_dbPath; }
    bool openWithPath(const QString &path); 

    QString lastError() const;

private:
    QString m_dbPath;
    QString m_lastError;
    QString m_connectionName;
    bool m_opened;

    QString generateId() const;
    QSqlDatabase database() const;
    bool executeQuery(const QString &queryText);
    QString generateRandomPassword(int length) const;
};

#endif