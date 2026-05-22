#include "User.h"
#include <QCryptographicHash>
#include <QPasswordDigestor>

static constexpr int PBKDF2_ITERATIONS = 100000;
static constexpr int PBKDF2_DKLEN = 32;

void User::setPassword(const QString &password, const QString &newSalt)
{
    salt = newSalt;
    const QByteArray key = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,
        password.toUtf8(),
        salt.toUtf8(),
        PBKDF2_ITERATIONS,
        PBKDF2_DKLEN
    );
    passwordHash = key.toHex();
}

bool User::checkPassword(const QString &password) const
{
    if (salt.isEmpty() || passwordHash.isEmpty()) return false;

    const QByteArray key = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,
        password.toUtf8(),
        salt.toUtf8(),
        PBKDF2_ITERATIONS,
        PBKDF2_DKLEN
    );
    return key.toHex() == passwordHash.toUtf8();
}

QJsonObject User::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["username"] = username;
    obj["role"] = role;
    obj["fullName"] = fullName;
    obj["email"] = email;
    obj["isActive"] = isActive;
    obj["mustChangePassword"] = mustChangePassword;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    return obj;
}