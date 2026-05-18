#include "WebSocketWorker.h"
#include "DatabaseService.h"
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

WebSocketWorker::WebSocketWorker(QObject *parent)
    : QObject(parent)
{}

WebSocketWorker::~WebSocketWorker() {
    stopServer();
}

void WebSocketWorker::setDatabasePath(const QString &dbPath) {
    m_dbPath = dbPath;
}

void WebSocketWorker::setSecret(const QString &secret) {
    m_secret = secret;
}

bool WebSocketWorker::isRunning() const {
    return m_running;
}

int WebSocketWorker::clientCount() const {
    return m_clients.size();
}

bool WebSocketWorker::startServer(quint16 port, bool useWss,
                                   const QString &certPath, const QString &keyPath)
{
    stopServer();

    if (!m_dbService && !m_dbPath.isEmpty()) {
        m_dbService = new DatabaseService("ws_worker_connection", this);
        if (!m_dbService->openWithPath(m_dbPath)) {
            qWarning() << "[WSS] Не удалось открыть БД в потоке воркера";
            delete m_dbService;
            m_dbService = nullptr;
        } else {
            qDebug() << "[WSS] БД открыта в потоке воркера";
        }
    }

    if (useWss) {
        m_server = new QWebSocketServer("SIEM Agent WSS",
                                        QWebSocketServer::SecureMode, this);
        QFile certFile(certPath);
        QFile keyFile(keyPath);

        if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly)) {
            QSslConfiguration sslConfig;
            sslConfig.setLocalCertificate(QSslCertificate(&certFile, QSsl::Pem));
            sslConfig.setPrivateKey(QSslKey(&keyFile, QSsl::Rsa, QSsl::Pem));
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            m_server->setSslConfiguration(sslConfig);
        } else {
            emit serverError("Ошибка загрузки SSL сертификатов: " + certPath);
            delete m_server;
            m_server = nullptr;
            return false;
        }
    } else {
        m_server = new QWebSocketServer("SIEM Agent WS",
                                        QWebSocketServer::NonSecureMode, this);
    }

    if (m_server->listen(QHostAddress::Any, port)) {
        connect(m_server, &QWebSocketServer::newConnection,
                this, &WebSocketWorker::onNewConnection);
        m_running = true;
        emit isRunningChanged();
        qDebug() << "[WSS] Сервер запущен на порту" << port
                 << (useWss ? "(WSS)" : "(WS)");
        return true;
    }

    emit serverError(m_server->errorString());
    delete m_server;
    m_server  = nullptr;
    m_running = false;
    emit isRunningChanged();
    return false;
}

void WebSocketWorker::stopServer() {
    if (m_server) {
        for (QWebSocket *s : m_clients) {
            s->close();
            s->deleteLater();
        }
        m_clients.clear();
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    if (m_running) {
        m_running = false;
        emit isRunningChanged();
        emit clientCountChanged();
    }
}

void WebSocketWorker::onNewConnection() {
    QWebSocket *socket = m_server->nextPendingConnection();
    connect(socket, &QWebSocket::textMessageReceived,
            this,   &WebSocketWorker::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected,
            this,   &WebSocketWorker::onClientDisconnected);
    m_clients << socket;
    emit clientCountChanged();
}

bool WebSocketWorker::checkRateLimit(const QString &ip) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    ClientState &state = m_clientStates[ip];

    if (state.blockedUntil > now) return false;
    if (state.blockedUntil > 0) {
        state.blockedUntil = 0;
        state.timestamps.clear();
    }

    while (!state.timestamps.isEmpty() &&
           now - state.timestamps.first() > 1000) {
        state.timestamps.dequeue();
    }

    if (state.timestamps.size() >= 50) {
        state.blockedUntil = now + 60000;
        qWarning() << "[WSS] RATE LIMIT: IP" << ip << "заблокирован на 60с";

        if (m_dbService) {
            Event floodEvent;
            floodEvent.id         = QUuid::createUuid().toString(QUuid::WithoutBraces);
            floodEvent.deviceName = "SIEM Agent";
            floodEvent.eventType  = "ddos_detected";
            floodEvent.action     = "block_ip";
            floodEvent.severity   = "critical";
            floodEvent.timestamp  = QDateTime::currentDateTime();
            floodEvent.rawLog     = QString("Превышен лимит (>50/сек). IP: %1").arg(ip);
            floodEvent.location   = ip;

            if (m_dbService->createEvent(floodEvent)) {
                emit eventReceived();
                emit eventForCorrelation(floodEvent);
            }
        }
        return false;
    }

    state.timestamps.enqueue(now);
    return true;
}

void WebSocketWorker::onTextMessageReceived(const QString &message) {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    QString ip = client->peerAddress().toString();
    if (!checkRateLimit(ip)) return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject json = doc.object();
    if (!validateMessage(json)) return;

    Event event = Event::fromJson(json);
    if (event.id.isEmpty())
        event.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    event.severity = json["severity"].toString().toLower();

    if (m_dbService && m_dbService->createEvent(event)) {
        emit eventReceived();
        emit eventForCorrelation(event);
    }
}

void WebSocketWorker::onClientDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
        emit clientCountChanged();
    }
}

bool WebSocketWorker::validateMessage(const QJsonObject &json) {
    if (!verifyHmac(json)) return false;
    for (const QString &field : {"deviceName", "eventType", "action"}) {
        if (!json.contains(field)) return false;
    }
    return true;
}

bool WebSocketWorker::verifyHmac(const QJsonObject &json) {
    if (m_secret.isEmpty()) return true;

    const QString nonce     = json["nonce"].toString();
    const QString signature = json["signature"].toString();

    if (nonce.isEmpty() || signature.isEmpty()) {
        qWarning() << "[WSS] Отклонено: отсутствует nonce или signature";
        return false;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_usedNonces.contains(nonce)) {
        if (now - m_usedNonces[nonce].timestamp <= NONCE_TTL_MS) {
            qWarning() << "[WSS] Отклонено: replay attack (nonce повторный)";
            return false;
        }
        m_usedNonces.remove(nonce);
    }

    QString payload = json["deviceName"].toString()
                    + json["eventType"].toString()
                    + nonce
                    + m_secret;

    QByteArray expected = QMessageAuthenticationCode::hash(
        payload.toUtf8(),
        m_secret.toUtf8(),
        QCryptographicHash::Sha256
    ).toHex();

    if (expected != signature.toUtf8()) {
        qWarning() << "[WSS] Отклонено -неверная HMAC-подпись";
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