#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QDateTime>
#include <QUuid>

#include "src/core/Logger.h"
#include "src/models/Alert.h"
#include "src/models/Event.h"
#include "src/services/DatabaseService.h"
#include "src/managers/AuthManager.h"
#include "src/services/HttpServer.h"
#include "src/services/WebSocketWorker.h"
#include "src/services/ReportService.h"
#include "src/models/EventListModel.h"
#include "src/models/AlertListModel.h"
#include "src/models/DashboardStatsModel.h"
#include "src/models/UserListModel.h"
#include "src/models/RuleListModel.h"
#include "src/engine/CorrelationEngine.h"

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

int main(int argc, char *argv[]){
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("SIEMAgent");
    QCoreApplication::setApplicationName("SIEMAgent");

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
            qputenv(line.left(eq).trimmed().toUtf8(), line.mid(eq + 1).trimmed().toUtf8());
        }
        envFile.close();
        qInfo() << "[CONFIG] .env файл загружен";
    }

    DatabaseService dbService;
    QString dbPath = sharedDbPath();
    qDebug() << "[SERVER] Using DB:" << dbPath;
    if (!dbService.openWithPath(dbPath)) {
        qCritical() << "[SERVER] Failed to open DB:" << dbService.lastError();
        return 1;
    }

    AuthManager authManager(&dbService);

    EventListModel  eventListModel(&dbService);
    AlertListModel  alertListModel(&dbService);
    DashboardStatsModel dashboardStats(&dbService);
    UserListModel   userListModel(&dbService);
    RuleListModel   ruleListModel(&dbService);
    ReportService   reportService(&dbService);
    CorrelationEngine correlationEngine(&dbService);

    HttpServer httpServer;
    httpServer.setDatabaseService(&dbService);
    httpServer.setAuthManager(&authManager);
    httpServer.setEventListModel(&eventListModel);
    httpServer.setAlertListModel(&alertListModel);
    httpServer.setDashboardStatsModel(&dashboardStats);
    httpServer.setUserListModel(&userListModel);
    httpServer.setRuleListModel(&ruleListModel);
    httpServer.setReportService(&reportService);
    httpServer.setWebRoot(QDir::current().filePath("web"));
    httpServer.setAllowedOrigin("*");

    quint16 httpPort = 8443;
    const QByteArray envHttpPort = qgetenv("SIEM_HTTP_PORT");
    if (!envHttpPort.isEmpty()) httpPort = envHttpPort.toUShort();

    if (!httpServer.startWithFallback(httpPort, &httpPort)) {
        return 1;
    }

    QThread wsThread;
    WebSocketWorker *wsWorker = new WebSocketWorker();
    wsWorker->setSecret(qEnvironmentVariable("SIEM_WS_SECRET"));
    wsWorker->moveToThread(&wsThread);
    httpServer.setWebSocketWorker(wsWorker);

    auto processIncomingEvent = [&](const Event &event) {
        Event ev = event;
        if (ev.id.isEmpty())
            ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!ev.timestamp.isValid())
            ev.timestamp = QDateTime::currentDateTimeUtc();
        if (ev.severity.isEmpty())
            ev.severity = QStringLiteral("low");

        if (!dbService.createEvent(ev)) {
            qWarning() << "[SERVER] createEvent failed:" << dbService.lastError();
            return;
        }

        eventListModel.appendNew();
        dashboardStats.onEventReceived();
        correlationEngine.analyze(ev);

        const QJsonObject json = ev.toJson();
        QMetaObject::invokeMethod(wsWorker, "broadcastEventJson", Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, json));
    };

    QObject::connect(wsWorker, &WebSocketWorker::eventForCorrelation,
                     &app, processIncomingEvent, Qt::QueuedConnection);

    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &alertListModel, &AlertListModel::appendNew);
    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     &dashboardStats, &DashboardStatsModel::onAlertReceived);
    QObject::connect(&correlationEngine, &CorrelationEngine::alertCreated,
                     wsWorker, [wsWorker](const Alert &alert) {
                         const QJsonObject json = alert.toJson();
                         QMetaObject::invokeMethod(wsWorker, "broadcastAlertJson",
                                                   Qt::QueuedConnection,
                                                   Q_ARG(QJsonObject, json));
                     });

    QObject::connect(wsWorker, &WebSocketWorker::eventReceived,
                     &eventListModel, &EventListModel::appendNew);
    QObject::connect(wsWorker, &WebSocketWorker::eventReceived,
                     &dashboardStats, &DashboardStatsModel::onEventReceived);
    QObject::connect(wsWorker, &WebSocketWorker::alertReceived,
                     &alertListModel, &AlertListModel::appendNew);
    QObject::connect(wsWorker, &WebSocketWorker::alertReceived,
                     &dashboardStats, &DashboardStatsModel::onAlertReceived);

    const QString certPath = resolveCertFile(QStringLiteral("server.crt"));
    const QString keyPath  = resolveCertFile(QStringLiteral("server.key"));

    QObject::connect(&wsThread, &QThread::started, wsWorker, [wsWorker, certPath, keyPath]() {
        if (!wsWorker->startServer(8080, true, certPath, keyPath)) {
            qCritical() << "[SERVER] Failed to start WebSocket on 8080";
        }
    });

    QObject::connect(&wsThread, &QThread::finished, wsWorker, &WebSocketWorker::stopServer);
    QObject::connect(&wsThread, &QThread::finished, wsWorker, &QObject::deleteLater);

    wsThread.start();

    QTimer chartRefreshTimer;
    QObject::connect(&chartRefreshTimer, &QTimer::timeout,
                     &dashboardStats, &DashboardStatsModel::refreshCharts);
    chartRefreshTimer.start(30'000);

    QTimer cleanupTimer;
    QObject::connect(&cleanupTimer, &QTimer::timeout,
                     &correlationEngine, &CorrelationEngine::globalCleanup);
    cleanupTimer.start(10 * 60 * 1000);

    qInfo() << "[SERVER] ready — HTTP:" << httpPort << "WS:8080 UI-WS:8081";
    return app.exec();
}
