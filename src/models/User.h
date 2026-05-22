#ifndef USER_H
#define USER_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

class DatabaseService;

class User {
public:
    QString id;
    QString username;
    QString passwordHash;
    QString salt;
    QString role;
    QString fullName;
    QString email;
    bool isActive = true;
    bool mustChangePassword = true;
    QDateTime createdAt;

    User() = default;

    void setPassword(const QString &password, const QString &newSalt);
    bool checkPassword(const QString &password) const;
    QJsonObject toJson() const;
};

#endif