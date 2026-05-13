#ifndef USERLISTMODEL_H
#define USERLISTMODEL_H

#include <QAbstractListModel>      
#include <QModelIndex>               
#include <QVariant>            
#include <QVector>

#include "User.h"

class DatabaseService;

class UserListModel : public QAbstractListModel {
    Q_OBJECT

public:

    enum Roles{
        UsernameRole = Qt::UserRole + 1,
        FullNameRole,
        EmailRole,
        RoleRole,
        IsActiveRole,
        IdRole
    };

    explicit UserListModel(DatabaseService *dbService, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override; 

    Q_INVOKABLE void refresh();

private:
    DatabaseService *m_dbService;
    QVector<User> m_users;
};

#endif