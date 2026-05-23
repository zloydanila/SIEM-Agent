#ifndef EVENTLISTMODEL_H
#define EVENTLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QDateTime>

class DatabaseService;
class Event;

class EventListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)

public:
    enum Roles {
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
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasMore() const;
    int totalCount() const;
    Q_INVOKABLE int count();

public slots:
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void appendNew();

signals:
    void hasMoreChanged();
    void totalCountChanged();

private:
    DatabaseService *m_db;
    QVector<Event> m_events;
    QString m_lastId;
    QDateTime m_lastTimestamp;
    bool m_hasMore = false;
    int m_totalCount = 0;
    static constexpr int PAGE_SIZE = 50;
};

#endif