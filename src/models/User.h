#ifndef USER_H
#define USER_H

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QCryptographicHash>

class User {
public:
    User();

    QString id;
    QString username;
    QString passwordHash;
    QString salt;
    QString role;
    QString fullName;
    QString email;
    bool isActive;
    bool mustChangePassword;
    QDateTime createdAt;

    void setPassword(const QString &password, const QString &salt);
    bool checkPassword(const QString &password) const;

    QJsonObject toJson() const;
    static User fromJson(const QJsonObject &json);


};

#endif