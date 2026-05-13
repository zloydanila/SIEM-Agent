#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include "../models/User.h"

class DatabaseService;

class AuthManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(User* currentUser READ getCurrentUser NOTIFY userChanged)

public:
    explicit AuthManager(DatabaseService *databaseService, QObject *parent = nullptr);

    bool isAuthenticated() const;
    User* getCurrentUser() const;

    bool currentUserIsAdmin() const;

public slots:
    void login(const QString &username, const QString &password);
    void logout();
    void registerUser(const QString &username, const QString &password, const QString &role, const QString &fullName, const QString &email);
    void updateUser(const QString &userId, const QString &username, const QString &fullName, const QString &role, const QString &email, bool isActive);
    void deleteUser(const QString &userId);
    Q_INVOKABLE bool changePassword(const QString &currentPassword, const QString &newPassword);
signals:
    void loginSuccess(QString username, QString role, QString fullName, QString email);
    void loginFailed(const QString &reason);
    void loggedOut();
    void authenticatedChanged();
    void userChanged();
    void userRegistered(QString username);
    void registrationFailed(const QString &reason);
    void userUpdated(const QString &userId);
    void updateFailed(const QString &reason);
    void userDeleted(const QString &userId);
    void deletionFailed(const QString &reason);
    void passwordChangeRequired();
    void errorOccured(const QString &message);
    void passwordChanged();

private:
    DatabaseService *m_db;
    bool m_isAuthenticated;
    User *m_currentUser;
};

#endif