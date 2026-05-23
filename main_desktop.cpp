#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDir>

#include "src/services/DatabaseService.h"
#include "src/managers/AuthManager.h"
#include "src/models/UserListModel.h"
#include "src/services/WebSocketService.h"
#include "src/models/RuleListModel.h"
#include "src/models/EventListModel.h"
#include "src/models/AlertListModel.h"
#include "src/models/DashboardStatsModel.h"
#include "src/models/AlertFilterProxyModel.h"
#include "src/engine/CorrelationEngine.h"
#include "src/models/Event.h"
#include "src/core/Logger.h"
#include "src/services/ReportService.h"

static constexpr quint16 WS_PORT = 8080;

// === ИЗМЕНЕНО: единый путь к БД для десктопа и сервера ===
static QString sharedDbPath()
{
    QString base = QDir::homePath() + "/.local/share/SIEMAgent";
    QDir().mkpath(base);
    return base + "/siemagent.db";
}

int main(int argc, char *argv[])
{
    Logger::instance().init("logs");

    QFile envFile(".env");
    if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&envFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            int eq = line.indexOf('=');
            if (eq < 0) continue;
            QString key   = line.left(eq).trimmed();
            QString value = line.mid(eq + 1).trimmed();
            qputenv(key.toUtf8(), value.toUtf8());
        }
        envFile.close();
        qInfo() << "[CONFIG] .env файл загружен";
    }

    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("SIEMAgent");
    QCoreApplication::setApplicationName("SIEMAgent");

    // === ИЗМЕНЕНО: используем sharedDbPath() вместо defaultDbPath() ===
    DatabaseService dbService("siem_ui_connection");
    if (!dbService.openWithPath(sharedDbPath())) {
        qDebug() << "DB open error:" << dbService.lastError();
        return -1;
    }

    ReportService reportService(&dbService);

    QString certPath = "certs/server.crt";
    QString keyPath  = "certs/server.key";

    QFile configFile("data/config/app_config.json");
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
        certPath = config.value("cert_path").toString(certPath);
        keyPath  = config.value("key_path").toString(keyPath);
        configFile.close();
    }

    QString wsSecret = qEnvironmentVariable("SIEM_WS_SECRET");
    if (wsSecret.isEmpty()) {
        qWarning() << "[CONFIG] SIEM_WS_SECRET не задан - "
                      "WebSocket будет работать без аутентификации";
    }

    CorrelationEngine correlationEngine(&dbService);

    WebSocketService wsService(WS_PORT);
    wsService.setSharedSecret(wsSecret);
    wsService.setDatabaseService(&dbService);

    AuthManager  authManager(&dbService);
    UserListModel userListModel(&dbService);
    EventListModel eventListModel(&dbService);
    AlertListModel alertListModel(&dbService);
    DashboardStatsModel statsModel(&dbService);
    RuleListModel ruleListModel(&dbService);

    AlertFilterProxyModel filteredAlertModel;
    filteredAlertModel.setSourceModel(&alertListModel);

    QTimer *cleanupTimer = new QTimer(&app);
    QObject::connect(cleanupTimer, &QTimer::timeout,
                     &correlationEngine, &CorrelationEngine::globalCleanup);
    cleanupTimer->start(300'000);

    QObject::connect(&wsService, &WebSocketService::eventReceived,
                     &eventListModel, &EventListModel::appendNew);
    QObject::connect(&wsService, &WebSocketService::eventReceived,
                     &statsModel,    &DashboardStatsModel::refresh);
    QObject::connect(&wsService, &WebSocketService::alertReceived,
                     &alertListModel, &AlertListModel::refresh);
    QObject::connect(&wsService, &WebSocketService::alertReceived,
                     &statsModel,    &DashboardStatsModel::refresh);

    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &alertListModel,    &AlertListModel::refresh);
    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &statsModel,        &DashboardStatsModel::refresh);

    QObject::connect(&statsModel, &DashboardStatsModel::eventsCleared,
                     &eventListModel, &EventListModel::refresh);
    QObject::connect(&statsModel, &DashboardStatsModel::alertsCleared,
                     &alertListModel, &AlertListModel::refresh);

    QObject::connect(&wsService, &WebSocketService::eventForCorrelation,
                 &correlationEngine, &CorrelationEngine::analyze,
                 Qt::QueuedConnection);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("authManager",         &authManager);
    engine.rootContext()->setContextProperty("userListModel",       &userListModel);
    engine.rootContext()->setContextProperty("wsService",           &wsService);
    engine.rootContext()->setContextProperty("eventListModel",      &eventListModel);
    engine.rootContext()->setContextProperty("alertListModel",      &alertListModel);
    engine.rootContext()->setContextProperty("filteredAlertModel",  &filteredAlertModel);
    engine.rootContext()->setContextProperty("dashboardStatsModel", &statsModel);
    engine.rootContext()->setContextProperty("ruleListModel",       &ruleListModel);
    engine.rootContext()->setContextProperty("wsPort",              static_cast<int>(WS_PORT));
    engine.rootContext()->setContextProperty("reportService", &reportService);

    qRegisterMetaType<Event>("Event");

    qmlRegisterSingletonType(
        QUrl("qrc:/qml/styles/Utils.qml"),
        "Utils", 1, 0, "Utils"
    );

    wsService.start(certPath, keyPath);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qDebug() << "QML load error";
        return -1;
    }

    return app.exec();
}