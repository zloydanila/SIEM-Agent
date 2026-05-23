#include "HttpServer.h"

#include "../services/DatabaseService.h"
#include "../managers/AuthManager.h"
#include "../services/ReportService.h"
#include "../models/DashboardStatsModel.h"
#include "../models/EventListModel.h"
#include "../models/AlertListModel.h"
#include "../models/UserListModel.h"
#include "../models/RuleListModel.h"
#include "../models/User.h"
#include "../engine/Rule.h"

#include <QFile>
#include <QDir>
#include <QUuid>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUrl>
#include <QCryptographicHash>
#include <QDebug>

static QString normalizeAlertStatus(const QString &status) {
    const QString s = status.trimmed().toLower();
    if (s == "open" || s == "investigating" || s == "closed") return s;
    if (s == "in_progress" || s == "working") return "investigating";
    return {};
}

static constexpr qint64 SESSION_TTL_MS = 24LL * 60 * 60 * 1000;

static QJsonObject ruleToJson(const Rule &r) {
    QJsonObject o;
    o["id"] = r.id;
    o["name"] = r.name;
    o["ruleType"] = r.ruleType;
    o["matchEventType"] = r.matchEventType;
    o["secondaryEventType"] = r.secondaryEventType;
    o["threshold"] = r.threshold;
    o["windowSeconds"] = r.windowSeconds;
    o["cooldownSeconds"] = r.cooldownSeconds;
    o["alertSeverity"] = r.alertSeverity;
    o["alertTitle"] = r.alertTitle;
    o["alertDescription"] = r.alertDescription;
    o["isEnabled"] = r.isEnabled;
    return o;
}

static Rule findRuleInList(DatabaseService *db, const QString &id, bool *ok = nullptr) {
    if (ok) *ok = false;
    Rule result;
    if (!db) return result;
    const auto rules = db->getAllRules();
    for (const auto &r : rules) {
        if (r.id == id) {
            if (ok) *ok = true;
            return r;
        }
    }
    return result;
}

HttpServer::HttpServer(QObject *parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

bool HttpServer::start(quint16 port) {
    if (!m_server.listen(QHostAddress::Any, port)) {
        emit serverError(m_server.errorString());
        return false;
    }
    emit serverStarted(port);
    return true;
}

void HttpServer::stop() {
    m_server.close();
    for (auto socket : m_clients.keys()) {
        if (socket) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    }
    m_clients.clear();
    m_sessions.clear();
    emit serverStopped();
}

void HttpServer::setDatabaseService(DatabaseService *db) { m_db = db; }
void HttpServer::setAuthManager(AuthManager *auth) { m_auth = auth; }
void HttpServer::setReportService(ReportService *report) { m_report = report; }
void HttpServer::setDashboardStatsModel(DashboardStatsModel *stats) { m_stats = stats; }
void HttpServer::setEventListModel(EventListModel *events) { m_events = events; }
void HttpServer::setAlertListModel(AlertListModel *alerts) { m_alerts = alerts; }
void HttpServer::setUserListModel(UserListModel *users) { m_users = users; }
void HttpServer::setRuleListModel(RuleListModel *rules) { m_rules = rules; }
void HttpServer::setWebRoot(const QString &root) { m_webRoot = root; }
void HttpServer::setAllowedOrigin(const QString &origin) { m_allowedOrigin = origin; }

void HttpServer::onNewConnection() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        m_clients.insert(socket, ClientRequest{});
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { handleSocket(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_clients.remove(socket);
            socket->deleteLater();
        });
    }
}

void HttpServer::handleSocket(QTcpSocket *socket) {
    if (!socket) return;
    auto &client = m_clients[socket];
    client.buffer += socket->readAll();
    if (!client.buffer.contains("\r\n\r\n")) return;
    processRequest(socket, client.buffer);
    client.buffer.clear();
    socket->disconnectFromHost();
}

