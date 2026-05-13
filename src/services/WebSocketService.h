#ifndef WEBSOCKETSERVICE_H
#define WEBSOCKETSERVICE_H

#include <QObject>
#include <QString>
#include <QThread>
#include "../models/Event.h"

class WebSocketWorker;
class DatabaseService;
class CorrelationEngine;

class WebSocketService : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int clientCount READ clientCount NOTIFY clientCountChanged)
    Q_PROPERTY(int port READ port CONSTANT)

public:
    explicit WebSocketService(quint16 port, QObject *parent = nullptr);
    ~WebSocketService();

    bool isRunning() const;
    int clientCount() const;
    quint16 port() const;

    void setSharedSecret(const QString &secret);
    void setDatabaseService(DatabaseService *dbService);

    void start(const QString &certPath = "", const QString &keyPath = "");

signals:
    void startServerRequested(quint16 port, bool useWss,
                              const QString &certPath, const QString &keyPath);
    void stopServerRequested();

    void eventReceived();
    void alertReceived();
    void eventForCorrelation(const Event &event);
    void isRunningChanged();
    void clientCountChanged();

private:
    WebSocketWorker *m_worker = nullptr;
    QThread *m_thread = nullptr;
    quint16 m_port = 0;
};

#endif