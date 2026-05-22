#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>
#include <QDebug>

#include "src/core/Logger.h"
#include "src/services/DatabaseService.h"
#include "src/services/WebSocketService.h"
#include "src/services/ReportService.h"
#include "src/engine/CorrelationEngine.h"
#include "src/managers/AuthManager.h"
#include "src/models/UserListModel.h"
#include "src/models/EventListModel.h"
#include "src/models/AlertListModel.h"
#include "src/models/DashboardStatsModel.h"
#include "src/models/RuleListModel.h"
#include "src/services/HttpServer.h"

static constexpr quint16 WS_PORT = 8080;
static constexpr quint16 HTTP_PORT = 8443;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("SIEMAgent");
    QCoreApplication::setApplicationName("SIEMAgent");  

    Logger::instance().init("logs");

    QFile envFile(".env");
    if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&envFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            int eq = line.indexOf('=');
            if (eq < 0) continue;
            QString key = line.left(eq).trimmed();
            QString value = line.mid(eq + 1).trimmed();
            qputenv(key.toUtf8(), value.toUtf8());
        }
        qInfo() << "[CONFIG] .env loaded";
    }

    DatabaseService dbService("siem_server_connection");
    if (!dbService.openWithPath(DatabaseService::defaultDbPath())) {
        qCritical() << "[DB] open failed:" << dbService.lastError();
        return -1;
    }

    if (!dbService.createDefaultAdmin()) {
        qWarning() << "[DB] default admin not created or already exists";
    }

    ReportService reportService(&dbService);
    CorrelationEngine correlationEngine(&dbService);

    AuthManager authManager(&dbService);
    UserListModel userListModel(&dbService);
    EventListModel eventListModel(&dbService);
    AlertListModel alertListModel(&dbService);
    DashboardStatsModel statsModel(&dbService);
    RuleListModel ruleListModel(&dbService);

    WebSocketService wsService(WS_PORT);
    wsService.setSharedSecret(qEnvironmentVariable("SIEM_WS_SECRET"));
    wsService.setDatabaseService(&dbService);

    QObject::connect(&wsService, &WebSocketService::eventReceived,
                     &eventListModel, &EventListModel::appendNew);
    QObject::connect(&wsService, &WebSocketService::eventReceived,
                     &statsModel, &DashboardStatsModel::refresh);
    QObject::connect(&wsService, &WebSocketService::alertReceived,
                     &alertListModel, &AlertListModel::refresh);
    QObject::connect(&wsService, &WebSocketService::alertReceived,
                     &statsModel, &DashboardStatsModel::refresh);

    QObject::connect(&wsService, &WebSocketService::eventForCorrelation,
                     &correlationEngine, &CorrelationEngine::analyze,
                     Qt::QueuedConnection);

    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &alertListModel, &AlertListModel::refresh);
    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &statsModel, &DashboardStatsModel::refresh);

    QObject::connect(&statsModel, &DashboardStatsModel::eventsCleared,
                     &eventListModel, &EventListModel::refresh);
    QObject::connect(&statsModel, &DashboardStatsModel::alertsCleared,
                     &alertListModel, &AlertListModel::refresh);

    QTimer cleanupTimer;
    QObject::connect(&cleanupTimer, &QTimer::timeout,
                     &correlationEngine, &CorrelationEngine::globalCleanup);
    cleanupTimer.start(300000);

    QFile configFile("data/config/app_config.json");
    QString certPath = "certs/server.crt";
    QString keyPath  = "certs/server.key";
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
        certPath = config.value("cert_path").toString(certPath);
        keyPath  = config.value("key_path").toString(keyPath);
    }

    wsService.start(certPath, keyPath);

    HttpServer httpServer;
    httpServer.setDatabaseService(&dbService);
    httpServer.setAuthManager(&authManager);
    httpServer.setReportService(&reportService);
    httpServer.setDashboardStatsModel(&statsModel);
    httpServer.setEventListModel(&eventListModel);
    httpServer.setAlertListModel(&alertListModel);
    httpServer.setUserListModel(&userListModel);
    httpServer.setRuleListModel(&ruleListModel);
    httpServer.setWebRoot("web");
    httpServer.setAllowedOrigin("*");

    QObject::connect(&httpServer, &HttpServer::serverStarted, [](quint16 port) {
        qInfo() << "[HTTP] started on port" << port;
    });
    QObject::connect(&httpServer, &HttpServer::serverError, [](const QString &err) {
        qCritical() << "[HTTP] error:" << err;
    });

    if (!httpServer.start(HTTP_PORT)) {
        return -1;
    }

    qInfo() << "[SERVER] ready";
    return app.exec();
}