void HttpServer::processRequest(QTcpSocket *socket, const QByteArray &request) {
    cleanupSessions();

    const int headerEnd = request.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        sendText(socket, "Bad Request", 400);
        return;
    }

    const QByteArray headerPart = request.left(headerEnd);
    const QByteArray body = request.mid(headerEnd + 4);

    const QList<QByteArray> lines = headerPart.split('\n');
    if (lines.isEmpty()) {
        sendText(socket, "Bad Request", 400);
        return;
    }

    const QByteArray requestLine = lines.first().trimmed();
    const QList<QByteArray> requestParts = requestLine.split(' ');
    if (requestParts.size() < 2) {
        sendText(socket, "Bad Request", 400);
        return;
    }

    const QByteArray method = requestParts[0];
    const QUrl url = parseUrlFromRequestLine(requestLine);
    const QString path = url.path();
    const QHash<QByteArray, QByteArray> headers = parseHeaders(request);

    if (method == "OPTIONS") {
        sendText(socket, "", 200);
        return;
    }

    if (path == "/" || path.isEmpty()) {
        sendFile(socket, QDir(m_webRoot).filePath("index.html"));
        return;
    }

    if (!path.startsWith("/api/")) {
        const QString relativePath = path.startsWith("/") ? path.mid(1) : path;
        sendFile(socket, QDir(m_webRoot).filePath(relativePath));
        return;
    }

    if (path == "/api/auth/login" && method == "POST") {
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendText(socket, "Invalid JSON", 400);
            return;
        }

        const QJsonObject json = doc.object();
        const QString username = json.value("username").toString().trimmed();
        const QString password = json.value("password").toString();

        if (username.isEmpty() || password.isEmpty() || !m_db) {
            sendText(socket, "Bad credentials", 401);
            return;
        }

        User user = m_db->findUserByUsername(username);
        if (user.id.isEmpty() || !user.isActive || !user.checkPassword(password)) {
            sendText(socket, "Bad credentials", 401);
            return;
        }

        const QString token = createSession(user.username, user.role, user.id);

        QJsonObject out;
        out["ok"] = true;
        out["token"] = token;
        out["user"] = sessionToJson(token);
        out["mustChangePassword"] = user.mustChangePassword;
        sendJson(socket, out);
        return;
    }

    if (path == "/api/auth/logout" && method == "POST") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        m_sessions.remove(token);
        sendJson(socket, QJsonObject{{"ok", true}});
        return;
    }

    if (path == "/api/auth/change-password" && method == "POST") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendText(socket, "Invalid JSON", 400);
            return;
        }

        const QJsonObject json = doc.object();
        const QString currentPassword = json.value("currentPassword").toString();
        const QString newPassword = json.value("newPassword").toString();

        if (!m_db) {
            sendText(socket, "DB error", 500);
            return;
        }

        const Session session = m_sessions.value(token);
        User user = m_db->findUserById(session.userId);
        if (user.id.isEmpty()) {
            sendText(socket, "User not found", 404);
            return;
        }

        if (!user.checkPassword(currentPassword)) {
            sendText(socket, "Текущий пароль введён неверно", 400);
            return;
        }

        if (newPassword.length() < 6) {
            sendText(socket, "Новый пароль должен быть не менее 6 символов", 400);
            return;
        }

        if (newPassword == currentPassword) {
            sendText(socket, "Новый пароль должен отличаться от текущего", 400);
            return;
        }

        user.setPassword(newPassword, m_db->generateSalt());
        user.mustChangePassword = false;

        if (!m_db->updateUser(user)) {
            sendText(socket, m_db->lastError(), 500);
            return;
        }

        m_sessions[token].role = user.role;

        QJsonObject out;
        out["ok"] = true;
        out["mustChangePassword"] = false;
        out["user"] = sessionToJson(token);
        sendJson(socket, out);
        return;
    }

    if (path == "/api/me" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        QJsonObject out;
        out["ok"] = true;
        out["user"] = sessionToJson(token);
        sendJson(socket, out);
        return;
    }

    if (path == "/api/status" && method == "GET") {
        QJsonObject out;
        out["wsRunning"] = true;
        out["wsPort"] = 8080;
        out["wsClients"] = 0;
        sendJson(socket, out);
        return;
    }

    if (path == "/api/dashboard" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        QJsonObject out;
        out["stats"] = dashboardJson();
        out["recentEvents"] = eventsJson(100);
        out["alerts"] = alertsJson();
        sendJson(socket, out);
        return;
    }

    if (path == "/api/events" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        sendJson(socket, eventsJson(100));
        return;
    }

    if (path == "/api/events" && method == "DELETE") {
        QString token;
        if (!checkAdmin(headers, &token)) {
            sendText(socket, "Forbidden", 403);
            return;
        }

        if (!m_db || !m_db->clearEvents()) {
            sendText(socket, m_db ? m_db->lastError() : "DB error", 500);
            return;
        }

        if (m_stats) m_stats->refresh();
        if (m_events) m_events->refresh();

        sendJson(socket, QJsonObject{{"ok", true}});
        return;
    }

    if (path == "/api/alerts" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        sendJson(socket, alertsJson());
        return;
    }

    if (path == "/api/alerts" && method == "DELETE") {
        QString token;
        if (!checkAdmin(headers, &token)) {
            sendText(socket, "Forbidden", 403);
            return;
        }

        if (!m_db || !m_db->clearAlerts()) {
            sendText(socket, m_db ? m_db->lastError() : "DB error", 500);
            return;
        }

        if (m_stats) m_stats->refresh();
        if (m_alerts) m_alerts->refresh();

        sendJson(socket, QJsonObject{{"ok", true}});
        return;
    }

    if (path == "/api/users" || path.startsWith("/api/users/")) {
        if (handleApiUsers(socket, method, path, headers, body)) return;
    }

    if (path == "/api/alerts" || path.startsWith("/api/alerts/")) {
        if (handleApiAlerts(socket, method, path, headers, body)) return;
    }

    if (path == "/api/rules" || path.startsWith("/api/rules/")) {
        if (handleApiRules(socket, method, path, headers, body)) return;
    }

    if (path == "/api/reports/events/csv" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        if (!m_report) {
            sendText(socket, "Report service not available", 500);
            return;
        }

        const QString temp = QDir::temp().filePath("events.csv");
        if (!m_report->exportEventsCsv(temp)) {
            sendText(socket, "Failed", 500);
            return;
        }

        QFile f(temp);
        if (!f.open(QIODevice::ReadOnly)) {
            sendText(socket, "Failed", 500);
            return;
        }

        sendResponse(socket, 200, "text/csv; charset=utf-8", f.readAll());
        return;
    }

    if (path == "/api/reports/alerts/csv" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        if (!m_report) {
            sendText(socket, "Report service not available", 500);
            return;
        }

        const QString temp = QDir::temp().filePath("alerts.csv");
        if (!m_report->exportAlertsCsv(temp)) {
            sendText(socket, "Failed", 500);
            return;
        }

        QFile f(temp);
        if (!f.open(QIODevice::ReadOnly)) {
            sendText(socket, "Failed", 500);
            return;
        }

        sendResponse(socket, 200, "text/csv; charset=utf-8", f.readAll());
        return;
    }

    if (path == "/api/reports/report/json" && method == "GET") {
        QString token;
        if (!checkAuth(headers, &token)) {
            sendText(socket, "Unauthorized", 401);
            return;
        }

        if (!m_report) {
            sendText(socket, "Report service not available", 500);
            return;
        }

        const QString temp = QDir::temp().filePath("report.json");
        if (!m_report->exportReportJson(temp)) {
            sendText(socket, "Failed", 500);
            return;
        }

        QFile f(temp);
        if (!f.open(QIODevice::ReadOnly)) {
            sendText(socket, "Failed", 500);
            return;
        }

        sendResponse(socket, 200, "application/json; charset=utf-8", f.readAll());
        return;
    }

    sendText(socket, "Not Found", 404);
}

