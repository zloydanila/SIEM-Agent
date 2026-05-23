#ifndef WEBSOCKETWORKER_H
#define WEBSOCKETWORKER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QSslConfiguration>
#include <QJsonObject>
#include <QList>
#include <QHash>
#include <QQueue>
#include <QDateTime>
#include "../models/Event.h"

class DatabaseService;

class WebSocketWorker : public QObject {
    Q_OBJECT

public:
    explicit WebSocketWorker(QObject *parent = nullptr);
    ~WebSocketWorker();

    void setDatabasePath(const QString &dbPath);
    void setSecret(const QString &secret);

    bool isRunning() const;
    int clientCount() const;

public slots:
    bool startServer(quint16 port, bool useWss = false,
                     const QString &certPath = "", const QString &keyPath = "");
    void stopServer();

signals:
    void eventReceived();
    void alertReceived();
    void clientCountChanged();
    void isRunningChanged();
    void serverError(const QString &error);
    void eventForCorrelation(const Event &event);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onClientDisconnected();

private:
    bool validateMessage(const QJsonObject &json);
    bool checkRateLimit(const QString &ip);
    bool verifyHmac(const QJsonObject &json);
    void broadcastJson(const QJsonObject &message, QWebSocket *except = nullptr);
    void processCorrelation(const Event &event);

    QWebSocketServer *m_server = nullptr;
    QList<QWebSocket*> m_clients;
    DatabaseService *m_dbService = nullptr;
    QString m_dbPath;

    bool m_running = false;
    QString m_secret;

    struct NonceEntry { qint64 timestamp; };
    static constexpr qint64 NONCE_TTL_MS = 10 * 60 * 1000;
    static constexpr int NONCE_MAX = 10000;
    static constexpr int NONCE_TRIM_TO = 8000;
    mutable QHash<QString, NonceEntry> m_usedNonces;
    mutable QQueue<QString> m_nonceLRU;

    struct ClientState {
        QQueue<qint64> timestamps;
        qint64 blockedUntil = 0;
    };
    QHash<QString, ClientState> m_clientStates;
};

#endif