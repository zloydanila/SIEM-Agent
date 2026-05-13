#include "AuthManager.h"
#include "../services/DatabaseService.h"
#include <QUuid>
#include <QDateTime>

AuthManager::AuthManager(DatabaseService *databaseService, QObject *parent)
    : QObject(parent),
      m_db(databaseService),
      m_isAuthenticated(false),
      m_currentUser(nullptr) {
}

bool AuthManager::isAuthenticated() const {
    return m_isAuthenticated;
}

User* AuthManager::getCurrentUser() const {
    return m_currentUser;
}

bool AuthManager::currentUserIsAdmin() const {
    return m_currentUser && m_currentUser->role == "admin";
}

void AuthManager::login(const QString &username, const QString &password) {
    if (!m_db) {
        emit loginFailed("Database is not initialized");
        return;
    }

    User user = m_db->findUserByUsername(username);

    if (user.username.isEmpty()) {
        emit loginFailed("Пользователь не найден");
        return;
    }

    if (!user.isActive) {
        emit loginFailed("Пользователь отключён");
        return;
    }

    if (!user.checkPassword(password)) {
        emit loginFailed("Неверный логин или пароль");
        return;
    }

    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
    }

    m_isAuthenticated = true;
    m_currentUser = new User(user);


    emit loginSuccess(user.username, user.role, user.fullName, user.email);

    if(user.mustChangePassword){
        emit passwordChangeRequired();
    }

    emit authenticatedChanged();
    emit userChanged();
}

void AuthManager::logout() {
    m_isAuthenticated = false;

    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
    }

    emit loggedOut();
    emit authenticatedChanged();
    emit userChanged();
}

void AuthManager::registerUser(const QString &username, const QString &password, const QString &role, const QString &fullName, const QString &email){
    if(!m_db){
        emit registrationFailed("Database is not initialized");
        return;
    }
    
    if(!currentUserIsAdmin()){
        emit registrationFailed("Только администратор может создать пользователя");
        return;
    }
    
    if(m_db -> userExists(username)){
        emit registrationFailed("Пользователь с таким логином уже существует");
        return;
    }

    User newUser;
    newUser.id = "";
    newUser.username = username;
    newUser.role = role;
    newUser.fullName = fullName;
    newUser.email = email;
    newUser.isActive = true;
    newUser.mustChangePassword = true;
    newUser.createdAt = QDateTime::currentDateTime();

    QString salt = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newUser.setPassword(password, salt);
    
    if(!m_db -> createUser(newUser)){
        emit registrationFailed("Failed to create user: " + m_db -> lastError());
        return;
    }

    emit userRegistered(username);
}

void AuthManager::updateUser(const QString &userId, const QString &username, const QString &fullName,const  QString &role, const QString &email, bool isActive){

    if(!m_db){
        emit registrationFailed("Database is not initialized");
        return;
    }

    if(!currentUserIsAdmin()){
        emit updateFailed("current user is not Admin");
        return;
    }

    User userToUpdate = m_db -> findUserById(userId);

    if(userToUpdate.id.isEmpty()){
        emit updateFailed("User not found");
        return;
    }

    if(userToUpdate.username != username && m_db -> userExists(username)){
        emit updateFailed("Username already in use");
        return;
    }

    userToUpdate.username = username;
    userToUpdate.fullName = fullName;
    userToUpdate.role = role;
    userToUpdate.email = email;
    userToUpdate.isActive = isActive;

    if(!m_db -> updateUser(userToUpdate)){
        emit updateFailed("Failed to update user: " + m_db -> lastError());
        return;
    }

    emit userUpdated(userId);
}

void AuthManager::deleteUser(const QString &userId){
    if(!m_db){
        emit deletionFailed("Database is not initialized");
        return;
    }

    if(!currentUserIsAdmin()){
        emit deletionFailed("current user is not admin");
        return;
    }

    User userToDelete = m_db -> findUserById(userId);

    if(userToDelete.id.isEmpty()){
        emit deletionFailed("User not found");
        return;
    }

    if(m_currentUser == nullptr || userToDelete.id == m_currentUser -> id){
        emit deletionFailed("can't delete yourself");
        return;
    }

    if(!m_db -> deleteUser(userId)){
        emit deletionFailed("Failed to deleted user: " + m_db -> lastError());
        return;
    }

    emit userDeleted(userId);

}

bool AuthManager::changePassword(const QString &currentPassword, const QString &newPassword) {
    if (!m_currentUser) return false;

    User user = m_db->findUserById(m_currentUser->id);

    if (!user.checkPassword(currentPassword)) {
        emit errorOccured("Текущий пароль введён неверно");
        return false;
    }

    if (newPassword.length() < 6) {
        emit errorOccured("Новый пароль должен быть не менее 6 символов");
        return false;
    }

    if (newPassword == currentPassword) {
        emit errorOccured("Новый пароль должен отличаться от текущего");
        return false;
    }

    QString newSalt = m_db->generateSalt();
    user.setPassword(newPassword, newSalt);
    user.mustChangePassword = false;

    if (!m_db->updateUser(user)) {
        emit errorOccured("Ошибка сохранения пароля");
        return false;
    }

    delete m_currentUser;
    m_currentUser = new User(user);

    emit passwordChanged();
    return true;
}