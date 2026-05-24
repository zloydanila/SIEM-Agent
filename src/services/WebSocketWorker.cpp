#include "WebSocketWorker.h"
#include "../models/Event.h"

#include <QJsonDocument>
#include <QUuid>
#include <QDebug>
#include <QFile>
#include <QHostAddress>
#include <QDateTime>
#include <QSslKey>
#include <QSslCertificate>
#include <QMessageAuthenticationCode>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QUrl>

WebSocketWorker::WebSocketWorker(QObject *parent) : QObject(parent) {}

WebSocketWorker::~WebSocketWorker() {
    stopServer();
}

void WebSocketWorker::setSecret(const QString &secret) {
    m_secret = secret;
}

bool WebSocketWorker::isRunning() const {
    return m_running || m_clientConnected;
}

bool WebSocketWorker::isClientConnected() const {
    return m_clientConnected;
}

int WebSocketWorker::clientCount() const {
    return m_uiClients.size();
}

quint16 WebSocketWorker::uiPort() const {
    return kDefaultUiPort;
}

void WebSocketWorker::updateRunningState() {
    const bool newRunning =
        (m_server && m_server->isListening()) ||
        (m_uiServer && m_uiServer->isListening()) ||
        m_clientConnected;

    if (m_running != newRunning) {
        m_running = newRunning;
        emit isRunningChanged();
    }
}

void WebSocketWorker::cleanupSockets(QList<QWebSocket*> &list) {
    while (!list.isEmpty()) {
        QWebSocket *socket = list.takeFirst();
        if (!socket) continue;
        socket->disconnect(this);
        socket->close();
        socket->deleteLater();
    }
}

bool WebSocketWorker::startServer(quint16 port, bool useWss, const QString &certPath, const QString &keyPath) {
    stopServer();

    if (useWss) {
        m_server = new QWebSocketServer(QStringLiteral("SIEM Agent WSS"), QWebSocketServer::SecureMode, this);

        QFile certFile(certPath);
        QFile keyFile(keyPath);
        if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
            emit serverError(QStringLiteral("Ошибка загрузки SSL сертификатов: %1").arg(certPath));
            delete m_server;
            m_server = nullptr;
            updateRunningState();
            return false;
        }

        QSslConfiguration sslConfig;
        sslConfig.setLocalCertificate(QSslCertificate(&certFile, QSsl::Pem));
        sslConfig.setPrivateKey(QSslKey(&keyFile, QSsl::Rsa, QSsl::Pem));
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        m_server->setSslConfiguration(sslConfig);
    } else {
        m_server = new QWebSocketServer(QStringLiteral("SIEM Agent WS"), QWebSocketServer::NonSecureMode, this);
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit serverError(m_server->errorString());
        delete m_server;
        m_server = nullptr;
        updateRunningState();
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketWorker::onNewAgentConnection);
    qDebug() << "[WSS] Agent server started on port" << port << (useWss ? "(WSS)" : "(WS)");

    m_uiServer = new QWebSocketServer(QStringLiteral("SIEM Agent UI WS"), QWebSocketServer::NonSecureMode, this);
    if (m_uiServer->listen(QHostAddress::Any, kDefaultUiPort)) {
        connect(m_uiServer, &QWebSocketServer::newConnection, this, &WebSocketWorker::onNewUiConnection);
        qDebug() << "[WS] UI WebSocket started on port" << kDefaultUiPort;
    } else {
        qWarning() << "[WS] Failed to start UI WebSocket on port" << kDefaultUiPort
                   << ":" << m_uiServer->errorString();
        m_uiServer->deleteLater();
        m_uiServer = nullptr;
    }

    updateRunningState();
    emit clientCountChanged();
    return true;
}

void WebSocketWorker::stopServer() {
    disconnectClient();

    if (m_uiServer) {
        cleanupSockets(m_uiClients);
        m_uiServer->close();
        m_uiServer->deleteLater();
        m_uiServer = nullptr;
    } else {
        cleanupSockets(m_uiClients);
    }

    if (m_server) {
        cleanupSockets(m_agentClients);
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    } else {
        cleanupSockets(m_agentClients);
    }

    m_lastAgentSocket = nullptr;
    emit clientCountChanged();
    updateRunningState();
}

void WebSocketWorker::onNewAgentConnection() {
    if (!m_server) return;

    QWebSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketWorker::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketWorker::onAgentDisconnected);

    m_agentClients << socket;
    updateRunningState();

    qDebug() << "[WSS] Agent connected:" << socket->peerAddress().toString()
             << "total agents:" << m_agentClients.size();
}

void WebSocketWorker::onNewUiConnection() {
    if (!m_uiServer) return;

    QWebSocket *socket = m_uiServer->nextPendingConnection();
    if (!socket) return;

    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketWorker::handleUiMessage);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketWorker::onUiDisconnected);

    m_uiClients << socket;
    emit clientCountChanged();
    updateRunningState();

    qDebug() << "[WS] UI subscriber connected, total UI clients:" << m_uiClients.size();
}

void WebSocketWorker::onAgentDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    m_agentClients.removeAll(client);
    if (m_lastAgentSocket == client) m_lastAgentSocket = nullptr;

    client->deleteLater();
    updateRunningState();

    qDebug() << "[WSS] Agent disconnected, total agents:" << m_agentClients.size();
}

void WebSocketWorker::onUiDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    m_uiClients.removeAll(client);
    client->deleteLater();

    emit clientCountChanged();
    updateRunningState();

    qDebug() << "[WS] UI subscriber disconnected, total UI clients:" << m_uiClients.size();
}