QHash<QByteArray, QByteArray> HttpServer::parseHeaders(const QByteArray &rawRequest) {
    QHash<QByteArray, QByteArray> headers;
    const int end = rawRequest.indexOf("\r\n\r\n");
    const QByteArray headerPart = end >= 0 ? rawRequest.left(end) : rawRequest;
    const QList<QByteArray> lines = headerPart.split('\n');

    for (int i = 1; i < lines.size(); ++i) {
        QByteArray line = lines[i].trimmed();
        if (line.isEmpty()) continue;

        const int pos = line.indexOf(':');
        if (pos <= 0) continue;

        QByteArray key = line.left(pos).trimmed().toLower();
        QByteArray value = line.mid(pos + 1).trimmed();
        headers.insert(key, value);
    }

    return headers;
}

QUrl HttpServer::parseUrlFromRequestLine(const QByteArray &requestLine) {
    const QList<QByteArray> parts = requestLine.split(' ');
    if (parts.size() < 2) return {};
    return QUrl(QString::fromUtf8(parts[1]));
}

QByteArray HttpServer::statusText(int status) {
    switch (status) {
    case 200: return "200 OK";
    case 201: return "201 Created";
    case 400: return "400 Bad Request";
    case 401: return "401 Unauthorized";
    case 403: return "403 Forbidden";
    case 404: return "404 Not Found";
    case 405: return "405 Method Not Allowed";
    case 500: return "500 Internal Server Error";
    default: return QByteArray::number(status) + " OK";
    }
}

