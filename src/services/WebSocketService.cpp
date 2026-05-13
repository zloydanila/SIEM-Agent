#include "WebSocketService.h"
#include "WebSocketWorker.h"
#include "DatabaseService.h"
#include <QDebug>

WebSocketService::WebSocketService(quint16 port, QObject *parent)
    : QObject(parent), m_port(port)
{
    m_thread = new QThread(this);
    m_worker = new WebSocketWorker();
    m_worker->moveToThread(m_thread);

    connect(this,     &WebSocketService::startServerRequested,
            m_worker, &WebSocketWorker::startServer);
    connect(this,     &WebSocketService::stopServerRequested,
            m_worker, &WebSocketWorker::stopServer);

    connect(m_worker, &WebSocketWorker::eventReceived,
            this,     &WebSocketService::eventReceived);
    connect(m_worker, &WebSocketWorker::alertReceived,
            this,     &WebSocketService::alertReceived);
    connect(m_worker, &WebSocketWorker::eventForCorrelation,
            this,     &WebSocketService::eventForCorrelation);
    connect(m_worker, &WebSocketWorker::isRunningChanged,
            this,     &WebSocketService::isRunningChanged);
    connect(m_worker, &WebSocketWorker::clientCountChanged,
            this,     &WebSocketService::clientCountChanged);

    connect(m_thread, &QThread::finished,
            m_worker, &QObject::deleteLater);

    m_thread->start();
}

WebSocketService::~WebSocketService() {
    emit stopServerRequested();
    m_thread->quit();
    m_thread->wait(3000);
}

bool WebSocketService::isRunning() const {
    return m_worker && m_worker->isRunning();
}

int WebSocketService::clientCount() const {
    return m_worker ? m_worker->clientCount() : 0;
}

quint16 WebSocketService::port() const {
    return m_port;
}

void WebSocketService::setSharedSecret(const QString &secret) {
    QMetaObject::invokeMethod(m_worker, [this, secret]() {
        m_worker->setSecret(secret);
    }, Qt::QueuedConnection);
}

void WebSocketService::setDatabaseService(DatabaseService *dbService) {
    QString path = dbService->dbPath();
    QMetaObject::invokeMethod(m_worker, [this, path]() {
        m_worker->setDatabasePath(path);
    }, Qt::QueuedConnection);
}

void WebSocketService::start(const QString &certPath, const QString &keyPath) {
    bool useWss = !certPath.isEmpty() && !keyPath.isEmpty();
    emit startServerRequested(m_port, useWss, certPath, keyPath);
}