#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <QVariantList>
#include <QSqlDatabase>

class User;
class Rule;
class Event;
class Alert;
class EventRecord;

class DatabaseService : public QObject {
    Q_OBJECT
public:
    explicit DatabaseService(const QString &connectionName = QString(), QObject *parent = nullptr);
    ~DatabaseService() override;

    QString lastError() const;
    QString generateId() const;
    QString generateSalt() const;
    QSqlDatabase database() const;

    bool open();
    static QString defaultDbPath();
    bool openWithPath(const QString &path);
    void close();
    bool initSchema();
    void seedDefaultRules();

    QString generateRandomPassword(int length) const;
    bool createDefaultAdmin();
    QString dbPath() const;
    
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

    bool clearEvents();
    bool clearAlerts();
    bool createEvent(const Event &event);
    QVector<Event> getRecentEvents(int limit = 100, int offset = 0) const;
    QVector<Event> getEventsPaged(int limit, const QString &lastId = QString(), const QDateTime &lastTimestamp = QDateTime()) const;
    QVector<Event> getEventsByDateRange(const QDateTime &from, const QDateTime &to) const;
    int getTotalEventsCount() const;

    bool createAlert(const Alert &alert);
    bool updateAlert(const Alert &alert);
    QVector<Alert> getActiveAlerts() const;
    QVector<Alert> getAllAlerts() const;
    int getAlertCountBySeverity(const QString &severity) const;
    int getAlertCountByStatus(const QString &status) const;
    int getAlertCount() const;

    int getEventCountBySeverity(const QString &severity) const;
    QVariantList getTopDevices(int limit) const;
    QVariantList getActivityLast7Hours() const;
    int getEventCount() const;

    bool saveEventForCorrelation(const QString &deviceName, const QString &eventType, const QDateTime &timestamp);
    QVector<EventRecord> loadRecentHistory(int maxEvents = 100) const;
    bool clearCorrelationHistory();
    bool pruneCorrelationHistory(int olderThanSeconds);

    bool updateAlertStatus(const QString &alertId, const QString &newStatus);

private:
    QString m_connectionName;
    bool m_opened = false;
    QString m_dbPath;
    QString m_lastError;
};

#endif