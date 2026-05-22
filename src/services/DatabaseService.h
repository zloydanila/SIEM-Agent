#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantList>
#include <QDateTime>
#include <QSqlDatabase>

class User;
class Rule;
class Event;
class Alert;
struct EventRecord;

class DatabaseService : public QObject {
    Q_OBJECT
public:
    explicit DatabaseService(const QString &connectionName, QObject *parent = nullptr);
    ~DatabaseService();

    QString lastError() const;

    bool open();
    bool openWithPath(const QString &path);
    void close();

    QString dbPath() const { return m_dbPath; }
    QString connectionName() const { return m_connectionName; }

    static QString defaultDbPath();
    QString generateId() const;
    QString generateSalt() const;
    QString generateRandomPassword(int length = 12) const;
    bool updateAlertStatus(const QString &alertId, const QString &newStatus);

    bool initSchema();

    void seedDefaultRules();
    bool createDefaultAdmin();

    bool userExists(const QString &username) const;
    User findUserByUsername(const QString &username) const;
    User findUserById(const QString &userId) const;
    QVector<User> getAllUsers() const;
    bool createUser(const User &user);
    bool updateUser(const User &user);
    bool deleteUser(const QString &userId);
    bool setUserActive(const QString &userId, bool active);
    bool setMustChangePassword(const QString &userId, bool value);

    bool createRule(const Rule &rule);
    bool updateRule(const Rule &rule);
    bool deleteRule(const QString &ruleId);
    bool setRuleEnabled(const QString &ruleId, bool enabled);
    QVector<Rule> getAllRules() const;
    bool ruleExists(const QString &ruleId) const;

    bool createEvent(const Event &event);
    QVector<Event> getRecentEvents(int limit, int offset = 0) const;
    QVector<Event> getEventsPaged(int limit, const QString &lastId, const QDateTime &lastTimestamp) const;
    QVector<Event> getEventsByDateRange(const QDateTime &from, const QDateTime &to) const;
    int getTotalEventsCount() const;
    int getEventCount() const;
    int getEventCountBySeverity(const QString &severity) const;
    bool clearEvents();

    bool createAlert(const Alert &alert);
    bool updateAlert(const Alert &alert);
    QVector<Alert> getActiveAlerts() const;
    QVector<Alert> getAllAlerts() const;
    int getAlertCount() const;
    int getAlertCountBySeverity(const QString &severity) const;
    int getAlertCountByStatus(const QString &status) const;
    bool clearAlerts();

    QVariantList getTopDevices(int limit) const;
    QVariantList getActivityLast7Hours() const;

    bool saveEventForCorrelation(const QString &deviceName, const QString &eventType, const QDateTime &timestamp);
    QVector<EventRecord> loadRecentHistory(int maxEvents) const;
    bool clearCorrelationHistory();
    bool pruneCorrelationHistory(int olderThanSeconds);

private:
    QSqlDatabase database() const;

    QString m_connectionName;
    QString m_dbPath;
    QString m_lastError;
    bool m_opened = false;
};