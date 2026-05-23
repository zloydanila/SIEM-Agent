#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <QThread>

#include "src/core/Logger.h"
#include "src/services/DatabaseService.h"
#include "src/managers/AuthManager.h"
#include "src/services/HttpServer.h"
#include "src/services/WebSocketService.h"
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

int main(int argc, char *argv[]){
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("SIEMAgent");
    QCoreApplication::setApplicationName("SIEMAgent");

    Logger::instance().init("logs");

    DatabaseService dbService;
    QString dbPath = sharedDbPath();
    qDebug() << "[SERVER] Using DB:" << dbPath;
    if (!dbService.openWithPath(dbPath)) {
        qCritical() << "[SERVER] Failed to open DB:" << dbService.lastError();
        return 1;
    }

    // === Менеджеры и модели ===
    AuthManager authManager(&dbService);

    EventListModel  eventListModel(&dbService);
    AlertListModel  alertListModel(&dbService);
    DashboardStatsModel dashboardStats(&dbService);
    UserListModel   userListModel(&dbService);
    RuleListModel   ruleListModel(&dbService);
    ReportService   reportService(&dbService);

    // === HTTP ===
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

    if (!httpServer.start(8443)) {
        qCritical() << "[SERVER] Failed to start HTTP server";
        return 1;
    }

    // === WebSocket Worker + CorrelationEngine в ОДНОМ потоке ===
    QThread wsThread;
    WebSocketWorker *wsWorker = new WebSocketWorker();
    wsWorker->setDatabasePath(dbService.dbPath());
    wsWorker->setSecret(qgetenv("WS_SECRET"));
    wsWorker->moveToThread(&wsThread);

    // === ИЗМЕНЕНО: CorrelationEngine создаёт свою БД в потоке воркера ===
    CorrelationEngine *correlationEngine = nullptr;
    QString dbPathForEngine = dbService.dbPath();  // Сохраняем путь до запуска потока
    
    QObject::connect(&wsThread, &QThread::started, [&, dbPathForEngine]() {
        // Создаём движок БЕЗ БД (nullptr)
        correlationEngine = new CorrelationEngine(nullptr);
        
        // === НОВОЕ: инициализируем БД в текущем потоке ===
        correlationEngine->initializeDatabase(dbPathForEngine);
        
        wsWorker->setCorrelationEngine(correlationEngine);

        // Подключаем сигналы через QueuedConnection (между потоками)
        QObject::connect(correlationEngine, &CorrelationEngine::alertCreated,
                         &alertListModel, &AlertListModel::appendNew,
                         Qt::QueuedConnection);
        QObject::connect(correlationEngine, &CorrelationEngine::alertCreated,
                         &dashboardStats, &DashboardStatsModel::refresh,
                         Qt::QueuedConnection);

        wsWorker->startServer(8080, true,
            QDir::current().filePath("certs/server.crt"),
            QDir::current().filePath("certs/server.key"));
    });

    QObject::connect(&wsThread, &QThread::finished, wsWorker, &WebSocketWorker::stopServer);
    QObject::connect(&wsThread, &QThread::finished, wsWorker, &QObject::deleteLater);
    if (correlationEngine) {
        QObject::connect(&wsThread, &QThread::finished, correlationEngine, &QObject::deleteLater);
    }

    // Приём события — обновляем списки (уже QueuedConnection через moveToThread)
    QObject::connect(wsWorker, &WebSocketWorker::eventReceived,
                     &eventListModel, &EventListModel::appendNew);
    QObject::connect(wsWorker, &WebSocketWorker::eventReceived,
                     &dashboardStats, &DashboardStatsModel::refresh);

    wsThread.start();

    // === Глобальная очистка истории каждые 10 минут ===
    QTimer cleanupTimer;
    // ИЗМЕНЕНО: cleanup через invokeMethod, потому что correlationEngine в другом потоке
    QObject::connect(&cleanupTimer, &QTimer::timeout, [&]() {
        if (correlationEngine) {
            QMetaObject::invokeMethod(correlationEngine, "globalCleanup", Qt::QueuedConnection);
        }
    });
    cleanupTimer.start(10 * 60 * 1000);

    qInfo() << "[SERVER] ready";
    return app.exec();
}