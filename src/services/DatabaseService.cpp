#include "DatabaseService.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QUuid>
#include <QDebug>

#include "../models/User.h"
#include "../engine/Rule.h"
#include "../models/Event.h"
#include "../models/Alert.h"
#include "../engine/EventRecord.h"\


static constexpr int PBKDF2_ITERATIONS = 100000;
static constexpr int PBKDF2_DKLEN = 32;

DatabaseService::DatabaseService(const QString &connectionName, QObject *parent)
    : QObject(parent), m_connectionName(connectionName), m_opened(false) {}

DatabaseService::~DatabaseService() {
    close();
}

QString DatabaseService::lastError() const { return m_lastError; }

QString DatabaseService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString DatabaseService::generateSalt() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces) +
           QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QSqlDatabase DatabaseService::database() const {
    return QSqlDatabase::database(m_connectionName);
}

bool DatabaseService::open() {
    return openWithPath(m_dbPath.isEmpty() ? defaultDbPath() : m_dbPath);
}

QString DatabaseService::defaultDbPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.local/share/SIEMAgent";
    QDir().mkpath(base);
    return base + "/siemagent.db";
}

bool DatabaseService::openWithPath(const QString &path) {
    if (m_opened) close();

    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase existing = QSqlDatabase::database(m_connectionName, false);
        if (existing.isValid()) existing.close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);

    if (path.isEmpty()) {
        QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (base.isEmpty()) base = QDir::homePath() + "/.local/share/SIEMAgent";
        QDir().mkpath(base);
        m_dbPath = base + "/siemagent.db";
    } else {
        m_dbPath = path;
    }

    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }

    m_opened = true;
    return initSchema();
}

void DatabaseService::close() {
    if (!m_opened) return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid()) db.close();
    }
    m_opened = false;
}

bool DatabaseService::initSchema() {
    if (!m_opened) return false;
    QSqlQuery q(database());

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT NOT NULL,
            full_name TEXT NOT NULL DEFAULT '',
            email TEXT NOT NULL DEFAULT '',
            is_active INTEGER NOT NULL DEFAULT 1,
            must_change_password INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL
        )
    )")) { m_lastError = q.lastError().text(); return false; }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS rules (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            rule_type TEXT NOT NULL,
            match_event_type TEXT NOT NULL,
            secondary_event_type TEXT NOT NULL DEFAULT '',
            threshold INTEGER NOT NULL DEFAULT 1,
            window_seconds INTEGER NOT NULL DEFAULT 60,
            cooldown_seconds INTEGER NOT NULL DEFAULT 60,
            alert_severity TEXT NOT NULL DEFAULT 'high',
            alert_title TEXT NOT NULL,
            alert_description TEXT NOT NULL DEFAULT '',
            is_enabled INTEGER NOT NULL DEFAULT 1
        )
    )")) { m_lastError = q.lastError().text(); return false; }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS events (
            id TEXT PRIMARY KEY,
            device_name TEXT NOT NULL DEFAULT '',
            event_type TEXT NOT NULL,
            action TEXT NOT NULL DEFAULT '',
            severity TEXT NOT NULL DEFAULT 'low',
            timestamp TEXT NOT NULL,
            raw_log TEXT NOT NULL DEFAULT '',
            location TEXT NOT NULL DEFAULT ''
        )
    )")) { m_lastError = q.lastError().text(); return false; }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS alerts (
            id TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            description TEXT NOT NULL DEFAULT '',
            severity TEXT NOT NULL DEFAULT 'medium',
            status TEXT NOT NULL DEFAULT 'open',
            device_name TEXT NOT NULL DEFAULT '',
            triggered_at TEXT NOT NULL,
            rule_id TEXT NOT NULL DEFAULT '',
            assigned_to TEXT NOT NULL DEFAULT '',
            comment TEXT NOT NULL DEFAULT '',
            related_event_ids TEXT NOT NULL DEFAULT '[]'
        )
    )")) { m_lastError = q.lastError().text(); return false; }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS correlation_history (
            id TEXT PRIMARY KEY,
            device_name TEXT NOT NULL,
            event_type TEXT NOT NULL,
            created_at TEXT NOT NULL
        )
    )")) { m_lastError = q.lastError().text(); return false; }

    if (!userExists("admin")) createDefaultAdmin();
    seedDefaultRules();
    return true;
}

