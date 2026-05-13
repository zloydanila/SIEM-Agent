#include "DatabaseService.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QDateTime>
#include <QVariant>
#include <QRandomGenerator>
#include <QDebug>

DatabaseService::DatabaseService(const QString &connectionName, QObject *parent)
    : QObject(parent),
      m_connectionName(connectionName),
      m_opened(false) {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.siem_agent";
    }
    QDir().mkpath(baseDir);
    m_dbPath = baseDir + "/siem_agent.db";
}

DatabaseService::~DatabaseService() {
    close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase DatabaseService::database() const {
    return QSqlDatabase::database(m_connectionName);
}

bool DatabaseService::open() {
    if (QSqlDatabase::contains(m_connectionName)) {
        auto db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) {
            m_opened = true;
            return true;
        }
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        m_opened = false;
        return false;
    }

    m_opened = true;
    return initSchema();
}

void DatabaseService::close() {
    if (QSqlDatabase::contains(m_connectionName)) {
        auto db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) {
            db.close();
        }
    }
    m_opened = false;
}

bool DatabaseService::executeQuery(const QString &queryText) {
    QSqlQuery query(database());
    if (!query.exec(queryText)) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseService::initSchema() {
    if (!m_opened) {
        return false;
    }

    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT NOT NULL,
            full_name TEXT,
            email TEXT,
            is_active INTEGER NOT NULL DEFAULT 1,
            must_change_password INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL
        )
    )";

    const QString eventsSql = R"(
        CREATE TABLE IF NOT EXISTS events(
            id TEXT PRIMARY KEY,
            device_name TEXT NOT NULL,
            event_type TEXT NOT NULL,
            action TEXT NOT NULL,
            severity TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            raw_log TEXT,
            location TEXT
        )
    )";

    const QString alertsSql = R"(
        CREATE TABLE IF NOT EXISTS alerts(
            id TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            description TEXT,
            severity TEXT NOT NULL,
            status TEXT NOT NULL,
            device_name TEXT,
            triggered_at TEXT NOT NULL,
            rule_id TEXT,
            assigned_to TEXT,
            comment TEXT
        )
    )";

    const QString ruleSql = R"(
        CREATE TABLE IF NOT EXISTS rules(
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            rule_type TEXT NOT NULL DEFAULT 'threshold',
            match_event_type TEXT NOT NULL,
            secondary_event_type TEXT,
            threshold INTEGER NOT NULL DEFAULT 1,
            window_seconds INTEGER NOT NULL DEFAULT 60,
            cooldown_seconds INTEGER NOT NULL DEFAULT 60,
            alert_severity TEXT NOT NULL DEFAULT 'high',
            alert_title TEXT NOT NULL,
            alert_description TEXT,
            is_enabled INTEGER NOT NULL DEFAULT 1
        )
    )";

    const QString historySql = R"(
        CREATE TABLE IF NOT EXISTS correlation_history (
            id TEXT PRIMARY KEY,
            device_name TEXT NOT NULL,
            event_type TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            created_at TEXT NOT NULL
        )
    )";


    const QString indexesSql[] = {
        "CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_events_severity ON events(severity)",
        "CREATE INDEX IF NOT EXISTS idx_events_device ON events(device_name)",
        "CREATE INDEX IF NOT EXISTS idx_events_type ON events(event_type)",
        "CREATE INDEX IF NOT EXISTS idx_alerts_status ON alerts(status)",
        "CREATE INDEX IF NOT EXISTS idx_alerts_severity ON alerts(severity)",
        "CREATE INDEX IF NOT EXISTS idx_alerts_device ON alerts(device_name)",
    };

    if (!executeQuery(eventsSql)) return false;
    if (!executeQuery(sql)) return false;
    if (!executeQuery(alertsSql)) return false;
    if (!executeQuery(ruleSql)) return false;
    if (!executeQuery(historySql)) return false;

    for (const QString &idxSql : indexesSql) {
        if (!executeQuery(idxSql)) {
            qWarning() << "[DB] Не удалось создать индекс:" << m_lastError;
        }
    }

    QSqlQuery alterQuery(database());
    if (!alterQuery.exec("ALTER TABLE users ADD COLUMN must_change_password INTEGER NOT NULL DEFAULT 0")) {
        QString err = alterQuery.lastError().text();
        if (!err.contains("duplicate column", Qt::CaseInsensitive)) {
            m_lastError = err;
            qWarning() << "[DB] ALTER TABLE users failed:" << err;
        }
    }

    seedDefaultRules();
    return createDefaultAdmin();
}

