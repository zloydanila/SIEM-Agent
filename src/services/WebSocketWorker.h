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

class WebSocketWorker : public QObject {
    Q_OBJECT

public:
    static constexpr quint16 kDefaultUiPort = 8081;

    explicit WebSocketWorker(QObject *parent = nullptr);
    ~WebSocketWorker();

    void setSecret(const QString &secret);

    Q_INVOKABLE bool isRunning() const;
    Q_INVOKABLE bool isClientConnected() const;
    Q_INVOKABLE int clientCount() const;
    quint16 uiPort() const;

public slots:
    void connectAsClient(const QString &host, quint16 port, bool secure = false);
    void disconnectClient();
    void broadcastAlertJson(const QJsonObject &alertJson);
    void broadcastEventJson(const QJsonObject &eventJson);
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
    void onNewAgentConnection();
    void onNewUiConnection();
    void onTextMessageReceived(const QString &message);
    void onAgentDisconnected();
    void onUiDisconnected();

private:
    bool validateMessage(const QJsonObject &json);
    bool checkRateLimit(const QString &ip);
    bool verifyHmac(const QJsonObject &json);

    void broadcastJsonToUi(const QJsonObject &message);
    void updateRunningState();
    void cleanupSockets(QList<QWebSocket*> &list);

    void handleUiMessage(const QString &message);
    void onClientSocketMessage(const QString &message);

    QWebSocketServer *m_server = nullptr;
    QWebSocketServer *m_uiServer = nullptr;

    QList<QWebSocket*> m_agentClients;
    QList<QWebSocket*> m_uiClients;

    QWebSocket *m_clientSocket = nullptr;
    bool m_clientConnected = false;
    bool m_running = false;
    QString m_secret;

    QWebSocket *m_lastAgentSocket = nullptr;

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