QByteArray HttpServer::extractBody(const QByteArray &request) {
    const int idx = request.indexOf("\r\n\r\n");
    if (idx < 0) return {};
    return request.mid(idx + 4);
}

void HttpServer::sendResponse(QTcpSocket *socket, int status, const QString &contentType,
                              const QByteArray &body,
                              const QList<QPair<QByteArray, QByteArray>> &extraHeaders) {
    if (!socket) return;

    QByteArray response;
    response += "HTTP/1.1 " + statusText(status) + "\r\n";
    response += "Content-Type: " + contentType.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Access-Control-Allow-Origin: " + m_allowedOrigin.toUtf8() + "\r\n";
    response += "Access-Control-Allow-Headers: Authorization, Content-Type\r\n";
    response += "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n";

    for (const auto &h : extraHeaders) {
        response += h.first + ": " + h.second + "\r\n";
    }

    response += "\r\n";
    response += body;

    socket->write(response);
    socket->flush();
}

void HttpServer::sendJson(QTcpSocket *socket, const QJsonObject &obj, int status) {
    sendResponse(socket, status, "application/json; charset=utf-8",
                 QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void HttpServer::sendJson(QTcpSocket *socket, const QJsonArray &arr, int status) {
    sendResponse(socket, status, "application/json; charset=utf-8",
                 QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void HttpServer::sendText(QTcpSocket *socket, const QString &text, int status) {
    sendResponse(socket, status, "text/plain; charset=utf-8", text.toUtf8());
}

QString HttpServer::mimeTypeForFile(const QString &path) const {
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "js") return "application/javascript; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "ico") return "image/x-icon";
    return "application/octet-stream";
}

void HttpServer::sendFile(QTcpSocket *socket, const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        sendText(socket, "File not found", 404);
        return;
    }

    sendResponse(socket, 200, mimeTypeForFile(path), file.readAll());
}

QString HttpServer::bearerTokenFromHeaders(const QHash<QByteArray, QByteArray> &headers) const {
    const QByteArray auth = headers.value("authorization");
    if (!auth.startsWith("Bearer ")) return {};
    return QString::fromUtf8(auth.mid(7).trimmed());
}

QString HttpServer::createSession(const QString &username, const QString &role, const QString &userId) {
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces) +
                          QUuid::createUuid().toString(QUuid::WithoutBraces);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_sessions[token] = Session{ username, role, userId, now, now + SESSION_TTL_MS };
    return token;
}

bool HttpServer::validateSession(const QString &token) const {
    if (token.isEmpty()) return false;
    auto it = m_sessions.constFind(token);
    if (it == m_sessions.constEnd()) return false;
    return QDateTime::currentMSecsSinceEpoch() <= it->expiresAt;
}

bool HttpServer::isAdminSession(const QString &token) const {
    if (!validateSession(token)) return false;
    return m_sessions.value(token).role.trimmed().toLower() == "admin";
}

QJsonObject HttpServer::sessionToJson(const QString &token) const {
    QJsonObject obj;
    if (!validateSession(token)) return obj;

    const Session s = m_sessions.value(token);
    obj["token"] = token;
    obj["username"] = s.username;
    obj["role"] = s.role;
    obj["id"] = s.userId;
    obj["userId"] = s.userId;
    obj["expiresAt"] = QDateTime::fromMSecsSinceEpoch(s.expiresAt).toString(Qt::ISODate);

    if (m_db) {
        const User user = m_db->findUserById(s.userId);
        if (!user.id.isEmpty()) {
            obj["fullName"] = user.fullName;
            obj["email"] = user.email;
            obj["isActive"] = user.isActive;
            obj["mustChangePassword"] = user.mustChangePassword;
        }
    }

    return obj;
}

void HttpServer::cleanupSessions() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QString> toRemove;

    for (auto it = m_sessions.cbegin(); it != m_sessions.cend(); ++it) {
        if (now > it->expiresAt) {
            toRemove.push_back(it.key());
        }
    }

    for (const auto &k : toRemove) {
        m_sessions.remove(k);
    }
}