static bool ruleSeedExists(QSqlDatabase db, const QString &ruleType, const QString &matchEventType,
                           const QString &secondaryEventType, const QString &name) {
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT 1 FROM rules
        WHERE rule_type = :rule_type
          AND match_event_type = :match_event_type
          AND secondary_event_type = :secondary_event_type
          AND name = :name
        LIMIT 1
    )");
    q.bindValue(":rule_type", ruleType);
    q.bindValue(":match_event_type", matchEventType);
    q.bindValue(":secondary_event_type", secondaryEventType);
    q.bindValue(":name", name);
    return q.exec() && q.next();
}

void DatabaseService::seedDefaultRules() {
    if (!m_opened) return;

    struct SeedRule {
        QString name;
        QString ruleType;
        QString matchEventType;
        QString secondaryEventType;
        int threshold;
        int windowSeconds;
        int cooldownSeconds;
        QString alertSeverity;
        QString alertTitle;
        QString alertDescription;
    };

    const QVector<SeedRule> seeds = {
        {
            "Critical events threshold",
            "threshold",
            "critical",
            "",
            1,
            60,
            60,
            "high",
            "Critical event detected",
            "A critical event was generated"
        },
        {
            "Multiple login failures",
            "threshold",
            "auth_failed",
            "",
            5,
            300,
            300,
            "high",
            "Repeated authentication failures",
            "Several failed logins were detected within a short time"
        },
        {
            "Suspicious access correlation",
            "correlation",
            "file_access",
            "privilege_escalation",
            1,
            600,
            600,
            "high",
            "Suspicious access pattern",
            "File access followed by privilege escalation"
        }
    };

    for (const auto &s : seeds) {
        if (ruleSeedExists(database(), s.ruleType, s.matchEventType, s.secondaryEventType, s.name))
            continue;

        Rule rule;
        rule.id = generateId();
        rule.name = s.name;
        rule.ruleType = s.ruleType;
        rule.matchEventType = s.matchEventType;
        rule.secondaryEventType = s.secondaryEventType;
        rule.threshold = s.threshold;
        rule.windowSeconds = s.windowSeconds;
        rule.cooldownSeconds = s.cooldownSeconds;
        rule.alertSeverity = s.alertSeverity;
        rule.alertTitle = s.alertTitle;
        rule.alertDescription = s.alertDescription;
        rule.isEnabled = true;

        createRule(rule);
    }
}

QString DatabaseService::generateRandomPassword(int length) const {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString out;
    out.reserve(length);
    for (int i = 0; i < length; ++i) {
        out.append(chars[QRandomGenerator::global()->bounded(int(sizeof(chars) - 1))]);
    }
    return out;
}

bool DatabaseService::createDefaultAdmin() {
    if (userExists("admin")) return true;

    User admin;
    admin.id = generateId();
    admin.username = "admin";
    admin.role = "admin";
    admin.fullName = "Administrator";
    admin.email = "admin@localhost";
    admin.isActive = true;
    admin.mustChangePassword = true;
    admin.createdAt = QDateTime::currentDateTime();

    const QString tempPassword = generateRandomPassword(12);
    admin.setPassword(tempPassword, generateSalt());

    qInfo() << "[DB] default admin password:" << tempPassword;

    return createUser(admin);
}

bool DatabaseService::userExists(const QString &username) const {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("SELECT 1 FROM users WHERE username = :u LIMIT 1");
    q.bindValue(":u", username);
    if (!q.exec()) return false;
    return q.next();
}