QString DatabaseService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString DatabaseService::generateSalt() const {
    QByteArray salt(32, Qt::Uninitialized);
    auto rng = QRandomGenerator::securelySeeded();
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<char>(rng.generate() & 0xFF);
    }
    return salt.toHex();
}

QString DatabaseService::generateRandomPassword(int length) const {
    const QString chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*";
    QString password;
    auto rng = QRandomGenerator::securelySeeded();
    for (int i = 0; i < length; ++i) {
        password.append(chars[rng.bounded(chars.size())]);
    }
    return password;
}

bool DatabaseService::createDefaultAdmin() {
    if (userExists("admin")) {
        return true;
    }

    QString randomPassword = generateRandomPassword(16);

    fprintf(stderr, "SIEM AGENT - Первый запуск\n");

    fprintf(stderr, "Login:    admin\n");
    fprintf(stderr, "Password: %-30s\n",
            randomPassword.toUtf8().constData());
    fprintf(stderr, "Поменяйте пароль после входа\n");

    User admin;
    admin.id              = generateId();
    admin.fullName        = "System Administrator";
    admin.username        = "admin";
    admin.role            = "admin";
    admin.email           = "admin@localhost";
    admin.isActive        = true;
    admin.mustChangePassword = true;
    admin.createdAt       = QDateTime::currentDateTime();
    admin.setPassword(randomPassword, generateSalt());

    return createUser(admin);
}