bool HttpServer::checkAuth(const QHash<QByteArray, QByteArray> &headers, QString *tokenOut) const {
    const QString token = bearerTokenFromHeaders(headers);
    if (tokenOut) *tokenOut = token;
    return validateSession(token);
}

bool HttpServer::checkAdmin(const QHash<QByteArray, QByteArray> &headers, QString *tokenOut) const {
    const QString token = bearerTokenFromHeaders(headers);
    if (tokenOut) *tokenOut = token;
    return isAdminSession(token);
}

bool HttpServer::checkOperator(const QHash<QByteArray, QByteArray> &headers, QString *tokenOut) const {
    const QString token = bearerTokenFromHeaders(headers);
    if (tokenOut) *tokenOut = token;
    if (!validateSession(token)) return false;

    const QString role = m_sessions.value(token).role.trimmed().toLower();
    return role == "admin" || role == "operator";
}

QString HttpServer::getRoleFromToken(const QString &token) const {
    if (!validateSession(token)) return "";
    return m_sessions.value(token).role;
}

QJsonObject HttpServer::dashboardJson() const {
    QJsonObject obj;
    if (!m_db) return obj;

    obj["criticalCount"] = m_db->getEventCountBySeverity("critical");
    obj["highCount"] = m_db->getEventCountBySeverity("high");
    obj["mediumCount"] = m_db->getEventCountBySeverity("medium");
    obj["lowCount"] = m_db->getEventCountBySeverity("low");
    obj["totalEvents"] = m_db->getTotalEventsCount();
    obj["totalAlerts"] = m_db->getAlertCount();
    obj["openAlerts"] = m_db->getAlertCountByStatus("open");

    QJsonArray topDevices;
    for (const QVariant &v : m_db->getTopDevices(5)) {
        const auto map = v.toMap();
        QJsonObject item;
        item["name"] = map.value("name").toString();
        item["count"] = map.value("count").toInt();
        topDevices.append(item);
    }
    obj["topDevices"] = topDevices;

    QJsonArray activity;
    for (const QVariant &v : m_db->getActivityLast7Hours()) {
        const auto map = v.toMap();
        QJsonObject item;
        item["hour"] = map.value("hour").toString();
        item["count"] = map.value("count").toInt();
        activity.append(item);
    }
    obj["activity"] = activity;

    return obj;
}