User DatabaseService::findUserByUsername(const QString &username) const {
    User user;
    if (!m_opened) return user;

    QSqlQuery q(database());
    q.prepare(R"(
        SELECT id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        FROM users WHERE username = :u LIMIT 1
    )");
    q.bindValue(":u", username);
    if (!q.exec() || !q.next()) return user;

    user.id = q.value("id").toString();
    user.username = q.value("username").toString();
    user.passwordHash = q.value("password_hash").toString();
    user.salt = q.value("salt").toString();
    user.role = q.value("role").toString();
    user.fullName = q.value("full_name").toString();
    user.email = q.value("email").toString();
    user.isActive = q.value("is_active").toInt() == 1;
    user.mustChangePassword = q.value("must_change_password").toInt() == 1;
    user.createdAt = QDateTime::fromString(q.value("created_at").toString(), Qt::ISODate);
    return user;
}

User DatabaseService::findUserById(const QString &userId) const {
    User user;
    if (!m_opened) return user;

    QSqlQuery q(database());
    q.prepare(R"(
        SELECT id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        FROM users WHERE id = :id LIMIT 1
    )");
    q.bindValue(":id", userId);
    if (!q.exec() || !q.next()) return user;

    user.id = q.value("id").toString();
    user.username = q.value("username").toString();
    user.passwordHash = q.value("password_hash").toString();
    user.salt = q.value("salt").toString();
    user.role = q.value("role").toString();
    user.fullName = q.value("full_name").toString();
    user.email = q.value("email").toString();
    user.isActive = q.value("is_active").toInt() == 1;
    user.mustChangePassword = q.value("must_change_password").toInt() == 1;
    user.createdAt = QDateTime::fromString(q.value("created_at").toString(), Qt::ISODate);
    return user;
}

