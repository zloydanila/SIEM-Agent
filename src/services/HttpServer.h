#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

class DatabaseService;
class AuthManager;
class ReportService;
class DashboardStatsModel;
class EventListModel;
class AlertListModel;
class UserListModel;
class RuleListModel;
class WebSocketWorker;

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);

    bool start(quint16 port);
    bool startWithFallback(quint16 preferredPort, quint16 *boundPort = nullptr);
    void stop();

    void setDatabaseService(DatabaseService *db);
    void setAuthManager(AuthManager *auth);
    void setReportService(ReportService *report);
    void setDashboardStatsModel(DashboardStatsModel *stats);
    void setEventListModel(EventListModel *events);
    void setAlertListModel(AlertListModel *alerts);
    void setUserListModel(UserListModel *users);
    void setRuleListModel(RuleListModel *rules);
    void setWebRoot(const QString &root);
    void setAllowedOrigin(const QString &origin);
    void setWebSocketWorker(WebSocketWorker *worker);
    QJsonArray eventsJson(int limit, int offset) const; 

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void serverError(const QString &error);

private slots:
    void onNewConnection();

private:
    struct Session {
        QString username;
        QString role;
        QString userId;
        qint64 createdAt = 0;
        qint64 expiresAt = 0;
    };

    struct ClientRequest {
        QByteArray buffer;
    };

    QTcpServer m_server;
    QHash<QTcpSocket*, ClientRequest> m_clients;
    QHash<QString, Session> m_sessions;

    DatabaseService *m_db = nullptr;
    AuthManager *m_auth = nullptr;
    ReportService *m_report = nullptr;
    DashboardStatsModel *m_stats = nullptr;
    EventListModel *m_events = nullptr;
    AlertListModel *m_alerts = nullptr;
    UserListModel *m_users = nullptr;
    RuleListModel *m_rules = nullptr;
    WebSocketWorker *m_wsWorker = nullptr;

    QString m_webRoot;
    QString m_allowedOrigin = "*";

    void handleSocket(QTcpSocket *socket);
    void processRequest(QTcpSocket *socket, const QByteArray &request);

    QHash<QByteArray, QByteArray> parseHeaders(const QByteArray &rawRequest);
    QUrl parseUrlFromRequestLine(const QByteArray &requestLine);

    QByteArray statusText(int status);
    void sendResponse(QTcpSocket *socket, int status, const QString &contentType,
                      const QByteArray &body,
                      const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {});
    void sendJson(QTcpSocket *socket, const QJsonObject &obj, int status = 200);
    void sendJson(QTcpSocket *socket, const QJsonArray &arr, int status = 200);
    void sendText(QTcpSocket *socket, const QString &text, int status = 200);
    void sendFile(QTcpSocket *socket, const QString &path);
    QString mimeTypeForFile(const QString &path) const;

    QString bearerTokenFromHeaders(const QHash<QByteArray, QByteArray> &headers) const;
    QString createSession(const QString &username, const QString &role, const QString &userId);
    bool validateSession(const QString &token) const;
    bool isAdminSession(const QString &token) const;
    QJsonObject sessionToJson(const QString &token) const;
    void cleanupSessions();
    bool checkAuth(const QHash<QByteArray, QByteArray> &headers, QString *tokenOut = nullptr) const;
    bool checkAdmin(const QHash<QByteArray, QByteArray> &headers, QString *tokenOut = nullptr) const;
    bool checkOperator(const QHash<QByteArray, QByteArray> &headers, QString *tokenOut = nullptr) const;
    QString getRoleFromToken(const QString &token) const;

    QJsonObject dashboardJson() const;
    QJsonArray eventsJson(int limit) const;
    QJsonArray alertsJson() const;
    QJsonArray usersJson() const;
    QJsonArray rulesJson() const;

    bool handleApiUsers(QTcpSocket *socket, const QByteArray &method, const QString &path,
                        const QHash<QByteArray, QByteArray> &headers, const QByteArray &body);
    bool handleApiRules(QTcpSocket *socket, const QByteArray &method, const QString &path,
                        const QHash<QByteArray, QByteArray> &headers, const QByteArray &body);
    QByteArray extractBody(const QByteArray &request);

    bool handleApiAlerts(QTcpSocket *socket, const QByteArray &method, const QString &path,
                         const QHash<QByteArray, QByteArray> &headers, const QByteArray &body);
};