QJsonArray HttpServer::eventsJson(int limit) const {
    QJsonArray arr;
    if (!m_db) return arr;

    const auto events = m_db->getRecentEvents(limit);
    for (const auto &e : events) {
        arr.append(e.toJson());
    }

    return arr;
}

QJsonArray HttpServer::alertsJson() const {
    QJsonArray arr;
    if (!m_db) return arr;

    const auto alerts = m_db->getAllAlerts();
    for (const auto &a : alerts) {
        arr.append(a.toJson());
    }

    return arr;
}

QJsonArray HttpServer::usersJson() const {
    QJsonArray arr;
    if (!m_db) return arr;

    const auto users = m_db->getAllUsers();
    for (const auto &u : users) {
        QJsonObject o;
        o["id"] = u.id;
        o["username"] = u.username;
        o["role"] = u.role;
        o["fullName"] = u.fullName;
        o["email"] = u.email;
        o["isActive"] = u.isActive;
        o["mustChangePassword"] = u.mustChangePassword;
        o["createdAt"] = u.createdAt.toString(Qt::ISODate);
        arr.append(o);
    }

    return arr;
}

QJsonArray HttpServer::rulesJson() const {
    QJsonArray arr;
    if (!m_db) return arr;

    const auto rules = m_db->getAllRules();
    for (const auto &r : rules) {
        QJsonObject o;
        o["id"] = r.id;
        o["name"] = r.name;
        o["ruleType"] = r.ruleType;
        o["matchEventType"] = r.matchEventType;
        o["secondaryEventType"] = r.secondaryEventType;
        o["threshold"] = r.threshold;
        o["windowSeconds"] = r.windowSeconds;
        o["cooldownSeconds"] = r.cooldownSeconds;
        o["alertSeverity"] = r.alertSeverity;
        o["alertTitle"] = r.alertTitle;
        o["alertDescription"] = r.alertDescription;
        o["isEnabled"] = r.isEnabled;
        arr.append(o);
    }

    return arr;
}

bool HttpServer::handleApiUsers(QTcpSocket *socket, const QByteArray &method, const QString &path,
                                const QHash<QByteArray, QByteArray> &headers, const QByteArray &body) {
    if (!m_db) {
        sendText(socket, "DB error", 500);
        return true;
    }

    QString token;
    if (!checkAdmin(headers, &token)) {
        sendText(socket, "Forbidden", 403);
        return true;
    }

    if (path == "/api/users" && method == "GET") {
        sendJson(socket, usersJson());
        return true;
    }

    if (path == "/api/users" && method == "POST") {
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendText(socket, "Invalid JSON", 400);
            return true;
        }

        const QJsonObject json = doc.object();

        User user;
        user.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        user.username = json.value("username").toString().trimmed();
        user.fullName = json.value("fullName").toString().trimmed();
        user.email = json.value("email").toString().trimmed();
        user.role = json.value("role").toString("viewer");
        user.isActive = json.value("isActive").toBool(true);
        user.mustChangePassword = json.value("mustChangePassword").toBool(true);
        user.createdAt = QDateTime::currentDateTime();

        const QString password = json.value("password").toString();
        if (user.username.isEmpty() || password.isEmpty()) {
            sendText(socket, "Username and password required", 400);
            return true;
        }

        user.setPassword(password, m_db->generateSalt());
        if (!m_db->createUser(user)) {
            sendText(socket, m_db->lastError(), 500);
            return true;
        }

        if (m_users) m_users->refresh();
        sendJson(socket, user.toJson(), 201);
        return true;
    }

    if (path.startsWith("/api/users/")) {
        const QString id = path.mid(QString("/api/users/").size());
        if (id.isEmpty()) {
            sendText(socket, "Not Found", 404);
            return true;
        }

        if (method == "PUT") {
            QJsonParseError err{};
            const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                sendText(socket, "Invalid JSON", 400);
                return true;
            }

            const QJsonObject json = doc.object();
            User user = m_db->findUserById(id);
            if (user.id.isEmpty()) {
                sendText(socket, "Not Found", 404);
                return true;
            }

            user.username = json.value("username").toString(user.username).trimmed();
            user.fullName = json.value("fullName").toString(user.fullName).trimmed();
            user.email = json.value("email").toString(user.email).trimmed();
            user.role = json.value("role").toString(user.role);
            user.isActive = json.value("isActive").toBool(user.isActive);
            user.mustChangePassword = json.value("mustChangePassword").toBool(user.mustChangePassword);

            const QString password = json.value("password").toString();
            if (!password.isEmpty()) {
                user.setPassword(password, m_db->generateSalt());
            }

            if (!m_db->updateUser(user)) {
                sendText(socket, m_db->lastError(), 500);
                return true;
            }

            if (m_users) m_users->refresh();
            sendJson(socket, user.toJson());
            return true;
        }

        if (method == "DELETE") {
            if (!m_db->deleteUser(id)) {
                sendText(socket, m_db->lastError(), 500);
                return true;
            }

            if (m_users) m_users->refresh();
            sendJson(socket, QJsonObject{{"ok", true}});
            return true;
        }
    }

    return false;
}