QVector<User> DatabaseService::getAllUsers() const {
    QVector<User> users;
    if (!m_opened) return users;

    QSqlQuery q(database());
    if (!q.exec(R"(
        SELECT id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        FROM users ORDER BY username
    )")) return users;

    while (q.next()) {
        User user;
        user.id = q.value("id").toString();
        user.username = q.value("username").toString();
        user.passwordHash = q.value("password_hash").toString();
        user.salt = q.value("salt").toString();
        user.role = q.value("role").toString();
        user.fullName = q.value("full_name").toString();
        user.email = q.value("email").toString();
        user.isActive = q.value("is_active").toInt() == 1;
        user.mustChangePassword = q.value("must_change_password").toInt() == 1;
        user.createdAt = QDateTime::fromString(q.value("created_at").toString(), Qt::ISODate);
        users.append(user);
    }
    return users;
}

bool DatabaseService::createUser(const User &user) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        INSERT INTO users (id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at)
        VALUES (:id, :username, :password_hash, :salt, :role, :full_name, :email, :is_active, :must_change_password, :created_at)
    )");
    q.bindValue(":id", user.id.isEmpty() ? generateId() : user.id);
    q.bindValue(":username", user.username);
    q.bindValue(":password_hash", user.passwordHash);
    q.bindValue(":salt", user.salt);
    q.bindValue(":role", user.role);
    q.bindValue(":full_name", user.fullName);
    q.bindValue(":email", user.email);
    q.bindValue(":is_active", user.isActive ? 1 : 0);
    q.bindValue(":must_change_password", user.mustChangePassword ? 1 : 0);
    q.bindValue(":created_at", user.createdAt.isValid() ? user.createdAt.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::updateUser(const User &user) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        UPDATE users SET
            username = :username,
            password_hash = :password_hash,
            salt = :salt,
            role = :role,
            full_name = :full_name,
            email = :email,
            is_active = :is_active,
            must_change_password = :must_change_password,
            created_at = :created_at
        WHERE id = :id
    )");
    q.bindValue(":id", user.id);
    q.bindValue(":username", user.username);
    q.bindValue(":password_hash", user.passwordHash);
    q.bindValue(":salt", user.salt);
    q.bindValue(":role", user.role);
    q.bindValue(":full_name", user.fullName);
    q.bindValue(":email", user.email);
    q.bindValue(":is_active", user.isActive ? 1 : 0);
    q.bindValue(":must_change_password", user.mustChangePassword ? 1 : 0);
    q.bindValue(":created_at", user.createdAt.isValid() ? user.createdAt.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseService::deleteUser(const QString &userId) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("DELETE FROM users WHERE id = :id");
    q.bindValue(":id", userId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseService::setUserActive(const QString &userId, bool active) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("UPDATE users SET is_active = :a WHERE id = :id");
    q.bindValue(":id", userId);
    q.bindValue(":a", active ? 1 : 0);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseService::setMustChangePassword(const QString &userId, bool value) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("UPDATE users SET must_change_password = :v WHERE id = :id");
    q.bindValue(":id", userId);
    q.bindValue(":v", value ? 1 : 0);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseService::createRule(const Rule &rule) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        INSERT INTO rules (id, name, rule_type, match_event_type, secondary_event_type, threshold, window_seconds, cooldown_seconds, alert_severity, alert_title, alert_description, is_enabled)
        VALUES (:id, :name, :rule_type, :match_event_type, :secondary_event_type, :threshold, :window_seconds, :cooldown_seconds, :alert_severity, :alert_title, :alert_description, :is_enabled)
    )");
    q.bindValue(":id", rule.id.isEmpty() ? generateId() : rule.id);
    q.bindValue(":name", rule.name);
    q.bindValue(":rule_type", rule.ruleType);
    q.bindValue(":match_event_type", rule.matchEventType);
    q.bindValue(":secondary_event_type", rule.secondaryEventType);
    q.bindValue(":threshold", rule.threshold);
    q.bindValue(":window_seconds", rule.windowSeconds);
    q.bindValue(":cooldown_seconds", rule.cooldownSeconds);
    q.bindValue(":alert_severity", rule.alertSeverity);
    q.bindValue(":alert_title", rule.alertTitle);
    q.bindValue(":alert_description", rule.alertDescription);
    q.bindValue(":is_enabled", rule.isEnabled ? 1 : 0);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::updateRule(const Rule &rule) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        UPDATE rules SET
            name = :name,
            rule_type = :rule_type,
            match_event_type = :match_event_type,
            secondary_event_type = :secondary_event_type,
            threshold = :threshold,
            window_seconds = :window_seconds,
            cooldown_seconds = :cooldown_seconds,
            alert_severity = :alert_severity,
            alert_title = :alert_title,
            alert_description = :alert_description,
            is_enabled = :is_enabled
        WHERE id = :id
    )");
    q.bindValue(":id", rule.id);
    q.bindValue(":name", rule.name);
    q.bindValue(":rule_type", rule.ruleType);
    q.bindValue(":match_event_type", rule.matchEventType);
    q.bindValue(":secondary_event_type", rule.secondaryEventType);
    q.bindValue(":threshold", rule.threshold);
    q.bindValue(":window_seconds", rule.windowSeconds);
    q.bindValue(":cooldown_seconds", rule.cooldownSeconds);
    q.bindValue(":alert_severity", rule.alertSeverity);
    q.bindValue(":alert_title", rule.alertTitle);
    q.bindValue(":alert_description", rule.alertDescription);
    q.bindValue(":is_enabled", rule.isEnabled ? 1 : 0);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseService::deleteRule(const QString &ruleId) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("DELETE FROM rules WHERE id = :id");
    q.bindValue(":id", ruleId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseService::setRuleEnabled(const QString &ruleId, bool enabled) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("UPDATE rules SET is_enabled = :e WHERE id = :id");
    q.bindValue(":id", ruleId);
    q.bindValue(":e", enabled ? 1 : 0);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

QVector<Rule> DatabaseService::getAllRules() const {
    QVector<Rule> rules;
    if (!m_opened) return rules;

    QSqlQuery q(database());
    if (!q.exec(R"(
        SELECT id, name, rule_type, match_event_type, secondary_event_type, threshold, window_seconds, cooldown_seconds, alert_severity, alert_title, alert_description, is_enabled
        FROM rules ORDER BY name
    )")) return rules;

    while (q.next()) {
        Rule r;
        r.id = q.value("id").toString();
        r.name = q.value("name").toString();
        r.ruleType = q.value("rule_type").toString();
        r.matchEventType = q.value("match_event_type").toString();
        r.secondaryEventType = q.value("secondary_event_type").toString();
        r.threshold = q.value("threshold").toInt();
        r.windowSeconds = q.value("window_seconds").toInt();
        r.cooldownSeconds = q.value("cooldown_seconds").toInt();
        r.alertSeverity = q.value("alert_severity").toString();
        r.alertTitle = q.value("alert_title").toString();
        r.alertDescription = q.value("alert_description").toString();
        r.isEnabled = q.value("is_enabled").toInt() == 1;
        rules.append(r);
    }
    return rules;
}

bool DatabaseService::ruleExists(const QString &ruleId) const {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare("SELECT 1 FROM rules WHERE id = :id LIMIT 1");
    q.bindValue(":id", ruleId);
    if (!q.exec()) return false;
    return q.next();
}

bool DatabaseService::clearEvents() {
    if (!m_opened) return false;
    QSqlQuery q(database());
    if (!q.exec("DELETE FROM events")) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::clearAlerts() {
    if (!m_opened) return false;
    QSqlQuery q(database());
    if (!q.exec("DELETE FROM alerts")) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::createEvent(const Event &event) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        INSERT INTO events (id, device_name, event_type, action, severity, timestamp, raw_log, location)
        VALUES (:id, :device_name, :event_type, :action, :severity, :timestamp, :raw_log, :location)
    )");
    q.bindValue(":id", event.id.isEmpty() ? generateId() : event.id);
    q.bindValue(":device_name", event.deviceName);
    q.bindValue(":event_type", event.eventType);
    q.bindValue(":action", event.action);
    q.bindValue(":severity", event.severity);
    q.bindValue(":timestamp", event.timestamp.isValid() ? event.timestamp.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":raw_log", event.rawLog);
    q.bindValue(":location", event.location);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QVector<Event> DatabaseService::getRecentEvents(int limit, int offset) const {
    QVector<Event> events;
    if (!m_opened) return events;

    QSqlQuery q(database());
    q.prepare(R"(
        SELECT id, device_name, event_type, action, severity, timestamp, raw_log, location
        FROM events
        ORDER BY timestamp DESC
        LIMIT :limit OFFSET :offset
    )");
    q.bindValue(":limit", limit);
    q.bindValue(":offset", offset);
    if (!q.exec()) return events;

    while (q.next()) {
        Event e;
        e.id = q.value("id").toString();
        e.deviceName = q.value("device_name").toString();
        e.eventType = q.value("event_type").toString();
        e.action = q.value("action").toString();
        e.severity = q.value("severity").toString();
        e.timestamp = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
        e.rawLog = q.value("raw_log").toString();
        e.location = q.value("location").toString();
        events.append(e);
    }
    return events;
}

QVector<Event> DatabaseService::getEventsPaged(int limit, const QString &lastId, const QDateTime &lastTimestamp) const {
    QVector<Event> events;
    if (!m_opened) return events;

    QSqlQuery q(database());
    if (lastId.isEmpty() || !lastTimestamp.isValid()) {
        q.prepare(R"(
            SELECT id, device_name, event_type, action, severity, timestamp, raw_log, location
            FROM events
            ORDER BY timestamp DESC, id DESC
            LIMIT :limit
        )");
        q.bindValue(":limit", limit);
    } else {
        q.prepare(R"(
            SELECT id, device_name, event_type, action, severity, timestamp, raw_log, location
            FROM events
            WHERE timestamp < :lastTimestamp
               OR (timestamp = :lastTimestamp AND id < :lastId)
            ORDER BY timestamp DESC, id DESC
            LIMIT :limit
        )");
        q.bindValue(":lastTimestamp", lastTimestamp.toString(Qt::ISODate));
        q.bindValue(":lastId", lastId);
        q.bindValue(":limit", limit);
    }

    if (!q.exec()) return events;
    while (q.next()) {
        Event e;
        e.id = q.value("id").toString();
        e.deviceName = q.value("device_name").toString();
        e.eventType = q.value("event_type").toString();
        e.action = q.value("action").toString();
        e.severity = q.value("severity").toString();
        e.timestamp = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
        e.rawLog = q.value("raw_log").toString();
        e.location = q.value("location").toString();
        events.append(e);
    }
    return events;
}

QVector<Event> DatabaseService::getEventsByDateRange(const QDateTime &from, const QDateTime &to) const {
    QVector<Event> events;
    if (!m_opened) return events;

    QSqlQuery q(database());
    q.prepare(R"(
        SELECT id, device_name, event_type, action, severity, timestamp, raw_log, location
        FROM events
        WHERE timestamp >= :from AND timestamp <= :to
        ORDER BY timestamp DESC
    )");
    q.bindValue(":from", from.toString(Qt::ISODate));
    q.bindValue(":to", to.toString(Qt::ISODate));
    if (!q.exec()) return events;

    while (q.next()) {
        Event e;
        e.id = q.value("id").toString();
        e.deviceName = q.value("device_name").toString();
        e.eventType = q.value("event_type").toString();
        e.action = q.value("action").toString();
        e.severity = q.value("severity").toString();
        e.timestamp = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
        e.rawLog = q.value("raw_log").toString();
        e.location = q.value("location").toString();
        events.append(e);
    }
    return events;
}

int DatabaseService::getTotalEventsCount() const {
    if (!m_opened) return 0;
    QSqlQuery q(database());
    if (!q.exec("SELECT COUNT(*) FROM events")) return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

bool DatabaseService::createAlert(const Alert &alert) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        INSERT INTO alerts (id, title, description, severity, status, device_name, triggered_at, rule_id, assigned_to, comment, related_event_ids)
        VALUES (:id, :title, :description, :severity, :status, :device_name, :triggered_at, :rule_id, :assigned_to, :comment, :related_event_ids)
    )");
    q.bindValue(":id", alert.id.isEmpty() ? generateId() : alert.id);
    q.bindValue(":title", alert.title);
    q.bindValue(":description", alert.description);
    q.bindValue(":severity", alert.severity);
    q.bindValue(":status", alert.status);
    q.bindValue(":device_name", alert.deviceName);
    q.bindValue(":triggered_at", alert.triggeredAt.isValid() ? alert.triggeredAt.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":rule_id", alert.ruleId);
    q.bindValue(":assigned_to", alert.assignedTo);
    q.bindValue(":comment", alert.comment);
    q.bindValue(":related_event_ids", alert.relatedEventIds.join(","));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::updateAlert(const Alert &alert) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        UPDATE alerts SET
            title = :title,
            description = :description,
            severity = :severity,
            status = :status,
            device_name = :device_name,
            triggered_at = :triggered_at,
            rule_id = :rule_id,
            assigned_to = :assigned_to,
            comment = :comment,
            related_event_ids = :related_event_ids
        WHERE id = :id
    )");
    q.bindValue(":id", alert.id);
    q.bindValue(":title", alert.title);
    q.bindValue(":description", alert.description);
    q.bindValue(":severity", alert.severity);
    q.bindValue(":status", alert.status);
    q.bindValue(":device_name", alert.deviceName);
    q.bindValue(":triggered_at", alert.triggeredAt.isValid() ? alert.triggeredAt.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":rule_id", alert.ruleId);
    q.bindValue(":assigned_to", alert.assignedTo);
    q.bindValue(":comment", alert.comment);
    q.bindValue(":related_event_ids", alert.relatedEventIds.join(","));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

QVector<Alert> DatabaseService::getActiveAlerts() const {
    QVector<Alert> alerts;
    if (!m_opened) return alerts;

    QSqlQuery q(database());
    if (!q.exec(R"(
        SELECT id, title, description, severity, status, device_name, triggered_at, rule_id, assigned_to, comment, related_event_ids
        FROM alerts
        WHERE status = 'open'
        ORDER BY triggered_at DESC
    )")) return alerts;

    while (q.next()) {
        Alert a;
        a.id = q.value("id").toString();
        a.title = q.value("title").toString();
        a.description = q.value("description").toString();
        a.severity = q.value("severity").toString();
        a.status = q.value("status").toString();
        a.deviceName = q.value("device_name").toString();
        a.triggeredAt = QDateTime::fromString(q.value("triggered_at").toString(), Qt::ISODate);
        a.ruleId = q.value("rule_id").toString();
        a.assignedTo = q.value("assigned_to").toString();
        a.comment = q.value("comment").toString();
        a.relatedEventIds = q.value("related_event_ids").toString().split(",", Qt::SkipEmptyParts);
        alerts.append(a);
    }
    return alerts;
}

QVector<Alert> DatabaseService::getAllAlerts() const {
    QVector<Alert> alerts;
    if (!m_opened) return alerts;

    QSqlQuery q(database());
    if (!q.exec(R"(
        SELECT id, title, description, severity, status, device_name, triggered_at, rule_id, assigned_to, comment, related_event_ids
        FROM alerts
        ORDER BY triggered_at DESC
    )")) return alerts;

    while (q.next()) {
        Alert a;
        a.id = q.value("id").toString();
        a.title = q.value("title").toString();
        a.description = q.value("description").toString();
        a.severity = q.value("severity").toString();
        a.status = q.value("status").toString();
        a.deviceName = q.value("device_name").toString();
        a.triggeredAt = QDateTime::fromString(q.value("triggered_at").toString(), Qt::ISODate);
        a.ruleId = q.value("rule_id").toString();
        a.assignedTo = q.value("assigned_to").toString();
        a.comment = q.value("comment").toString();
        a.relatedEventIds = q.value("related_event_ids").toString().split(",", Qt::SkipEmptyParts);
        alerts.append(a);
    }
    return alerts;
}

int DatabaseService::getAlertCountBySeverity(const QString &severity) const {
    if (!m_opened) return 0;
    QSqlQuery q(database());
    q.prepare("SELECT COUNT(*) FROM alerts WHERE severity = :s");
    q.bindValue(":s", severity);
    if (!q.exec()) return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

int DatabaseService::getAlertCountByStatus(const QString &status) const {
    if (!m_opened) return 0;
    QSqlQuery q(database());
    q.prepare("SELECT COUNT(*) FROM alerts WHERE status = :s");
    q.bindValue(":s", status);
    if (!q.exec()) return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

int DatabaseService::getAlertCount() const {
    if (!m_opened) return 0;
    QSqlQuery q(database());
    if (!q.exec("SELECT COUNT(*) FROM alerts")) return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

int DatabaseService::getEventCountBySeverity(const QString &severity) const {
    if (!m_opened) return 0;
    QSqlQuery q(database());
    q.prepare("SELECT COUNT(*) FROM events WHERE severity = :s");
    q.bindValue(":s", severity);
    if (!q.exec()) return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

QVariantList DatabaseService::getTopDevices(int limit) const {
    QVariantList out;
    if (!m_opened) return out;
    QSqlQuery q(database());
    q.prepare(R"(
        SELECT device_name, COUNT(*) AS count
        FROM events
        GROUP BY device_name
        ORDER BY count DESC
        LIMIT :limit
    )");
    q.bindValue(":limit", limit);
    if (!q.exec()) return out;
    while (q.next()) {
        QVariantMap m;
        m["name"] = q.value("device_name").toString();
        m["count"] = q.value("count").toInt();
        out.append(m);
    }
    return out;
}

QVariantList DatabaseService::getActivityLast7Hours() const {
    QVariantList out;
    if (!m_opened) return out;

    const QDateTime nowLocal = QDateTime::currentDateTime();
    const QDateTime nowUtc = nowLocal.toUTC();

    for (int i = 6; i >= 0; --i) {
        const QDateTime fromUtc = nowUtc.addSecs(-i * 3600);
        const QDateTime toUtc = fromUtc.addSecs(3599);

        QSqlQuery q(database());
        q.prepare(R"(
            SELECT COUNT(*)
            FROM events
            WHERE timestamp >= :from AND timestamp <= :to
        )");
        q.bindValue(":from", fromUtc.toString(Qt::ISODate));
        q.bindValue(":to", toUtc.toString(Qt::ISODate));

        int count = 0;
        if (q.exec() && q.next()) count = q.value(0).toInt();

        QVariantMap m;
        m["hour"] = fromUtc.toLocalTime().toString("hh:00");
        m["count"] = count;
        out.append(m);
    }

    return out;
}

int DatabaseService::getEventCount() const {
    return getTotalEventsCount();
}

bool DatabaseService::saveEventForCorrelation(const QString &deviceName, const QString &eventType, const QDateTime &timestamp) {
    if (!m_opened) return false;
    QSqlQuery q(database());
    q.prepare(R"(
        INSERT INTO correlation_history (id, device_name, event_type, created_at)
        VALUES (:id, :device_name, :event_type, :created_at)
    )");
    q.bindValue(":id", generateId());
    q.bindValue(":device_name", deviceName);
    q.bindValue(":event_type", eventType);
    q.bindValue(":created_at", timestamp.isValid() ? timestamp.toUTC().toString(Qt::ISODate) : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QVector<EventRecord> DatabaseService::loadRecentHistory(int maxEvents) const {
    QVector<EventRecord> out;
    if (!m_opened) return out;
    QSqlQuery q(database());
    q.prepare(R"(
        SELECT device_name, event_type, created_at
        FROM correlation_history
        ORDER BY created_at DESC
        LIMIT :limit
    )");
    q.bindValue(":limit", maxEvents);
    if (!q.exec()) return out;
    while (q.next()) {
        EventRecord r;
        r.deviceName = q.value("device_name").toString();
        r.eventType = q.value("event_type").toString();
        r.timestamp = QDateTime::fromString(q.value("created_at").toString(), Qt::ISODate);
        out.append(r);
    }
    return out;
}

bool DatabaseService::clearCorrelationHistory() {
    if (!m_opened) return false;
    QSqlQuery q(database());
    if (!q.exec("DELETE FROM correlation_history")) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::pruneCorrelationHistory(int olderThanSeconds) {
    if (!m_opened) return false;
    QDateTime cutoff = QDateTime::currentDateTimeUtc().addSecs(-olderThanSeconds);
    QSqlQuery q(database());
    q.prepare("DELETE FROM correlation_history WHERE created_at < :cutoff");
    q.bindValue(":cutoff", cutoff.toString(Qt::ISODate));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseService::updateAlertStatus(const QString &alertId, const QString &newStatus) {
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        m_lastError = "Database is not open";
        return false;
    }

    QSqlQuery q(db);
    q.prepare("UPDATE alerts SET status = :status WHERE id = :id");
    q.bindValue(":status", newStatus);
    q.bindValue(":id", alertId);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }

    if (q.numRowsAffected() <= 0) {
        m_lastError = "Alert not found";
        return false;
    }

    return true;
}