bool DatabaseService::createUser(const User &user) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO users (
            id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        ) VALUES (
            :id, :username, :password_hash, :salt, :role, :full_name, :email, :is_active,:must_change_password, :created_at
        )
    )");

    query.bindValue(":id", user.id.isEmpty() ? generateId() : user.id);
    query.bindValue(":username", user.username);
    query.bindValue(":password_hash", user.passwordHash);
    query.bindValue(":salt", user.salt);
    query.bindValue(":role", user.role);
    query.bindValue(":full_name", user.fullName);
    query.bindValue(":email", user.email);
    query.bindValue(":must_change_password", user.mustChangePassword ? 1 : 0);
    query.bindValue(":is_active", user.isActive ? 1 : 0);
    query.bindValue(":created_at", user.createdAt.isValid() ? user.createdAt.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseService::updateUser(const User &user) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare(R"(
        UPDATE users SET
            username = :username,
            password_hash = :password_hash,
            salt = :salt,
            role = :role,
            full_name = :full_name,
            email = :email,
            must_change_password = :must_change_password,
            is_active = :is_active
        WHERE id = :id
    )");

    query.bindValue(":id", user.id);
    query.bindValue(":username", user.username);
    query.bindValue(":password_hash", user.passwordHash);
    query.bindValue(":salt", user.salt);
    query.bindValue(":role", user.role);
    query.bindValue(":full_name", user.fullName);
    query.bindValue(":email", user.email);
    query.bindValue(":must_change_password", user.mustChangePassword ? 1 : 0);
    query.bindValue(":is_active", user.isActive ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseService::deleteUser(const QString &userId) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare("DELETE FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseService::setUserActive(const QString &userId, bool active) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare("UPDATE users SET is_active = :active WHERE id = :id");
    query.bindValue(":active", active ? 1 : 0);
    query.bindValue(":id", userId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

User DatabaseService::findUserByUsername(const QString &username) const {
    User user;
    if (!m_opened) return user;

    QSqlQuery query(database());
    query.prepare(R"(
        SELECT id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        FROM users WHERE username = :username LIMIT 1
    )");
    query.bindValue(":username", username);

    if (!query.exec() || !query.next()) {
        return user;
    }

    user.id = query.value("id").toString();
    user.username = query.value("username").toString();
    user.passwordHash = query.value("password_hash").toString();
    user.salt = query.value("salt").toString();
    user.role = query.value("role").toString();
    user.fullName = query.value("full_name").toString();
    user.email = query.value("email").toString();
    user.mustChangePassword = query.value("must_change_password").toInt() == 1;
    user.isActive = query.value("is_active").toInt() == 1;
    user.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);

    return user;
}

User DatabaseService::findUserById(const QString &userId) const {
    User user;
    if (!m_opened) return user;

    QSqlQuery query(database());
    query.prepare(R"(
        SELECT id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        FROM users WHERE id = :id LIMIT 1
    )");
    query.bindValue(":id", userId);

    if (!query.exec() || !query.next()) {
        return user;
    }

    user.id = query.value("id").toString();
    user.username = query.value("username").toString();
    user.passwordHash = query.value("password_hash").toString();
    user.salt = query.value("salt").toString();
    user.role = query.value("role").toString();
    user.fullName = query.value("full_name").toString();
    user.email = query.value("email").toString();
    user.mustChangePassword = query.value("must_change_password").toInt() == 1;
    user.isActive = query.value("is_active").toInt() == 1;
    user.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);

    return user;
}

QVector<User> DatabaseService::getAllUsers() const {
    QVector<User> users;
    if (!m_opened) return users;

    QSqlQuery query(database());
    if (!query.exec(R"(
        SELECT id, username, password_hash, salt, role, full_name, email, is_active, must_change_password, created_at
        FROM users ORDER BY username
    )")) {
        return users;
    }

    while (query.next()) {
        User user;
        user.id = query.value("id").toString();
        user.username = query.value("username").toString();
        user.passwordHash = query.value("password_hash").toString();
        user.salt = query.value("salt").toString();
        user.role = query.value("role").toString();
        user.fullName = query.value("full_name").toString();
        user.email = query.value("email").toString();
        user.isActive = query.value("is_active").toInt() == 1;
        user.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
        users.append(user);
    }

    return users;
}

bool DatabaseService::userExists(const QString &username) const {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare("SELECT 1 FROM users WHERE username = :username LIMIT 1");
    query.bindValue(":username", username);

    if (!query.exec()) {
        return false;
    }

    return query.next();
}

bool DatabaseService::createEvent(const Event &event){
    if (!m_opened) return false;
    QSqlQuery query(database());
    
    query.prepare(R"(
        INSERT INTO events(
            id, device_name, event_type, action, severity, timestamp, raw_log, location)
            VALUES (:id, :device_name, :event_type, :action, :severity, :timestamp, :raw_log, :location)
    )");

    query.bindValue(":id", event.id.isEmpty() ? generateId() : event.id);
    query.bindValue(":device_name", event.deviceName);
    query.bindValue(":event_type", event.eventType);
    query.bindValue(":action", event.action);
    query.bindValue(":severity", event.severity);
    query.bindValue(":timestamp", event.timestamp.toString(Qt::ISODate));
    query.bindValue(":raw_log", event.rawLog);
    query.bindValue(":location", event.location);

    if(!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QVector<Event> DatabaseService::getRecentEvents(int limit, int offset) const{
    QVector<Event> events;
    if(!m_opened) return events;

    QSqlQuery query(database());
    query.prepare("SELECT * FROM events ORDER BY timestamp DESC LIMIT :limit OFFSET :offset");
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);
    if(!query.exec()) return events;

    while (query.next()) {
        Event e;
        e.id = query.value("id").toString();
        e.deviceName = query.value("device_name").toString();
        e.eventType = query.value("event_type").toString();
        e.action = query.value("action").toString();
        e.severity = query.value("severity").toString();
        e.timestamp = QDateTime::fromString(query.value("timestamp").toString(), Qt::ISODate);
        e.rawLog = query.value("raw_log").toString();
        e.location = query.value("location").toString();
        events.append(e);
    }

    return events;
}

QVector<Event> DatabaseService::getEventsPaged(int limit,
                                                const QString &lastId,
                                                const QDateTime &lastTimestamp) const {
    QVector<Event> events;
    if (!m_opened) return events;

    QSqlQuery query(database());

    if (lastId.isEmpty() || !lastTimestamp.isValid()) {
        query.prepare(R"(
            SELECT id, device_name, event_type, action, severity,
                   timestamp, raw_log, location
            FROM events
            ORDER BY timestamp DESC, id DESC
            LIMIT :limit
        )");
        query.bindValue(":limit", limit);
    } else {

        query.prepare(R"(
            SELECT id, device_name, event_type, action, severity,
                   timestamp, raw_log, location
            FROM events
            WHERE (timestamp < :last_ts)
               OR (timestamp = :last_ts2 AND id < :last_id)
            ORDER BY timestamp DESC, id DESC
            LIMIT :limit
        )");
        query.bindValue(":last_ts",  lastTimestamp.toString(Qt::ISODate));
        query.bindValue(":last_ts2", lastTimestamp.toString(Qt::ISODate));
        query.bindValue(":last_id",  lastId);
        query.bindValue(":limit",    limit);
    }

    if (!query.exec()) return events;

    while (query.next()) {
        Event e;
        e.id         = query.value("id").toString();
        e.deviceName = query.value("device_name").toString();
        e.eventType  = query.value("event_type").toString();
        e.action     = query.value("action").toString();
        e.severity   = query.value("severity").toString();
        e.timestamp  = QDateTime::fromString(
                           query.value("timestamp").toString(), Qt::ISODate);
        e.rawLog     = query.value("raw_log").toString();
        e.location   = query.value("location").toString();
        events.append(e);
    }
    return events;
}

QVector<Event> DatabaseService::getEventsByDateRange(const QDateTime &from, const QDateTime &to) const {
    QVector<Event> events;
    if (!m_opened) return events;

    QSqlQuery query(database());
    query.prepare("SELECT * FROM events WHERE timestamp BETWEEN :from AND :to ORDER BY timestamp DESC");
    query.bindValue(":from", from.toString(Qt::ISODate));
    query.bindValue(":to", to.toString(Qt::ISODate));

    if (!query.exec()) return events;

    while (query.next()) {
        Event e;
        e.id = query.value("id").toString();
        e.deviceName = query.value("device_name").toString();
        e.eventType = query.value("event_type").toString();
        e.action = query.value("action").toString();
        e.severity = query.value("severity").toString();
        e.timestamp = QDateTime::fromString(query.value("timestamp").toString(), Qt::ISODate);
        e.rawLog = query.value("raw_log").toString();
        e.location = query.value("location").toString();
        events.append(e);
    }
    return events;
}

int DatabaseService::getTotalEventsCount() const {
    if (!m_opened) return 0;
    QSqlQuery query(database());
    if (query.exec("SELECT COUNT(*) FROM events") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool DatabaseService::createAlert(const Alert &alert) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO alerts (id, title, description, severity, status, device_name, triggered_at, rule_id, assigned_to, comment)
        VALUES (:id, :title, :description, :severity, :status, :device_name, :triggered_at, :rule_id, :assigned_to, :comment)
    )");

    query.bindValue(":id", alert.id.isEmpty() ? generateId() : alert.id);
    query.bindValue(":title", alert.title);
    query.bindValue(":description", alert.description);
    query.bindValue(":severity", alert.severity);
    query.bindValue(":status", alert.status);
    query.bindValue(":device_name", alert.deviceName);
    query.bindValue(":triggered_at", alert.triggeredAt.toString(Qt::ISODate));
    query.bindValue(":rule_id", alert.ruleId);
    query.bindValue(":assigned_to", alert.assignedTo);
    query.bindValue(":comment", alert.comment);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseService::updateAlert(const Alert &alert) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare(R"(
        UPDATE alerts SET
            title = :title,
            description = :description,
            severity = :severity,
            status = :status,
            device_name = :device_name,
            triggered_at = :triggered_at,
            rule_id = :rule_id,
            assigned_to = :assigned_to,
            comment = :comment
        WHERE id = :id
    )");

    query.bindValue(":id", alert.id);
    query.bindValue(":title", alert.title);
    query.bindValue(":description", alert.description);
    query.bindValue(":severity", alert.severity);
    query.bindValue(":status", alert.status);
    query.bindValue(":device_name", alert.deviceName);
    query.bindValue(":triggered_at", alert.triggeredAt.toString(Qt::ISODate));
    query.bindValue(":rule_id", alert.ruleId);
    query.bindValue(":assigned_to", alert.assignedTo);
    query.bindValue(":comment", alert.comment);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

QVector<Alert> DatabaseService::getActiveAlerts() const {
    QVector<Alert> alerts;
    if (!m_opened) return alerts;

    QSqlQuery query(database());
    if (!query.exec("SELECT * FROM alerts WHERE status != 'closed' ORDER BY triggered_at DESC")) {
        return alerts;
    }

    while (query.next()) {
        Alert a;
        a.id = query.value("id").toString();
        a.title = query.value("title").toString();
        a.description = query.value("description").toString();
        a.severity = query.value("severity").toString();
        a.status = query.value("status").toString();
        a.deviceName = query.value("device_name").toString();
        a.triggeredAt = QDateTime::fromString(query.value("triggered_at").toString(), Qt::ISODate);
        a.ruleId = query.value("rule_id").toString();
        a.assignedTo = query.value("assigned_to").toString();
        a.comment = query.value("comment").toString();
        alerts.append(a);
    }
    return alerts;
}

QVector<Alert> DatabaseService::getAllAlerts() const {
    QVector<Alert> alerts;
    if (!m_opened) return alerts;

    QSqlQuery query(database());
    if (!query.exec("SELECT * FROM alerts ORDER BY triggered_at DESC")) {
        return alerts;
    }

    while (query.next()) {
        Alert a;
        a.id = query.value("id").toString();
        a.title = query.value("title").toString();
        a.description = query.value("description").toString();
        a.severity = query.value("severity").toString();
        a.status = query.value("status").toString();
        a.deviceName = query.value("device_name").toString();
        a.triggeredAt = QDateTime::fromString(query.value("triggered_at").toString(), Qt::ISODate);
        a.ruleId = query.value("rule_id").toString();
        a.assignedTo = query.value("assigned_to").toString();
        a.comment = query.value("comment").toString();
        alerts.append(a);
    }
    return alerts;
}

int DatabaseService::getAlertCountBySeverity(const QString &severity) const {
    if (!m_opened) return 0;
    QSqlQuery query(database());
    query.prepare("SELECT COUNT(*) FROM alerts WHERE severity = :severity AND status != 'closed'");
    query.bindValue(":severity", severity);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseService::getAlertCountByStatus(const QString &status) const {
    if (!m_opened) return 0;
    QSqlQuery query(database());
    query.prepare("SELECT COUNT(*) FROM alerts WHERE status = :status");
    query.bindValue(":status", status);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseService::getEventCountBySeverity(const QString &severity) const{
    if(!m_opened) return 0;
    QSqlQuery query(database());
    query.prepare("SELECT COUNT(*) FROM events WHERE severity = :severity");
    query.bindValue(":severity", severity);
    if(query.exec() && query.next()){
        return query.value(0).toInt();
    }
    return 0;
}

QVariantList DatabaseService::getTopDevices(int limit) const{
    QVariantList list;
    if(!m_opened) return list;
    QSqlQuery query(database());
    query.prepare("SELECT device_name, COUNT(*) as cnt FROM events GROUP BY device_name ORDER BY cnt DESC LIMIT :limit");
    query.bindValue(":limit", limit);
    if(query.exec()){
        while(query.next()){
            QVariantMap item;
            item["name"] = query.value("device_name").toString();
            item["count"] = query.value("cnt").toInt();
            list.append(item);
        }
    }
    return list;
}

QVariantList DatabaseService::getActivityLast7Hours() const {
    QVariantList list;
    if (!m_opened) return list;

    QSqlQuery query(database());

    query.prepare(R"(
        SELECT strftime('%H:00', timestamp, 'localtime') AS hour,
               COUNT(*) AS cnt
        FROM events
        WHERE timestamp >= datetime('now', '-7 hours')
        GROUP BY strftime('%Y-%m-%d %H', timestamp, 'localtime')
        ORDER BY strftime('%Y-%m-%d %H', timestamp, 'localtime')
    )");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["hour"]  = query.value("hour").toString();
            item["count"] = query.value("cnt").toInt();
            list.append(item);
        }
    }
    return list;
}

int DatabaseService::getAlertCount() const{
    if(!m_opened) return 0;
    QSqlQuery query(database());
    query.exec("SELECT COUNT(*) FROM alerts");
    return query.next() ? query.value(0).toInt() : 0;
}

int DatabaseService::getEventCount() const{
    if(!m_opened) return 0;
    QSqlQuery query(database());
    query.exec("SELECT COUNT(*) FROM events");
    return query.next() ? query.value(0).toInt() : 0;
}

bool DatabaseService::setMustChangePassword(const QString &userId, bool value){
    if(!m_opened) return false;
    QSqlQuery query(database());
    query.prepare("UPDATE users SET must_change_password = :val WHERE id = :id");
    query.bindValue(":val", value ? 1 : 0);
    query.bindValue(":id", userId);

    if(!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseService::createRule(const Rule &rule) {
    if (!m_opened) return false;
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO rules (
            id, name, rule_type, match_event_type, secondary_event_type, threshold,
            window_seconds, cooldown_seconds, alert_severity, alert_title, alert_description, is_enabled
        ) VALUES (
            :id, :name, :rule_type, :match_event_type, :secondary_event_type, :threshold,
            :window_seconds, :cooldown_seconds, :alert_severity, :alert_title, :alert_description, :is_enabled
        )
    )");

    query.bindValue(":id", rule.id.isEmpty() ? generateId() : rule.id);
    query.bindValue(":name", rule.name);
    query.bindValue(":rule_type", rule.ruleType);
    query.bindValue(":match_event_type",rule.matchEventType);
    query.bindValue(":secondary_event_type", rule.secondaryEventType);
    query.bindValue(":threshold", rule.threshold);
    query.bindValue(":window_seconds", rule.windowSeconds);
    query.bindValue(":cooldown_seconds", rule.cooldownSeconds);
    query.bindValue(":alert_severity", rule.alertSeverity);
    query.bindValue(":alert_title", rule.alertTitle);
    query.bindValue(":alert_description", rule.alertDescription);
    query.bindValue(":is_enabled", rule.isEnabled ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseService::updateRule(const Rule &rule) {
    if (!m_opened) return false;
    QSqlQuery query(database());
    query.prepare(R"(
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

    query.bindValue(":id", rule.id);
    query.bindValue(":name", rule.name);
    query.bindValue(":rule_type", rule.ruleType);
    query.bindValue(":match_event_type", rule.matchEventType);
    query.bindValue(":secondary_event_type", rule.secondaryEventType);
    query.bindValue(":threshold", rule.threshold);
    query.bindValue(":window_seconds", rule.windowSeconds);
    query.bindValue(":cooldown_seconds", rule.cooldownSeconds);
    query.bindValue(":alert_severity", rule.alertSeverity);
    query.bindValue(":alert_title", rule.alertTitle);
    query.bindValue(":alert_description", rule.alertDescription);
    query.bindValue(":is_enabled", rule.isEnabled ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseService::deleteRule(const QString &ruleId) {
    if (!m_opened) return false;
    QSqlQuery query(database());
    query.prepare("DELETE FROM rules WHERE id = :id");
    query.bindValue(":id", ruleId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseService::setRuleEnabled(const QString &ruleId, bool enabled){
    if(!m_opened) return false;
    QSqlQuery query(database());
    query.prepare("UPDATE rules SET is_enabled = :val WHERE id = :id");
    query.bindValue(":val", enabled ? 1 : 0);
    query.bindValue(":id", ruleId);

    if(!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

QVector<Rule> DatabaseService::getAllRules() const {
    QVector<Rule> rules;
    if (!m_opened) return rules;  

    QSqlQuery query(database());
    if (!query.exec("SELECT * FROM rules ORDER BY name")) return rules;

    while (query.next()) {
        Rule r;
        r.id = query.value("id").toString();
        r.name = query.value("name").toString();
        r.ruleType = query.value("rule_type").toString();
        r.matchEventType = query.value("match_event_type").toString();
        r.secondaryEventType = query.value("secondary_event_type").toString();
        r.threshold = query.value("threshold").toInt();
        r.windowSeconds = query.value("window_seconds").toInt();
        r.cooldownSeconds = query.value("cooldown_seconds").toInt();
        r.alertSeverity = query.value("alert_severity").toString();
        r.alertTitle = query.value("alert_title").toString();
        r.alertDescription = query.value("alert_description").toString();
        r.isEnabled = query.value("is_enabled").toInt() == 1;
        rules.append(r);
    }
    return rules;
}

bool DatabaseService::ruleExists(const QString &ruleId) const{
    if(!m_opened) return false;
    QSqlQuery query(database());
    query.prepare("SELECT 1 FROM rules WHERE id = :id LIMIT 1");
    query.bindValue(":id", ruleId);
    if(!query.exec()) return false;
    return query.next();
}

void DatabaseService::seedDefaultRules() {
    QSqlQuery check(database());
    if (check.exec("SELECT COUNT(*) FROM rules") && check.next()) {
        if (check.value(0).toInt() > 0) return;
    }

    QVector<Rule> defaults;

    Rule r1;
    r1.id = "DIRECT_CRITICAL";
    r1.name = "Direct Critical Event Alert";
    r1.ruleType = "threshold";
    r1.matchEventType = "emergency_stop";
    r1.threshold = 1; r1.windowSeconds = 60; r1.cooldownSeconds = 60;
    r1.alertSeverity = "critical";
    r1.alertTitle = "Аварийная остановка";
    r1.alertDescription = "Зафиксирована аварийная остановка оборудования";
    defaults.append(r1);

    Rule r2;
    r2.id = "DIRECT_SAFETY_BYPASS";
    r2.name = "Safety Bypass Alert";
    r2.ruleType = "threshold";
    r2.matchEventType = "safety_bypass";
    r2.threshold = 1; r2.windowSeconds = 60; r2.cooldownSeconds = 60;
    r2.alertSeverity = "critical";
    r2.alertTitle = "Отключение защиты";
    r2.alertDescription = "Система защитных блокировок отключена без разрешения";
    defaults.append(r2);

    Rule r3;
    r3.id = "DIRECT_MALWARE";
    r3.name = "Malware Detected Alert";
    r3.ruleType = "threshold";
    r3.matchEventType = "malware_detected";
    r3.threshold = 1; r3.windowSeconds = 60; r3.cooldownSeconds = 300;
    r3.alertSeverity = "critical";
    r3.alertTitle = "Обнаружена угроза";
    r3.alertDescription = "На устройстве обнаружено вредоносное программное обеспечение";
    defaults.append(r3);


    Rule r4;
    r4.id = "DIRECT_BRUTE_FORCE";
    r4.name = "Brute Force Alert";
    r4.ruleType = "threshold";
    r4.matchEventType = "brute_force";
    r4.threshold = 1; r4.windowSeconds = 60; r4.cooldownSeconds = 120;
    r4.alertSeverity = "high";
    r4.alertTitle = "Подбор пароля";
    r4.alertDescription = "Зафиксирована попытка подбора пароля";
    defaults.append(r4);

    Rule r5;
    r5.id = "DIRECT_UNAUTHORIZED";
    r5.name = "Unauthorized Access Alert";
    r5.ruleType = "threshold";
    r5.matchEventType = "unauthorized_access";
    r5.threshold = 1; r5.windowSeconds = 60; r5.cooldownSeconds = 120;
    r5.alertSeverity = "high";
    r5.alertTitle = "Несанкционированный доступ";
    r5.alertDescription = "Попытка доступа с неизвестного источника";
    defaults.append(r5);

    Rule r6;
    r6.id = "DIRECT_PROCESS_ANOMALY";
    r6.name = "Process Anomaly Alert";
    r6.ruleType = "threshold";
    r6.matchEventType = "process_anomaly";
    r6.threshold = 1; r6.windowSeconds = 60; r6.cooldownSeconds = 120;
    r6.alertSeverity = "high";
    r6.alertTitle = "Аномалия процесса";
    r6.alertDescription = "Показание датчика вышло за допустимый предел";
    defaults.append(r6);


    Rule r7;
    r7.id = "CORR_SUSPICIOUS_BACKUP";
    r7.name = "Suspicious Backup After Auth Failure";
    r7.ruleType = "correlation";
    r7.matchEventType = "config_backup";
    r7.secondaryEventType = "auth_failure";
    r7.threshold = 1; r7.windowSeconds = 300; r7.cooldownSeconds = 300;
    r7.alertSeverity = "high";
    r7.alertTitle = "Подозрительное резервное копирование";
    r7.alertDescription = "Конфигурация скопирована после неудачной авторизации";
    defaults.append(r7);

    for (const Rule &rule : defaults) {
        createRule(rule);
    }
}

bool DatabaseService::clearEvents() {
    if (!m_opened) return false;
    QSqlQuery query(database());
    if (!query.exec("DELETE FROM events")) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseService::clearAlerts() {
    if (!m_opened) return false;
    QSqlQuery query(database());
    if (!query.exec("DELETE FROM alerts")) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseService::saveEventForCorrelation(const QString &deviceName,
                                              const QString &eventType,
                                              const QDateTime &timestamp) {
    if (!m_opened) return false;

    QSqlQuery query(database());
    query.prepare(R"(
        INSERT OR IGNORE INTO correlation_history 
        (id, device_name, event_type, timestamp, created_at)
        VALUES (:id, :device_name, :event_type, :timestamp, :created_at)
    )");

    query.bindValue(":id", generateId());
    query.bindValue(":device_name", deviceName);
    query.bindValue(":event_type", eventType);
    query.bindValue(":timestamp", timestamp.toString(Qt::ISODate));
    query.bindValue(":created_at", QDateTime::currentDateTime().toString(Qt::ISODate));

    return query.exec();
}

QVector<EventRecord> DatabaseService::loadRecentHistory(int maxEvents) const {
    QVector<EventRecord> history;
    if (!m_opened) return history;

    QSqlQuery query(database());
    query.prepare(R"(
        SELECT device_name, event_type, timestamp
        FROM correlation_history
        ORDER BY timestamp DESC
        LIMIT :max_events
    )");
    query.bindValue(":max_events", maxEvents);

    if (!query.exec()) return history;

    while (query.next()) {
        EventRecord rec;
        rec.deviceName = query.value("device_name").toString();
        rec.eventType  = query.value("event_type").toString();
        rec.timestamp  = QDateTime::fromString(
                            query.value("timestamp").toString(), Qt::ISODate);
        history.append(rec);
    }
    return history;
}

bool DatabaseService::clearCorrelationHistory() {
    if (!m_opened) return false;
    QSqlQuery query(database());
    return query.exec("DELETE FROM correlation_history");
}

bool DatabaseService::pruneCorrelationHistory(int olderThanSeconds) {
    if (!m_opened) return false;
    QSqlQuery query(database());
    query.prepare(R"(
        DELETE FROM correlation_history
        WHERE timestamp < datetime('now', :offset)
    )");
    query.bindValue(":offset", QString("-%1 seconds").arg(olderThanSeconds));
    return query.exec();
}

bool DatabaseService::openWithPath(const QString &path) {
    m_dbPath = path;
    return open();
}

QString DatabaseService::lastError() const {
    return m_lastError;
}