bool HttpServer::handleApiRules(QTcpSocket *socket, const QByteArray &method, const QString &path,
                                const QHash<QByteArray, QByteArray> &headers, const QByteArray &body) {
    if (!m_db) {
        sendText(socket, "DB error", 500);
        return true;
    }

    QString token;
    if (!checkAdmin(headers, &token)) {
        sendText(socket, "Forbidden", 403);
        return true;
    }

    if (path == "/api/rules" && method == "GET") {
        sendJson(socket, rulesJson());
        return true;
    }

    if (path == "/api/rules" && method == "POST") {
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendText(socket, "Invalid JSON", 400);
            return true;
        }

        const QJsonObject json = doc.object();

        Rule rule;
        rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        rule.name = json.value("name").toString().trimmed();
        rule.ruleType = json.value("ruleType").toString("threshold");
        rule.matchEventType = json.value("matchEventType").toString().trimmed();
        rule.secondaryEventType = json.value("secondaryEventType").toString().trimmed();
        rule.threshold = json.value("threshold").toInt(1);
        rule.windowSeconds = json.value("windowSeconds").toInt(60);
        rule.cooldownSeconds = json.value("cooldownSeconds").toInt(60);
        rule.alertSeverity = json.value("alertSeverity").toString("high");
        rule.alertTitle = json.value("alertTitle").toString().trimmed();
        rule.alertDescription = json.value("alertDescription").toString().trimmed();
        rule.isEnabled = json.value("isEnabled").toBool(true);

        if (rule.name.isEmpty() || rule.matchEventType.isEmpty() || rule.alertTitle.isEmpty()) {
            sendText(socket, "Missing required fields", 400);
            return true;
        }

        if (!m_db->createRule(rule)) {
            sendText(socket, m_db->lastError(), 500);
            return true;
        }

        if (m_rules) m_rules->refresh();
        sendJson(socket, ruleToJson(rule), 201);
        return true;
    }

    if (path.startsWith("/api/rules/")) {
        const QString id = path.mid(QString("/api/rules/").size());
        if (id.isEmpty()) {
            sendText(socket, "Not Found", 404);
            return true;
        }

        bool found = false;
        Rule rule = findRuleInList(m_db, id, &found);
        if (!found) {
            sendText(socket, "Not Found", 404);
            return true;
        }

        if (method == "PUT") {
            QJsonParseError err{};
            const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                sendText(socket, "Invalid JSON", 400);
                return true;
            }

            const QJsonObject json = doc.object();

            rule.name = json.value("name").toString(rule.name).trimmed();
            rule.ruleType = json.value("ruleType").toString(rule.ruleType);
            rule.matchEventType = json.value("matchEventType").toString(rule.matchEventType).trimmed();
            rule.secondaryEventType = json.value("secondaryEventType").toString(rule.secondaryEventType).trimmed();
            rule.threshold = json.value("threshold").toInt(rule.threshold);
            rule.windowSeconds = json.value("windowSeconds").toInt(rule.windowSeconds);
            rule.cooldownSeconds = json.value("cooldownSeconds").toInt(rule.cooldownSeconds);
            rule.alertSeverity = json.value("alertSeverity").toString(rule.alertSeverity);
            rule.alertTitle = json.value("alertTitle").toString(rule.alertTitle).trimmed();
            rule.alertDescription = json.value("alertDescription").toString(rule.alertDescription).trimmed();
            rule.isEnabled = json.value("isEnabled").toBool(rule.isEnabled);

            if (!m_db->updateRule(rule)) {
                sendText(socket, m_db->lastError(), 500);
                return true;
            }

            if (m_rules) m_rules->refresh();
            sendJson(socket, ruleToJson(rule), 200);
            return true;
        }

        if (method == "PATCH") {
            QJsonParseError err{};
            const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                sendText(socket, "Invalid JSON", 400);
                return true;
            }

            const QJsonObject json = doc.object();
            rule.isEnabled = json.value("isEnabled").toBool(rule.isEnabled);

            if (!m_db->updateRule(rule)) {
                sendText(socket, m_db->lastError(), 500);
                return true;
            }

            if (m_rules) m_rules->refresh();
            sendJson(socket, QJsonObject{{"ok", true}, {"id", id}, {"isEnabled", rule.isEnabled}});
            return true;
        }

        if (method == "DELETE") {
            if (!m_db->deleteRule(id)) {
                sendText(socket, m_db->lastError(), 500);
                return true;
            }

            if (m_rules) m_rules->refresh();
            sendJson(socket, QJsonObject{{"ok", true}});
            return true;
        }
    }

    return false;
}

