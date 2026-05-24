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
#include <QUuid>
#include <QDateTime>

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
#include "src/models/Alert.h"
#include "src/core/Logger.h"
#include "src/services/ReportService.h"

static constexpr quint16 WS_PORT = 8080;

static QString sharedDbPath()
{
    QString base = QDir::homePath() + "/.local/share/SIEMAgent";
    QDir().mkpath(base);
    return base + "/siemagent.db";
}

static QString resolveCertFile(const QString &fileName)
{
    const QStringList bases = {
        QDir::current().absolutePath(),
        QCoreApplication::applicationDirPath(),
        QDir(QCoreApplication::applicationDirPath()).filePath(".."),
    };
    for (const QString &base : bases) {
        const QString path = QDir(base).filePath(QStringLiteral("certs/") + fileName);
        if (QFile::exists(path)) return path;
    }
    return QDir::current().filePath(QStringLiteral("certs/") + fileName);
}

int main(int argc, char *argv[])
{
    qRegisterMetaType<Event>("Event");
    qRegisterMetaType<Alert>("Alert");

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

    DatabaseService dbService("siem_ui_connection");
    if (!dbService.openWithPath(sharedDbPath())) {
        qDebug() << "DB open error:" << dbService.lastError();
        return -1;
    }

    ReportService reportService(&dbService);

    QString certPath = resolveCertFile(QStringLiteral("server.crt"));
    QString keyPath  = resolveCertFile(QStringLiteral("server.key"));

    QFile configFile(QDir::current().filePath("data/config/app_config.json"));
    if (!configFile.exists())
        configFile.setFileName(resolveCertFile(QStringLiteral("../data/config/app_config.json")));
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

    auto processIncomingEvent = [&](const Event &event) {
        Event ev = event;
        if (ev.id.isEmpty())
            ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!ev.timestamp.isValid())
            ev.timestamp = QDateTime::currentDateTimeUtc();
        if (ev.severity.isEmpty())
            ev.severity = QStringLiteral("low");

        if (!dbService.createEvent(ev)) {
            qWarning() << "[DESKTOP] createEvent failed:" << dbService.lastError();
            return;
        }

        eventListModel.appendNew();
        statsModel.onEventReceived();
        correlationEngine.analyze(ev);

        if (wsService.isRunning() || wsService.isClientConnected())
            wsService.broadcastEvent(ev.toJson());
    };

    QObject::connect(&wsService, &WebSocketService::eventForCorrelation,
                     &app, processIncomingEvent, Qt::QueuedConnection);

    QObject::connect(&wsService, &WebSocketService::eventReceived,
                     &eventListModel, &EventListModel::appendNew);
    QObject::connect(&wsService, &WebSocketService::eventReceived,
                     &statsModel,    &DashboardStatsModel::onEventReceived);
    QObject::connect(&wsService, &WebSocketService::alertReceived,
                     &alertListModel, &AlertListModel::appendNew);
    QObject::connect(&wsService, &WebSocketService::alertReceived,
                     &statsModel,    &DashboardStatsModel::onAlertReceived);

    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &alertListModel,    &AlertListModel::appendNew);
    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &statsModel,        &DashboardStatsModel::onAlertReceived);

    QObject::connect(&statsModel, &DashboardStatsModel::eventsCleared,
                     &eventListModel, &EventListModel::refresh);
    QObject::connect(&statsModel, &DashboardStatsModel::alertsCleared,
                     &alertListModel, &AlertListModel::refresh);

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

    QTimer chartRefreshTimer;
    QObject::connect(&chartRefreshTimer, &QTimer::timeout,
                     &statsModel, &DashboardStatsModel::refreshCharts);
    chartRefreshTimer.start(30'000);

    qmlRegisterSingletonType(
        QUrl("qrc:/qml/styles/Utils.qml"),
        "Utils", 1, 0, "Utils"
    );

    wsService.start(certPath, keyPath);

    QTimer::singleShot(600, &wsService, [&wsService]() {
        if (!wsService.isRunning()) {
            qInfo() << "[WS] Порт агента занят — подключаемся к серверу как клиент (wss://127.0.0.1:"
                    << wsService.uiPort() << ")";
            wsService.connectAsClient(QStringLiteral("127.0.0.1"), wsService.uiPort());
        }
    });

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qDebug() << "QML load error";
        return -1;
    }

    return app.exec();
}
