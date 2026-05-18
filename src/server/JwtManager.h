#ifndef JWT_MANAGER_H
#define JWT_MANAGER_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageAuthenticationCode>
#include <QCryptographicHash>
#include <QDateTime>

class JwtManager {
public:
    JwtManager(const QString &secret) : m_secret(secret) {}

    QString generateToken(const QJsonObject &payload, int expireSeconds = 86400) {
        QJsonObject fullPayload = payload;
        fullPayload["iat"] = QDateTime::currentSecsSinceEpoch();
        fullPayload["exp"] = QDateTime::currentSecsSinceEpoch() + expireSeconds;
        QString data = QString::fromUtf8(QJsonDocument(fullPayload).toJson(QJsonDocument::Compact));
        QString signature = hmacSha256(data, m_secret);
        return data + "." + signature;
    }

    bool verifyToken(const QString &token, QJsonObject &payload) {
        int dot = token.lastIndexOf('.');
        if (dot < 0) return false;
        QString data = token.left(dot);
        QString signature = token.mid(dot + 1);
        if (hmacSha256(data, m_secret) != signature) return false;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if (!doc.isObject()) return false;
        payload = doc.object();
        qint64 exp = payload["exp"].toInteger();
        return (exp > QDateTime::currentSecsSinceEpoch());
    }

private:
    QString m_secret;
    QString hmacSha256(const QString &data, const QString &key) {
        return QMessageAuthenticationCode::hash(
            data.toUtf8(), key.toUtf8(), QCryptographicHash::Sha256).toHex();
    }
};

#endif