bool HttpServer::handleApiAlerts(QTcpSocket *socket, const QByteArray &method, const QString &path,
                                 const QHash<QByteArray, QByteArray> &headers, const QByteArray &body) {
    if (!m_db) {
        sendText(socket, "DB error", 500);
        return true;
    }

    QString token;
    if (!checkAuth(headers, &token)) {
        sendText(socket, "Unauthorized", 401);
        return true;
    }

    if (path == "/api/alerts" && method == "GET") {
        sendJson(socket, alertsJson());
        return true;
    }

    if (path.startsWith("/api/alerts/") && method == "PATCH") {
        if (!checkOperator(headers, &token)) {
            sendText(socket, "Forbidden", 403);
            return true;
        }

        QString id = path.mid(QString("/api/alerts/").size());
        if (id.endsWith("/status")) {
            id.chop(QString("/status").size());
        }

        if (id.isEmpty()) {
            sendText(socket, "Not Found", 404);
            return true;
        }

        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendText(socket, "Invalid JSON", 400);
            return true;
        }

        const QJsonObject json = doc.object();
        const QString newStatus = normalizeAlertStatus(json.value("status").toString());
        if (newStatus.isEmpty()) {
            sendText(socket, "Invalid status", 400);
            return true;
        }

        if (!m_db->updateAlertStatus(id, newStatus)) {
            sendText(socket, m_db->lastError().isEmpty() ? "Failed to update alert" : m_db->lastError(), 500);
            return true;
        }

        if (m_alerts) m_alerts->refresh();
        if (m_stats) m_stats->refresh();

        sendJson(socket, QJsonObject{
                             {"ok", true},
                             {"id", id},
                             {"status", newStatus}
                         }, 200);
        return true;
    }

    return false;
}