#include "UserListModel.h"
#include "../services/DatabaseService.h"

UserListModel::UserListModel(DatabaseService *dbService, QObject *parent)
    : QAbstractListModel(parent),
      m_dbService(dbService) {
    refresh();
}

int UserListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_users.size();
}

QVariant UserListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_users.size()) {
        return {};
    }

    const User &user = m_users[index.row()];

    switch (role) {
    case UsernameRole:  return user.username;
    case FullNameRole:  return user.fullName;
    case EmailRole:  return user.email;
    case RoleRole:  return user.role;
    case IsActiveRole:  return user.isActive;
    case IdRole:  return user.id;
    }

    return {};
}


QHash<int, QByteArray> UserListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[UsernameRole] = "username";
    roles[FullNameRole] = "fullName";
    roles[EmailRole] = "email";
    roles[RoleRole] = "role";
    roles[IsActiveRole] = "isActive";
    roles[IdRole] = "id";
    return roles;
}

void UserListModel::refresh() {
    if (!m_dbService) return;

    QVector<User> newUsers = m_dbService->getAllUsers();

    beginResetModel();
    m_users = newUsers;
    endResetModel();
}