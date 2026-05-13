#ifndef ALERTLISTMODEL_H
#define ALERTLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "Alert.h"

class DatabaseService;

class AlertListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        SeverityRole,
        StatusRole,
        DeviceNameRole,
        TriggeredAtRole,
        RuleIdRole,
        AssignedToRole,
        CommentRole,
        RelatedEventIdsRole
    };

    explicit AlertListModel(DatabaseService *db, QObject *parent = nullptr);

    int     rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void updateStatus(const QString &id, const QString &status);
    int totalCount() const;

public slots:
    void refresh();
    void appendNew();  

signals:
    void totalCountChanged();

private:
    DatabaseService *m_db;
    QVector<Alert>   m_alerts;
    int              m_totalCount = 0;
};

#endif