bool WebSocketWorker::checkRateLimit(const QString &ip) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    ClientState &state = m_clientStates[ip];

    if (state.blockedUntil > now) return false;

    if (state.blockedUntil > 0) {
        state.blockedUntil = 0;
        state.timestamps.clear();
    }

    while (!state.timestamps.isEmpty() && now - state.timestamps.first() > 1000) {
        state.timestamps.dequeue();
    }

    if (state.timestamps.size() >= 50) {
        state.blockedUntil = now + 60000;
        qWarning() << "[WSS] RATE LIMIT: IP" << ip << "blocked for 60s";
        return false;
    }

    state.timestamps.enqueue(now);
    return true;
}

void WebSocketWorker::onTextMessageReceived(const QString &message) {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    const QString ip = client->peerAddress().toString();
    if (!checkRateLimit(ip)) return;

    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    const QJsonObject json = doc.object();
    if (!validateMessage(json)) return;

    Event event = Event::fromJson(json);
    if (event.id.isEmpty())
        event.id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    event.severity = json.value("severity").toString().toLower();
    event.timestamp = QDateTime::currentDateTimeUtc();

    m_lastAgentSocket = client;
    emit eventForCorrelation(event);
}

bool WebSocketWorker::validateMessage(const QJsonObject &json) {
    if (!verifyHmac(json)) return false;

    for (const QString &field : {QStringLiteral("deviceName"), QStringLiteral("eventType"), QStringLiteral("action")}) {
        if (!json.contains(field)) return false;
    }

    return true;
}

bool WebSocketWorker::verifyHmac(const QJsonObject &json) {
    if (m_secret.isEmpty()) return true;

    const QString nonce = json.value(QStringLiteral("nonce")).toString();
    const QString signature = json.value(QStringLiteral("signature")).toString();

    if (nonce.isEmpty() || signature.isEmpty()) {
        qWarning() << "[WSS] Rejected: missing nonce or signature";
        return false;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_usedNonces.contains(nonce)) {
        if (now - m_usedNonces[nonce].timestamp <= NONCE_TTL_MS) {
            qWarning() << "[WSS] Rejected: replay attack";
            return false;
        }
        m_usedNonces.remove(nonce);
    }

    const QString payload =
        json.value(QStringLiteral("deviceName")).toString() +
        json.value(QStringLiteral("eventType")).toString() +
        nonce +
        m_secret;

    const QByteArray expected = QMessageAuthenticationCode::hash(
        payload.toUtf8(),
        m_secret.toUtf8(),
        QCryptographicHash::Sha256
    ).toHex().toLower();

    const QByteArray received = signature.toUtf8().toLower();

    if (expected != received) {
        qWarning() << "[WSS] Rejected: invalid HMAC signature";
        return false;
    }

    m_usedNonces[nonce] = { now };
    m_nonceLRU.enqueue(nonce);

    if (m_usedNonces.size() > NONCE_MAX) {
        while (!m_nonceLRU.isEmpty() && m_usedNonces.size() > NONCE_TRIM_TO) {
            m_usedNonces.remove(m_nonceLRU.dequeue());
        }
    }

    return true;
}

void WebSocketWorker::broadcastJsonToUi(const QJsonObject &message) {
    if (m_uiClients.isEmpty()) return;

    const QString text = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));

    for (QWebSocket *socket : std::as_const(m_uiClients)) {
        if (!socket) continue;
        if (socket->state() == QAbstractSocket::ConnectedState)
            socket->sendTextMessage(text);
    }
}

void WebSocketWorker::broadcastAlertJson(const QJsonObject &alertJson) {
    broadcastJsonToUi(QJsonObject{
        { QStringLiteral("type"), QStringLiteral("alert") },
        { QStringLiteral("payload"), alertJson }
    });
    emit alertReceived();
}

void WebSocketWorker::broadcastEventJson(const QJsonObject &eventJson) {
    Q_UNUSED(m_lastAgentSocket);

    broadcastJsonToUi(QJsonObject{
        { QStringLiteral("type"), QStringLiteral("event") },
        { QStringLiteral("payload"), eventJson }
    });

    m_lastAgentSocket = nullptr;
    emit eventReceived();
}

void WebSocketWorker::connectAsClient(const QString &host, quint16 port, bool secure) {
    if (m_clientSocket) disconnectClient();

    m_clientSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(m_clientSocket, &QWebSocket::connected, this, [this, host, port, secure]() {
        m_clientConnected = true;
        updateRunningState();
        qDebug() << "[WS] Desktop client connected to" << (secure ? "wss://" : "ws://") << host << ":" << port;
    });

    connect(m_clientSocket, &QWebSocket::disconnected, this, [this]() {
        if (m_clientConnected) {
            m_clientConnected = false;
            updateRunningState();
            qDebug() << "[WS] Desktop client disconnected from server";
        }
    });

    connect(m_clientSocket, &QWebSocket::textMessageReceived,
            this, &WebSocketWorker::onClientSocketMessage);

    const QString scheme = secure ? QStringLiteral("wss") : QStringLiteral("ws");
    const QUrl url(QStringLiteral("%1://%2:%3").arg(scheme, host).arg(port));
    m_clientSocket->open(url);
}

void WebSocketWorker::disconnectClient() {
    if (!m_clientSocket) return;

    m_clientSocket->disconnect(this);
    m_clientSocket->close();
    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;

    if (m_clientConnected) {
        m_clientConnected = false;
        updateRunningState();
    }
}

void WebSocketWorker::onClientSocketMessage(const QString &message) {
    handleUiMessage(message);
}

void WebSocketWorker::handleUiMessage(const QString &message) {
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    const QJsonObject root = doc.object();
    const QString type = root.value(QStringLiteral("type")).toString().toLower();

    if (type == QLatin1String("event")) {
        emit eventReceived();
        return;
    }

    if (type == QLatin1String("alert")) {
        emit alertReceived();
    }
}