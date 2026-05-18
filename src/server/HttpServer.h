#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSslSocket>
#include <QJsonObject>
#include "JwtManager.h"

class DatabaseService;
class AuthManager;
class EventListModel;
class AlertListModel;
class DashboardStatsModel;
class UserListModel;
class RuleListModel;
class ReportService;

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(DatabaseService *db, AuthManager *auth,
                        EventListModel *eventModel, AlertListModel *alertModel,
                        DashboardStatsModel *statsModel, UserListModel *userModel,
                        RuleListModel *ruleModel, ReportService *report,
                        const QString &jwtSecret,
                        QObject *parent = nullptr);
    bool start(quint16 port, const QString &certPath, const QString &keyPath);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void handleRequest(QSslSocket *socket, const QByteArray &raw);
    void serveStatic(QSslSocket *socket, const QString &path);
    void serveApi(QSslSocket *socket, const QString &method, const QString &path,
                  const QJsonObject &body, const QString &jwt);
    void sendJson(QSslSocket *socket, const QJsonObject &obj, int statusCode = 200);
    void sendError(QSslSocket *socket, int statusCode, const QString &message);
    QJsonObject extractJsonBody(const QByteArray &request);
    QString extractJwt(const QByteArray &request);

    QTcpServer *m_server;
    DatabaseService *m_db;
    AuthManager *m_auth;
    EventListModel *m_eventModel;
    AlertListModel *m_alertModel;
    DashboardStatsModel *m_statsModel;
    UserListModel *m_userModel;
    RuleListModel *m_ruleModel;
    ReportService *m_report;
    JwtManager *m_jwt;
};

#endif