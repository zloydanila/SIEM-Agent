#ifndef EVENTLISTMODEL_H
#define EVENTLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "Event.h"

class DatabaseService;

class EventListModel: public QAbstractListModel{
    Q_OBJECT
public:
    enum Roles{
        IdRole = Qt::UserRole + 1,
        DeviceNameRole,
        EventTypeRole,
        ActionRole,
        SeverityRole,
        TimestampRole,
        RawLogRole,
        LocationRole
    };

    explicit EventListModel(DatabaseService *db, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(int count READ count NOTIFY hasMoreChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    bool hasMore() const;
    int totalCount() const;
public slots:
    void refresh();
    void loadMore();
    void appendNew();
    int count();
    

signals:
    void hasMoreChanged();
    void totalCountChanged();

private:
    DatabaseService *m_db;
    QVector<Event> m_events;
    bool m_hasMore = true;
    int m_totalCount = 0;
    QString m_lastId;
    QDateTime m_lastTimestamp;
    static constexpr int PAGE_SIZE = 100;
};

#endif