#include "HttpServer.h"
#include "../services/DatabaseService.h"
#include "../managers/AuthManager.h"
#include "../models/EventListModel.h"
#include "../models/AlertListModel.h"
#include "../models/DashboardStatsModel.h"
#include "../models/UserListModel.h"
#include "../models/RuleListModel.h"
#include "../services/ReportService.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

HttpServer::HttpServer(DatabaseService *db, AuthManager *auth,
                       EventListModel *eventModel, AlertListModel *alertModel,
                       DashboardStatsModel *statsModel, UserListModel *userModel,
                       RuleListModel *ruleModel, ReportService *report,
                       const QString &jwtSecret, QObject *parent)
    : QObject(parent), m_db(db), m_auth(auth), m_eventModel(eventModel),
      m_alertModel(alertModel), m_statsModel(statsModel), m_userModel(userModel),
      m_ruleModel(ruleModel), m_report(report) {
    m_jwt = new JwtManager(jwtSecret);
}

bool HttpServer::start(quint16 port, const QString &certPath, const QString &keyPath) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
    return m_server->listen(QHostAddress::Any, port);
}

void HttpServer::onNewConnection() {
    while (QTcpSocket *raw = m_server->nextPendingConnection()) {
        QSslSocket *ssl = new QSslSocket(this);
        ssl->setSocketDescriptor(raw->socketDescriptor());
        QFile certFile("certs/server.crt");
        QFile keyFile("certs/server.key");
        certFile.open(QIODevice::ReadOnly);
        keyFile.open(QIODevice::ReadOnly);
        ssl->setLocalCertificate(QSslCertificate(&certFile, QSsl::Pem));
        ssl->setPrivateKey(QSslKey(&keyFile, QSsl::Rsa, QSsl::Pem));
        ssl->startServerEncryption();
        connect(ssl, &QSslSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(ssl, &QSslSocket::disconnected, ssl, &QObject::deleteLater);
    }
}

void HttpServer::onReadyRead() {
    QSslSocket *s = qobject_cast<QSslSocket*>(sender());
    if (!s) return;
    handleRequest(s, s->readAll());
}

void HttpServer::handleRequest(QSslSocket *socket, const QByteArray &raw) {
    QString req = QString::fromUtf8(raw);
    QStringList lines = req.split("\r\n");
    if (lines.isEmpty()) return;
    QStringList first = lines[0].split(' ');
    if (first.size() < 2) return;
    QString method = first[0];
    QString path = first[1];
    QJsonObject body = extractJsonBody(raw);
    QString jwt = extractJwt(raw);
    if (path.startsWith("/api/")) {
        serveApi(socket, method, path, body, jwt);
    } else {
        serveStatic(socket, path);
    }
}

void HttpServer::serveApi(QSslSocket *socket, const QString &method, const QString &path,
                          const QJsonObject &body, const QString &jwt) {
    QJsonObject jwtPayload;
    bool isAuth = (path == "/api/auth/login") ? true : m_jwt->verifyToken(jwt, jwtPayload);
    if (!isAuth) { sendError(socket, 401, "Unauthorized"); return; }
    QString role = jwtPayload["role"].toString();
    QString userId = jwtPayload["id"].toString();

    if (path == "/api/auth/login" && method == "POST") {
        User user;
        if (m_auth->authenticate(body["username"].toString(), body["password"].toString(), user)) {
            QJsonObject p;
            p["id"] = user.id; p["username"] = user.username; p["role"] = user.role;
            p["fullName"] = user.fullName; p["email"] = user.email;
            QString token = m_jwt->generateToken(p);
            QJsonObject res;
            res["token"] = token; res["user"] = p;
            sendJson(socket, res);
        } else sendError(socket, 401, "Invalid credentials");
    }
    else if (path == "/api/dashboard" && method == "GET") {
        m_statsModel->refresh();
        QJsonObject d;
        d["totalEvents"] = m_statsModel->totalEvents();
        d["totalAlerts"] = m_statsModel->totalAlerts();
        d["openAlerts"] = m_statsModel->openAlerts();
        d["criticalCount"] = m_statsModel->criticalCount();
        d["highCount"] = m_statsModel->highCount();
        d["mediumCount"] = m_statsModel->mediumCount();
        d["lowCount"] = m_statsModel->lowCount();
        QJsonArray topDev;
        for (const QVariant &v : m_statsModel->topDevices()) {
            QVariantMap m = v.toMap();
            QJsonObject o; o["name"] = m["name"].toString(); o["count"] = m["count"].toInt();
            topDev.append(o);
        }
        d["topDevices"] = topDev;
        QJsonArray act;
        const QVariantList ad = m_statsModel->activityData();
        const QStringList al = m_statsModel->activityLabels();
        for (int i = 0; i < ad.size(); i++) {
            QJsonObject o; o["hour"] = al[i]; o["count"] = ad[i].toInt();
            act.append(o);
        }
        d["activity"] = act;
        sendJson(socket, d);
    }
    else if (path == "/api/events" && method == "GET") {
        QVector<Event> events = m_db->getRecentEvents(200);
        QJsonArray arr;
        for (const Event &e : events) arr.append(e.toJson());
        QJsonObject res; res["events"] = arr; res["total"] = m_db->getTotalEventsCount();
        sendJson(socket, res);
    }
    else if (path == "/api/alerts" && method == "GET") {
        QVector<Alert> alerts = m_db->getAllAlerts();
        QJsonArray arr;
        for (const Alert &a : alerts) arr.append(a.toJson());
        QJsonObject res; res["alerts"] = arr; res["total"] = m_db->getAlertCount();
        sendJson(socket, res);
    }
    else if (path.startsWith("/api/alerts/") && path.endsWith("/status") && method == "PATCH") {
        QString alertId = path.section('/', 2, 2);
        m_alertModel->updateStatus(alertId, body["status"].toString());
        sendJson(socket, {{"success", true}});
    }
    else if (path == "/api/users" && method == "GET" && role == "admin") {
        QVector<User> users = m_db->getAllUsers();
        QJsonArray arr;
        for (const User &u : users) {
            QJsonObject o;
            o["id"] = u.id; o["username"] = u.username; o["role"] = u.role;
            o["fullName"] = u.fullName; o["email"] = u.email; o["isActive"] = u.isActive;
            arr.append(o);
        }
        sendJson(socket, {{"users", arr}});
    }
    else if (path == "/api/users" && method == "POST" && role == "admin") {
        User newUser;
        newUser.username = body["username"].toString();
        newUser.role = body["role"].toString();
        newUser.fullName = body["fullName"].toString();
        newUser.email = body["email"].toString();
        newUser.isActive = true;
        newUser.mustChangePassword = true;
        newUser.setPassword(body["password"].toString(), m_db->generateSalt());
        if (m_db->createUser(newUser)) sendJson(socket, {{"success", true}});
        else sendError(socket, 500, m_db->lastError());
    }
    else if (path.startsWith("/api/users/") && method == "DELETE" && role == "admin") {
        QString uid = path.section('/', 2, 2);
        if (m_db->deleteUser(uid)) sendJson(socket, {{"success", true}});
        else sendError(socket, 500, m_db->lastError());
    }
    else if (path == "/api/rules" && method == "GET") {
        QVector<Rule> rules = m_db->getAllRules();
        QJsonArray arr;
        for (const Rule &r : rules) {
            QJsonObject o;
            o["id"] = r.id; o["name"] = r.name; o["ruleType"] = r.ruleType;
            o["matchEventType"] = r.matchEventType; o["secondaryEventType"] = r.secondaryEventType;
            o["threshold"] = r.threshold; o["windowSeconds"] = r.windowSeconds;
            o["cooldownSeconds"] = r.cooldownSeconds; o["alertSeverity"] = r.alertSeverity;
            o["alertTitle"] = r.alertTitle; o["isEnabled"] = r.isEnabled;
            arr.append(o);
        }
        sendJson(socket, {{"rules", arr}});
    }
    else if (path == "/api/rules" && method == "POST" && role == "admin") {
        Rule rule;
        rule.name = body["name"].toString();
        rule.ruleType = body["ruleType"].toString();
        rule.matchEventType = body["matchEventType"].toString();
        rule.secondaryEventType = body["secondaryEventType"].toString();
        rule.threshold = body["threshold"].toInt(1);
        rule.windowSeconds = body["windowSeconds"].toInt(60);
        rule.cooldownSeconds = body["cooldownSeconds"].toInt(60);
        rule.alertSeverity = body["alertSeverity"].toString();
        rule.alertTitle = body["alertTitle"].toString();
        rule.alertDescription = body["alertDescription"].toString();
        rule.isEnabled = true;
        if (m_db->createRule(rule)) sendJson(socket, {{"success", true}});
        else sendError(socket, 500, m_db->lastError());
    }
    else if (path.startsWith("/api/rules/") && method == "DELETE" && role == "admin") {
        QString rid = path.section('/', 2, 2);
        if (m_db->deleteRule(rid)) sendJson(socket, {{"success", true}});
        else sendError(socket, 500, m_db->lastError());
    }
    else if (path.startsWith("/api/rules/") && path.endsWith("/toggle") && method == "PATCH" && role == "admin") {
        QString rid = path.section('/', 2, 2);
        bool enabled = body["enabled"].toBool(true);
        if (m_db->setRuleEnabled(rid, enabled)) sendJson(socket, {{"success", true}});
        else sendError(socket, 500, m_db->lastError());
    }
    else if (path == "/api/export/events/csv" && method == "GET") {
        QString fp = m_report->defaultExportPath("events") + ".csv";
        m_report->exportEventsCsv(fp);
        sendJson(socket, {{"file", fp}});
    }
    else if (path == "/api/export/alerts/csv" && method == "GET") {
        QString fp = m_report->defaultExportPath("alerts") + ".csv";
        m_report->exportAlertsCsv(fp);
        sendJson(socket, {{"file", fp}});
    }
    else if (path == "/api/export/report/json" && method == "GET") {
        QString fp = m_report->defaultExportPath("report") + ".json";
        m_report->exportReportJson(fp);
        sendJson(socket, {{"file", fp}});
    }
    else {
        sendError(socket, 404, "Not found");
    }
}

void HttpServer::serveStatic(QSslSocket *socket, const QString &path) {
    QString filePath = "web" + (path == "/" ? "/index.html" : path);
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) { sendError(socket, 404, "File not found"); return; }
    QByteArray content = f.readAll();
    QString mime = "text/html";
    if (path.endsWith(".css")) mime = "text/css";
    else if (path.endsWith(".js")) mime = "application/javascript";
    else if (path.endsWith(".svg")) mime = "image/svg+xml";
    QByteArray resp = "HTTP/1.1 200 OK\r\nContent-Type: " + mime.toUtf8() + "\r\n"
                      "Content-Length: " + QByteArray::number(content.size()) + "\r\n"
                      "Access-Control-Allow-Origin: *\r\n\r\n" + content;
    socket->write(resp);
    socket->disconnectFromHost();
}

void HttpServer::sendJson(QSslSocket *socket, const QJsonObject &obj, int statusCode) {
    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray resp = QString("HTTP/1.1 %1 OK\r\nContent-Type: application/json\r\n"
                              "Content-Length: %2\r\n\r\n").arg(statusCode).arg(json.size()).toUtf8() + json;
    socket->write(resp);
    socket->disconnectFromHost();
}

void HttpServer::sendError(QSslSocket *socket, int code, const QString &msg) {
    QJsonObject obj; obj["error"] = msg;
    sendJson(socket, obj, code);
}

QJsonObject HttpServer::extractJsonBody(const QByteArray &request) {
    int idx = request.indexOf("\r\n\r\n");
    if (idx == -1) return {};
    QByteArray body = request.mid(idx + 4);
    return QJsonDocument::fromJson(body).object();
}

QString HttpServer::extractJwt(const QByteArray &request) {
    QString req = QString::fromUtf8(request);
    QRegularExpression re("Authorization: Bearer (.+)\\r?\\n");
    QRegularExpressionMatch m = re.match(req);
    if (m.hasMatch()) return m.captured(1);
    return {};
}