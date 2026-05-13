#include "User.h"
#include <QPasswordDigestor>
#include <QCryptographicHash>

User::User()
    : isActive(true), mustChangePassword(false){
}


void User::setPassword(const QString &password, const QString &salt){
    QByteArray hash = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,password.toUtf8(), salt.toUtf8(),100000,  32
    );
    passwordHash = hash.toHex();
    this->salt = salt;
}

bool User::checkPassword(const QString &password) const {
    if (passwordHash.isEmpty() || salt.isEmpty()) return false;

    QByteArray hash = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,
        password.toUtf8(),
        salt.toUtf8(),
        100000,
        32
    );
    return hash.toHex() == passwordHash;
}

QJsonObject User::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["username"] = username;
    obj["passwordHash"] = passwordHash;
    obj["salt"] = salt;
    obj["role"] = role;
    obj["fullName"] = fullName;
    obj["email"] = email;
    obj["isActive"] = isActive;
    obj["mustChangePassword"] = mustChangePassword;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    return obj;
}

User User::fromJson(const QJsonObject &json) {
    User user;
    user.id = json["id"].toString();
    user.username = json["username"].toString();
    user.passwordHash = json["passwordHash"].toString();
    user.salt = json["salt"].toString();
    user.role = json["role"].toString("viewer");
    user.fullName = json["fullName"].toString();
    user.email = json["email"].toString();
    user.isActive = json["isActive"].toBool(true);
    user.mustChangePassword = json["mustChangePassword"].toBool(false);
    user.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    if (!user.createdAt.isValid()) {
        user.createdAt = QDateTime::currentDateTime();
    }
    return user;
}